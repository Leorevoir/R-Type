#include "plugins/ui_sfx.hpp"

#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Filepath.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Plugins/AudioPlugin.hpp>
#include <R-Engine/Plugins/Ui/Systems.hpp>
#include <R-Engine/UI/Events.hpp>
#include <R-Engine/UI/Button.hpp>

/* Simple resource to store loaded SFX handles */
struct UiSfxHandles {
    r::AudioHandle select = r::AudioInvalidHandle;
    r::AudioHandle click = r::AudioInvalidHandle;
};

/* UiSfxTag / UiSfxBorn / UiSfxCounter are declared in the header so other plugins can use them */
#include <plugins/ui_sfx.hpp>

static void ui_sfx_startup_load(r::ecs::ResMut<r::AudioManager> audio, r::ecs::ResMut<UiSfxHandles> sfx)
{
    /* Load the two SFX from the project's assets folder */
    sfx.ptr->select = audio.ptr->load(r::path::get("assets/sounds/select.mp3"));
    sfx.ptr->click = audio.ptr->load(r::path::get("assets/sounds/click.mp3"));

    if (sfx.ptr->select == r::AudioInvalidHandle) {
        r::Logger::warn("Failed to load assets/sounds/select.mp3");
    }
    if (sfx.ptr->click == r::AudioInvalidHandle) {
        r::Logger::warn("Failed to load assets/sounds/click.mp3");
    }
    r::Logger::info(std::string{"UiSfx: handles -> select="} + std::to_string(sfx.ptr->select) + ", click=" + std::to_string(sfx.ptr->click));
}

/* Spawn a short-lived audio entity when the pointer enters a UI handle
   (buttons are targeted by the pointer system) */
static void ui_sfx_entered_system(r::ecs::ResMut<r::UiEvents> events, r::ecs::ResMut<UiSfxHandles> sfx,
    r::ecs::Res<UiSfxCounter> counter, r::ecs::Commands &commands)
{
    if (events.ptr->entered.empty())
        return;

    if (sfx.ptr->select == r::AudioInvalidHandle)
        return;
    for (const auto h : events.ptr->entered) {
        if (h == r::ecs::NULL_ENTITY)
            continue;
        /* Spawn an entity with an AudioPlayer + AudioSink so the AudioPlugin plays it.
           Tag it and record born frame so we keep it until next frame (guarantee audio system
           can detect it and call PlaySound once). We set born to current counter value. */
        commands.spawn(UiSfxTag{}, UiSfxBorn{counter.ptr->frame}, r::AudioPlayer{ sfx.ptr->select }, r::AudioSink{});
    }
}

/* Spawn a short-lived audio entity on UI clicks */
static void ui_sfx_click_system(r::ecs::EventReader<r::UiClick> click_reader, r::ecs::ResMut<UiSfxHandles> sfx,
    r::ecs::Res<UiSfxCounter> counter, r::ecs::Commands &commands)
{
    if (sfx.ptr->click == r::AudioInvalidHandle)
        return;
    for (const auto &c : click_reader) {
        if (c.entity == r::ecs::NULL_ENTITY)
            continue;
        r::Logger::info(std::string{"UiSfx: click event for entity "} + std::to_string(static_cast<u32>(c.entity)) + ", spawning handle=" + std::to_string(sfx.ptr->click));
        commands.spawn(UiSfxTag{}, UiSfxBorn{counter.ptr->frame}, r::AudioPlayer{ sfx.ptr->click }, r::AudioSink{});
    }
}

static void ui_sfx_frame_tick(r::ecs::ResMut<UiSfxCounter> counter)
{
    counter.ptr->frame += 1;
}

/* Cleanup transient UI SFX entities once we've advanced at least one frame so the
    audio system had a chance to process them and start playback. */
static void ui_sfx_cleanup_system(r::ecs::Commands &cmds, r::ecs::Res<UiSfxCounter> counter,
    r::ecs::Query<r::ecs::Ref<UiSfxBorn>, r::ecs::With<UiSfxTag>> q)
{
    const auto cur = counter.ptr->frame;
    for (auto it = q.begin(); it != q.end(); ++it) {
        auto [born, _] = *it;
        const auto bf = born.ptr->frame;
        if (cur > bf) {
            cmds.despawn(it.entity());
        }
    }
}
void UiSfxPlugin::build(r::Application &app)
{
    app
        .insert_resource(UiSfxHandles{})
    .add_systems<ui_sfx_startup_load>(r::Schedule::STARTUP)

    /* per-frame counter */
    .insert_resource(UiSfxCounter{})
    .add_systems<ui_sfx_frame_tick>(r::Schedule::UPDATE)

        /* Spawn SFX in response to UI hover (entered) and clicks. Schedule in UPDATE and ensure
           pointer_system has already produced events by running after it. */
        .add_systems<ui_sfx_entered_system>(r::Schedule::UPDATE)
        .after<r::ui::pointer_system>()
        .add_systems<ui_sfx_click_system>(r::Schedule::UPDATE)
        .after<r::ui::pointer_system>()
        .run_if<r::run_conditions::on_event<r::UiClick>>()
        /* Remove transient UI SFX entities after the audio system had a chance to start
           playback (audio systems run during UPDATE). Running cleanup in RENDER_2D
           ensures we don't leave AudioPlayer entities around that would cause replay. */
        .add_systems<ui_sfx_cleanup_system>(r::Schedule::RENDER_2D);
}
