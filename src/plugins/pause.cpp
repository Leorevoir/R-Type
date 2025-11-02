#include "plugins/pause.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/Core/States.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Plugins/InputPlugin.hpp>
#include <R-Engine/Plugins/UiPlugin.hpp>
#include <R-Engine/UI/Button.hpp>
#include <R-Engine/UI/Components.hpp>
#include <R-Engine/UI/Events.hpp>
#include <R-Engine/UI/Text.hpp>

#include <components/ui.hpp>
#include <resources/ui_state.hpp>
#include <state/game_state.hpp>
#include <string>

static void build_pause_menu(r::ecs::Commands &cmds)
{
    cmds.spawn(PauseRoot{}, r::UiNode{},
            r::Style{
                .width_pct = 100.f,
                .height_pct = 100.f,
                .background = r::Color{0, 0, 0, 180}, /* Semi-transparent black */
                .direction = r::LayoutDirection::Column,
                .justify = r::JustifyContent::Center,
                .align = r::AlignItems::Center,
                .gap = 10.f,
            },
            r::ComputedLayout{}, r::Visibility::Visible)
        .with_children([&](r::ecs::ChildBuilder &parent) {
            parent.spawn(r::UiNode{},
                r::Style{
                    .height = 100.f,
                    .width_pct = 100.f,
                    .justify = r::JustifyContent::Center,
                    .align = r::AlignItems::Center,
                },
                r::UiText{.content = std::string("PAUSED"), .font_size = 96, .color = r::Color{255, 255, 255, 255}, .font_path = {}},
                r::ComputedLayout{}, r::Visibility::Visible);

            auto create_button = [&](const std::string &text, PauseMenuButton::Action action) {
                parent.spawn(r::UiNode{}, r::UiButton{}, PauseMenuButton{action},
                    r::Style{.width = 280.f,
                        .height = 45.f,
                        .direction = r::LayoutDirection::Column,
                        .justify = r::JustifyContent::Center,
                        .align = r::AlignItems::Center},
                    r::UiText{.content = text, .font_size = 22, .font_path = {}}, r::ComputedLayout{}, r::Visibility::Visible);
            };

            create_button("Resume", PauseMenuButton::Action::Resume);
            create_button("Options", PauseMenuButton::Action::Options);
            create_button("Main Menu", PauseMenuButton::Action::BackToMenu);
        });
}

static void cleanup_pause_menu(r::ecs::Commands &cmds, r::ecs::Query<r::ecs::With<PauseRoot>> query)
{
    for (auto it = query.begin(); it != query.end(); ++it) {
        cmds.despawn(it.entity());
    }
}

static void pause_menu_button_handler(r::ecs::EventReader<r::UiClick> click_reader, r::ecs::Query<r::ecs::Ref<PauseMenuButton>> buttons,
    r::ecs::ResMut<r::NextState<GameState>> next_state, r::ecs::ResMut<PreviousGameState> prev_game_state,
    r::ecs::Res<r::State<GameState>> current_state, r::ecs::Res<StateBeforePause> state_before_pause)
{
    for (const auto &click : click_reader) {
        if (click.entity == r::ecs::NULL_ENTITY)
            continue;

        for (auto it = buttons.begin(); it != buttons.end(); ++it) {
            if (it.entity() != click.entity)
                continue;

            auto [btn] = *it;
            switch (btn.ptr->action) {
                case PauseMenuButton::Action::Resume:
                    next_state.ptr->set(state_before_pause.ptr->state);
                    break;
                case PauseMenuButton::Action::Options:
                    prev_game_state.ptr->state = current_state.ptr->current();
                    next_state.ptr->set(GameState::SettingsMenu);
                    break;
                case PauseMenuButton::Action::BackToMenu:
                    next_state.ptr->set(GameState::MainMenu);
                    break;
                default:
                    break;
            }
            return;
        }
    }
}

static void check_for_pause_system(r::ecs::Res<r::UserInput> user_input, r::ecs::Res<r::InputMap> input_map,
    r::ecs::ResMut<r::NextState<GameState>> next_state, r::ecs::Res<r::State<GameState>> current_state,
    r::ecs::ResMut<StateBeforePause> state_before_pause)
{
    if (input_map.ptr->isActionPressed("Pause", *user_input.ptr)) {
        r::Logger::info("Pause button pressed. Pausing game.");
        state_before_pause.ptr->state = current_state.ptr->current();
        next_state.ptr->set(GameState::Paused);
    }
}

void PausePlugin::build(r::Application &app)
{
    app.insert_resource(StateBeforePause{})
        .add_systems<build_pause_menu>(r::OnEnter{GameState::Paused})
        .add_systems<cleanup_pause_menu>(r::OnExit{GameState::Paused})
        .add_systems<pause_menu_button_handler>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::Paused>>()
        .run_if<r::run_conditions::on_event<r::UiClick>>()
        .add_systems<check_for_pause_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()
        .run_or<r::run_conditions::in_state<GameState::BossBattle>>();
}
