#include "plugins/settings.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Backend.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Plugins/PostProcessingPlugin.hpp>
#include <R-Engine/Plugins/UiPlugin.hpp>
#include <R-Engine/Plugins/WindowPlugin.hpp>
#include <R-Engine/UI/Button.hpp>
#include <R-Engine/UI/Components.hpp>
#include <R-Engine/UI/Events.hpp>
#include <R-Engine/UI/Text.hpp>
#include <R-Engine/UI/Theme.hpp>

#include <components/ui.hpp>
#include <resources/ui_state.hpp>
#include <resources/video_settings.hpp>
#include <state/game_state.hpp>
#include <string>

static void create_video_settings_content(r::ecs::ChildBuilder &parent)
{
    parent
        .spawn(VideoSettingsRoot{}, r::UiNode{},
            r::Style{
                .width_pct = 100.f,
                .height_pct = 100.f,
                .direction = r::LayoutDirection::Column,
                .justify = r::JustifyContent::Start,
                .align = r::AlignItems::Start,
                .gap = 10.f,
            },
            r::ComputedLayout{}, r::Visibility::Visible)
        .with_children([&](r::ecs::ChildBuilder &content) {
            auto create_row = [&](const std::string &label, auto component_tag, const std::string &initial_value) {
                /* Each row is a container */
                content
                    .spawn(r::UiNode{},
                        r::Style{.height = 40.f,
                            .width_pct = 100.f,
                            .direction = r::LayoutDirection::Row,
                            .justify = r::JustifyContent::Start,
                            .align = r::AlignItems::Center},
                        r::ComputedLayout{}, r::Visibility::Visible)
                    .with_children([&](r::ecs::ChildBuilder &row) {
                        /* Left column: Label (takes up 60% of the row's width) */
                        row.spawn(r::UiNode{}, r::Style{.width_pct = 60.f, .align = r::AlignItems::Center},
                            r::UiText{.content = label, .color = r::Color{200, 230, 235, 255}}, r::ComputedLayout{},
                            r::Visibility::Visible);

                        /* Right column: Button (takes up 40% of the row's width) */
                        row.spawn(component_tag, r::UiNode{}, r::UiButton{},
                            r::Style{
                                .height = 35.f,
                                .width_pct = 40.f,
                                .justify = r::JustifyContent::Center,
                                .align = r::AlignItems::Center,
                            },
                            r::UiText{.content = initial_value}, r::ComputedLayout{}, r::Visibility::Visible);
                    });
            };

            create_row("Display Mode", DisplayModeDropdown{}, "Windowed");
            create_row("Resolution", ResolutionDropdown{}, "1280x720");
            create_row("V-Sync", VSyncToggle{}, "On");
            create_row("Framerate Limit", FramerateLimitSlider{}, "60");
            create_row("Post-Processing", PostProcessingToggle{}, "Off");
        });
}

static void create_audio_settings_content(r::ecs::ChildBuilder &parent)
{
    parent.spawn(AudioSettingsRoot{}, r::UiNode{}, r::UiText{.content = "Audio Settings Here"}, r::Style{}, r::ComputedLayout{},
        r::Visibility::Hidden);
}
static void create_controls_settings_content(r::ecs::ChildBuilder &parent)
{
    parent.spawn(ControlsSettingsRoot{}, r::UiNode{}, r::UiText{.content = "Controls Settings Here"}, r::Style{}, r::ComputedLayout{},
        r::Visibility::Hidden);
}
static void create_accessibility_settings_content(r::ecs::ChildBuilder &parent)
{
    parent.spawn(AccessibilitySettingsRoot{}, r::UiNode{}, r::UiText{.content = "Accessibility Settings Here"}, r::Style{},
        r::ComputedLayout{}, r::Visibility::Hidden);
}

static void build_settings_menu(r::ecs::Commands &cmds)
{
    /* Root: Row with sidebar (left) and content (right) */
    cmds.spawn(SettingsRoot{}, r::UiNode{},
            r::Style{
                .width_pct = 100.f,
                .height_pct = 100.f,
                .background = r::Color{8, 8, 10, 255}, /* Dark background */
                .direction = r::LayoutDirection::Row,
            },
            r::ComputedLayout{}, r::Visibility::Visible)
        .with_children([&](r::ecs::ChildBuilder &parent) {
            /* --- Left Sidebar --- */
            parent
                .spawn(r::UiNode{},
                    r::Style{
                        .width = 220.f,
                        .height_pct = 100.f,
                        .background = r::Color{14, 14, 16, 255},
                        .padding = 20.f,
                        .direction = r::LayoutDirection::Column,
                        .justify = r::JustifyContent::SpaceBetween,
                        .align = r::AlignItems::Center,
                        .gap = 8.f,
                    },
                    r::ComputedLayout{}, r::Visibility::Visible)
                .with_children([&](r::ecs::ChildBuilder &side) {
                    /* Container for top buttons */
                    side.spawn(r::UiNode{},
                            r::Style{.width_pct = 100.f,
                                .direction = r::LayoutDirection::Column,
                                .align = r::AlignItems::Center,
                                .gap = 8.f},
                            r::ComputedLayout{}, r::Visibility::Visible)
                        .with_children([&](r::ecs::ChildBuilder &col) {
                            /* Helper */
                            auto create_tab = [&](const std::string &text, SettingsMenuButton::Action action, float font_size = 20.f) {
                                col.spawn(r::UiNode{}, r::UiButton{}, SettingsMenuButton{action},
                                    r::Style{
                                        .width = 180.f,
                                        .height = 80.f,
                                        .justify = r::JustifyContent::Center,
                                        .align = r::AlignItems::Center,
                                    },
                                    r::UiText{.content = text, .font_size = static_cast<int>(font_size), .font_path = {}},
                                    r::ComputedLayout{}, r::Visibility::Visible);
                            };
                            create_tab("Video", SettingsMenuButton::Action::Video);
                            create_tab("Audio", SettingsMenuButton::Action::Audio);
                            create_tab("Controls", SettingsMenuButton::Action::Controls);
                            create_tab("Accessibility", SettingsMenuButton::Action::Accessibility, 18.f);
                        });

                    /* "Back" button at the bottom */
                    side.spawn(r::UiNode{}, r::UiButton{}, SettingsMenuButton{SettingsMenuButton::Action::Back},
                        r::Style{
                            .width = 180.f,
                            .height = 60.f,
                            .justify = r::JustifyContent::Center,
                            .align = r::AlignItems::Center,
                        },
                        r::UiText{.content = std::string("Back"), .font_size = 20, .font_path = {}}, r::ComputedLayout{},
                        r::Visibility::Visible);
                });

            /* --- Right Content Panel --- */
            parent
                .spawn(r::UiNode{},
                    r::Style{
                        .height_pct = 100.f,
                        .padding = 20.f,
                        .direction = r::LayoutDirection::Column,
                        .justify = r::JustifyContent::Start,
                        .align = r::AlignItems::Start,
                        .gap = 12.f,
                    },
                    r::ComputedLayout{}, r::Visibility::Visible)
                .with_children([&](r::ecs::ChildBuilder &content) {
                    /* Title that displays the current tab */
                    content.spawn(r::UiNode{}, SettingsTitleText{}, r::Style{.height = 56.f, .justify = r::JustifyContent::Center},
                        r::UiText{.content = std::string("Video"), .font_size = 28, .color = r::Color{200, 230, 235, 255}, .font_path = {}},
                        r::ComputedLayout{}, r::Visibility::Visible);
                    /* Content area that holds the specific settings panels */
                    content
                        .spawn(r::UiNode{},
                            r::Style{
                                .width = 700.f,
                                .height_pct = 100.f,
                                .background = r::Color{12, 12, 14, 255},
                                .padding = 12.f,
                            },
                            r::ComputedLayout{}, r::Visibility::Visible)
                        .with_children([&](r::ecs::ChildBuilder &settings_area) {
                            create_video_settings_content(settings_area);
                            create_audio_settings_content(settings_area);
                            create_controls_settings_content(settings_area);
                            create_accessibility_settings_content(settings_area);
                        });
                });
        });
}

static void cleanup_settings_menu(r::ecs::Commands &cmds, r::ecs::Query<r::ecs::With<SettingsRoot>> query)
{
    for (auto it = query.begin(); it != query.end(); ++it) {
        cmds.despawn(it.entity());
    }
}

static void settings_sidebar_handler(r::ecs::EventReader<r::UiClick> click_reader, r::ecs::Query<r::ecs::Ref<SettingsMenuButton>> buttons,
    r::ecs::Query<r::ecs::Mut<r::UiText>, r::ecs::With<SettingsTitleText>> title,
    r::ecs::Query<r::ecs::Mut<r::Visibility>, r::ecs::With<VideoSettingsRoot>> video_view,
    r::ecs::Query<r::ecs::Mut<r::Visibility>, r::ecs::With<AudioSettingsRoot>> audio_view,
    r::ecs::Query<r::ecs::Mut<r::Visibility>, r::ecs::With<ControlsSettingsRoot>> controls_view,
    r::ecs::Query<r::ecs::Mut<r::Visibility>, r::ecs::With<AccessibilitySettingsRoot>> accessibility_view,
    r::ecs::ResMut<r::NextState<GameState>> next_state, r::ecs::Res<PreviousGameState> prev_game_state)
{
    for (const auto &click : click_reader) {
        if (click.entity == r::ecs::NULL_ENTITY)
            continue;

        for (auto it = buttons.begin(); it != buttons.end(); ++it) {
            if (it.entity() != click.entity)
                continue;

            auto [btn] = *it;
            std::string new_label;

            /* Hide all views first */
            for (auto [vis, _] : video_view) {
                *vis.ptr = r::Visibility::Hidden;
            }
            for (auto [vis, _] : audio_view) {
                *vis.ptr = r::Visibility::Hidden;
            }
            for (auto [vis, _] : controls_view) {
                *vis.ptr = r::Visibility::Hidden;
            }
            for (auto [vis, _] : accessibility_view) {
                *vis.ptr = r::Visibility::Hidden;
            }

            switch (btn.ptr->action) {
                case SettingsMenuButton::Action::Video:
                    new_label = "Video";
                    for (auto [vis, _] : video_view) {
                        *vis.ptr = r::Visibility::Visible;
                    }
                    break;
                case SettingsMenuButton::Action::Audio:
                    new_label = "Audio";
                    for (auto [vis, _] : audio_view) {
                        *vis.ptr = r::Visibility::Visible;
                    }
                    break;
                case SettingsMenuButton::Action::Controls:
                    new_label = "Controls";
                    for (auto [vis, _] : controls_view) {
                        *vis.ptr = r::Visibility::Visible;
                    }
                    break;
                case SettingsMenuButton::Action::Accessibility:
                    new_label = "Accessibility";
                    for (auto [vis, _] : accessibility_view) {
                        *vis.ptr = r::Visibility::Visible;
                    }
                    break;
                case SettingsMenuButton::Action::Back:
                    r::Logger::info("Settings: Back button clicked. Returning to previous state.");
                    next_state.ptr->set(prev_game_state.ptr->state);
                    return;
                default:
                    break;
            }

            if (!new_label.empty()) {
                for (auto [text, _] : title) {
                    text.ptr->content = new_label;
                }
                r::Logger::info("Settings tab changed to: " + new_label);
            }
            return;
        }
    }
}

static void sync_video_settings_ui(r::ecs::Res<VideoSettings> settings,
    r::ecs::Query<r::ecs::Mut<r::UiText>, r::ecs::With<DisplayModeDropdown>> display_mode_q,
    r::ecs::Query<r::ecs::Mut<r::UiText>, r::ecs::With<ResolutionDropdown>> resolution_q,
    r::ecs::Query<r::ecs::Mut<r::UiText>, r::ecs::With<VSyncToggle>> vsync_q,
    r::ecs::Query<r::ecs::Mut<r::UiText>, r::ecs::With<FramerateLimitSlider>> framerate_q,
    r::ecs::Query<r::ecs::Mut<r::UiText>, r::ecs::With<PostProcessingToggle>> post_processing_q)
{
    for (auto [text, _] : display_mode_q) {
        switch (settings.ptr->display_mode) {
            case DisplayMode::Fullscreen:
                text.ptr->content = "Fullscreen";
                break;
            case DisplayMode::Windowed:
                text.ptr->content = "Windowed";
                break;
            case DisplayMode::BorderlessWindowed:
                text.ptr->content = "Borderless";
                break;
            default:
                break;
        }
    }
    for (auto [text, _] : resolution_q) {
        text.ptr->content = std::to_string(settings.ptr->resolution.width) + "x" + std::to_string(settings.ptr->resolution.height);
    }
    for (auto [text, _] : vsync_q) {
        text.ptr->content = settings.ptr->vsync ? "On" : "Off";
    }
    for (auto [text, _] : framerate_q) {
        text.ptr->content = settings.ptr->framerate_limit == 0 ? "Uncapped" : std::to_string(settings.ptr->framerate_limit);
    }
    for (auto [text, _] : post_processing_q) {
        text.ptr->content = settings.ptr->post_processing_effects ? "On" : "Off";
    }
}

static void update_resolution_button_state(r::ecs::Res<VideoSettings> settings,
    r::ecs::Query<r::ecs::Mut<r::UiButton>, r::ecs::With<ResolutionDropdown>> resolution_button_q)
{
    for (auto [button, _] : resolution_button_q) {
        button.ptr->disabled = (settings.ptr->display_mode != DisplayMode::Windowed);
    }
}

static void video_settings_button_handler(r::ecs::EventReader<r::UiClick> click_reader, r::ecs::ResMut<VideoSettings> settings,
    r::ecs::Query<r::ecs::With<DisplayModeDropdown>> display_mode_q, r::ecs::Query<r::ecs::With<ResolutionDropdown>> resolution_q,
    r::ecs::Query<r::ecs::With<VSyncToggle>> vsync_q, r::ecs::Query<r::ecs::With<FramerateLimitSlider>> framerate_q,
    r::ecs::Query<r::ecs::With<PostProcessingToggle>> post_processing_q)
{
    for (const auto &click : click_reader) {
        auto it_display = display_mode_q.begin();
        if (it_display != display_mode_q.end() && it_display.entity() == click.entity) {
            settings.ptr->display_mode = static_cast<DisplayMode>((static_cast<int>(settings.ptr->display_mode) + 1) % 3);
            return;
        }

        auto it_res = resolution_q.begin();
        if (it_res != resolution_q.end() && it_res.entity() == click.entity) {
            if (settings.ptr->resolution.width == 1280)
                settings.ptr->resolution = {1920, 1080};
            else if (settings.ptr->resolution.width == 1920)
                settings.ptr->resolution = {2560, 1440};
            else
                settings.ptr->resolution = {1280, 720};
            return;
        }

        auto it_vsync = vsync_q.begin();
        if (it_vsync != vsync_q.end() && it_vsync.entity() == click.entity) {
            settings.ptr->vsync = !settings.ptr->vsync;
            return;
        }

        auto it_fps = framerate_q.begin();
        if (it_fps != framerate_q.end() && it_fps.entity() == click.entity) {
            if (settings.ptr->framerate_limit == 60)
                settings.ptr->framerate_limit = 120;
            else if (settings.ptr->framerate_limit == 120)
                settings.ptr->framerate_limit = 144;
            else if (settings.ptr->framerate_limit == 144)
                settings.ptr->framerate_limit = 0; /* Uncapped */
            else
                settings.ptr->framerate_limit = 60;
            return;
        }

        auto it_pp = post_processing_q.begin();
        if (it_pp != post_processing_q.end() && it_pp.entity() == click.entity) {
            settings.ptr->post_processing_effects = !settings.ptr->post_processing_effects;
            settings.ptr->selected_effect =
                settings.ptr->post_processing_effects ? r::PostProcessingState::Bloom : r::PostProcessingState::Disabled;
            return;
        }
    }
}

static void apply_video_settings(r::ecs::Res<VideoSettings> settings, r::ecs::ResMut<r::WindowPluginConfig> window_config,
    r::ecs::ResMut<r::PostProcessingPluginConfig> pp_config)
{
    /* VSync */
    if (settings.ptr->vsync)
        window_config.ptr->settings |= r::WindowPluginSettings(FLAG_VSYNC_HINT);
    else
        window_config.ptr->settings &= ~r::WindowPluginSettings(FLAG_VSYNC_HINT);

    /* Display Mode & Resolution */
    if (settings.ptr->display_mode == DisplayMode::Fullscreen) {
        window_config.ptr->settings |= r::WindowPluginSettings::MAXIMIZED;
    } else {
        window_config.ptr->settings &= ~r::WindowPluginSettings::MAXIMIZED;
    }

    if (settings.ptr->display_mode == DisplayMode::Windowed) {
        window_config.ptr->settings |= r::WindowPluginSettings::DECORATED;
        window_config.ptr->size = settings.ptr->resolution;
    } else if (settings.ptr->display_mode == DisplayMode::BorderlessWindowed) {
        window_config.ptr->settings &= ~r::WindowPluginSettings::DECORATED;
    }

    /* Framerate */
    window_config.ptr->frame_per_second = static_cast<u32>(settings.ptr->framerate_limit);

    /* Post Processing */
    pp_config.ptr->state = settings.ptr->selected_effect;

    r::Logger::info("Staged video settings for application.");
}

void SettingsPlugin::build(r::Application &app)
{
    app.insert_resource(VideoSettings{})
        .add_systems<build_settings_menu>(r::OnEnter{GameState::SettingsMenu})
        .add_systems<cleanup_settings_menu>(r::OnExit{GameState::SettingsMenu})
        .add_systems<apply_video_settings>(r::OnExit{GameState::SettingsMenu})
        .add_systems<settings_sidebar_handler>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::SettingsMenu>>()
        .run_if<r::run_conditions::on_event<r::UiClick>>()
        .add_systems<video_settings_button_handler>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::SettingsMenu>>()
        .run_if<r::run_conditions::on_event<r::UiClick>>()
        .add_systems<sync_video_settings_ui>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::SettingsMenu>>()
        .add_systems<update_resolution_button_state>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::SettingsMenu>>();
}
