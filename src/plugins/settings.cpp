#include "plugins/settings.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Plugins/UiPlugin.hpp>
#include <R-Engine/UI/Button.hpp>
#include <R-Engine/UI/Components.hpp>
#include <R-Engine/UI/Events.hpp>
#include <R-Engine/UI/Text.hpp>
#include <R-Engine/UI/Theme.hpp>

#include <components/ui.hpp>
#include <resources/ui_state.hpp>
#include <state/game_state.hpp>
#include <string>

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
                        .width_pct = 100.f,
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
                    /* Placeholder content area */
                    content.spawn(r::UiNode{},
                        r::Style{.width_pct = 100.f, .height_pct = 100.f, .background = r::Color{12, 12, 14, 255}, .padding = 12.f},
                        r::ComputedLayout{}, r::Visibility::Visible);
                });
        });
}

static void cleanup_settings_menu(r::ecs::Commands &cmds, r::ecs::Query<r::ecs::With<SettingsRoot>> query)
{
    for (auto it = query.begin(); it != query.end(); ++it) {
        cmds.despawn(it.entity());
    }
}

static void settings_menu_button_handler(r::ecs::EventReader<r::UiClick> click_reader,
    r::ecs::Query<r::ecs::Ref<SettingsMenuButton>> buttons, r::ecs::Query<r::ecs::Mut<r::UiText>, r::ecs::With<SettingsTitleText>> title,
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
            switch (btn.ptr->action) {
                case SettingsMenuButton::Action::Video:
                    new_label = "Video";
                    break;
                case SettingsMenuButton::Action::Audio:
                    new_label = "Audio";
                    break;
                case SettingsMenuButton::Action::Controls:
                    new_label = "Controls";
                    break;
                case SettingsMenuButton::Action::Accessibility:
                    new_label = "Accessibility";
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

void SettingsPlugin::build(r::Application &app)
{
    app.add_systems<build_settings_menu>(r::OnEnter{GameState::SettingsMenu})
        .add_systems<cleanup_settings_menu>(r::OnExit{GameState::SettingsMenu})
        .add_systems<settings_menu_button_handler>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::SettingsMenu>>()
        .run_if<r::run_conditions::on_event<r::UiClick>>();
}
