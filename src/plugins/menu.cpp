#include "plugins/menu.hpp"
#include "R-Engine/Components/Transform3d.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Plugins/Ui/Systems.hpp>
#include <R-Engine/Plugins/UiPlugin.hpp>
#include <R-Engine/UI/Button.hpp>
#include <R-Engine/UI/Components.hpp>
#include <R-Engine/UI/Events.hpp>
#include <R-Engine/UI/Image.hpp>
#include <R-Engine/UI/InputState.hpp>
#include <R-Engine/UI/Text.hpp>
#include <R-Engine/UI/Theme.hpp>
#include <R-Engine/Plugins/AudioPlugin.hpp>
#include <R-Engine/Core/Filepath.hpp>

#include <components/player.hpp>
#include <components/ui.hpp>
#include <resources/game_state.hpp>
#include <resources/ui_state.hpp>
#include <state/game_state.hpp>

/* ================================================================================= */
/* Menu Systems :: Helpers */
/* ================================================================================= */

static void create_menu_title(r::ecs::ChildBuilder &parent)
{
    parent.spawn(r::UiNode{},
        r::Style{
            .height = 200.f,
            .width_pct = 100.f,
            .background = r::Color{0, 0, 0, 1},
            .margin = 0.f,
            .padding = 0.f,
        },
        r::UiImage{
            .path = "assets/textures/r-type_title.png",
            .tint = r::Color{255, 255, 255, 255},
            .keep_aspect = true,
        },
        r::ComputedLayout{}, r::Visibility::Visible);
}

static void create_menu_button(r::ecs::ChildBuilder &parent, MenuButton::Action action, const std::string &text)
{
    parent.spawn(r::UiNode{}, r::UiButton{}, MenuButton{action},
        r::Style{.width = 280.f,
            .height = 45.f,
            .direction = r::LayoutDirection::Column,
            .justify = r::JustifyContent::Center,
            .align = r::AlignItems::Center},
        r::UiText{
            .content = text,
            .font_path = {},
        },
        r::ComputedLayout{}, r::Visibility::Visible);
}

/* ================================================================================= */
/* HUD Systems */
/* ================================================================================= */

static void build_game_hud(r::ecs::Commands &cmds)
{
    cmds.spawn(HudRoot{}, r::UiNode{},
            r::Style{
                .height = 40.f,
                .width_pct = 100.f,
                .background = r::Color{0, 0, 0, 100},
                .padding = 10.f,
                .direction = r::LayoutDirection::Row,
                .justify = r::JustifyContent::SpaceBetween,
                .align = r::AlignItems::Center,
                .position = r::PositionType::Absolute,
                .offset_y = 0.f,
            },
            r::ComputedLayout{}, r::Visibility::Visible)
        .with_children([&](r::ecs::ChildBuilder &parent) {
            parent.spawn(r::UiNode{}, ScoreText{},
                r::UiText{
                    .content = "Score: 0",
                    .font_size = 20,
                    .color = {255, 255, 255, 255},
                    .font_path = {},
                },
                r::Style{
                    .width = 200.f,
                    .background = r::Color{0, 0, 0, 0},
                },
                r::ComputedLayout{}, r::Visibility::Visible);
            parent.spawn(r::UiNode{}, LivesText{},
                r::UiText{
                    .content = "Lives: 3",
                    .font_size = 20,
                    .color = {255, 255, 255, 255},
                    .font_path = {},
                },
                r::Style{
                    .width = 100.f,
                    .background = r::Color{0, 0, 0, 0},
                    .align = r::AlignItems::End,
                },
                r::ComputedLayout{}, r::Visibility::Visible);
        });
}

static void update_game_hud(r::ecs::Res<PlayerScore> score, r::ecs::Res<PlayerLives> lives,
    r::ecs::Query<r::ecs::Mut<r::UiText>, r::ecs::With<ScoreText>> score_query,
    r::ecs::Query<r::ecs::Mut<r::UiText>, r::ecs::With<LivesText>> lives_query)
{
    for (auto [text, _] : score_query) {
        text.ptr->content = "Score: " + std::to_string(score.ptr->value);
    }
    for (auto [text, _] : lives_query) {
        text.ptr->content = "Lives: " + std::to_string(lives.ptr->count);
    }
}

static void cleanup_game_hud(r::ecs::Commands &cmds, r::ecs::Query<r::ecs::With<HudRoot>> hud_query)
{
    for (auto it = hud_query.begin(); it != hud_query.end(); ++it) {
        cmds.despawn(it.entity());
    }
}

/* ================================================================================= */
/* Menu Systems */
/* ================================================================================= */

static void setup_ui_theme(r::ecs::ResMut<r::UiTheme> theme, r::ecs::ResMut<r::UiPluginConfig> cfg)
{
    cfg.ptr->show_debug_overlay = false;

    /* R-Type cyan theme (#62DDFF) */
    theme.ptr->button.bg_normal = r::Color{0, 36, 48, 255};
    theme.ptr->button.bg_hover = r::Color{98, 221, 255, 100};
    theme.ptr->button.bg_pressed = r::Color{98, 221, 255, 150};
    theme.ptr->button.bg_disabled = r::Color{50, 50, 50, 255};

    theme.ptr->button.border_normal = r::Color{98, 221, 255, 255};
    theme.ptr->button.border_hover = r::Color{98, 221, 255, 255};
    theme.ptr->button.border_pressed = r::Color{98, 221, 255, 255};
    theme.ptr->button.border_disabled = r::Color{100, 100, 100, 255};

    theme.ptr->button.border_thickness = 2.f;
    theme.ptr->button.text = r::Color{98, 221, 255, 255};
}

static void build_main_menu(r::ecs::Commands &cmds)
{
    cmds.spawn(MenuRoot{}, r::UiNode{},
            r::Style{
                .width_pct = 100.f,
                .height_pct = 100.f,
                .background = r::Color{255, 255, 255, 40},
                .margin = 0.f,
                .padding = 0.f,
                .direction = r::LayoutDirection::Column,
                .justify = r::JustifyContent::Center,
                .align = r::AlignItems::Center,
                .gap = 10.f,
            },
            r::ComputedLayout{}, r::Visibility::Visible)
        .with_children([&](r::ecs::ChildBuilder &parent) {
            create_menu_title(parent);
            create_menu_button(parent, MenuButton::Action::Play, "Play");
            create_menu_button(parent, MenuButton::Action::Options, "Options");
            create_menu_button(parent, MenuButton::Action::Quit, "Quit");
        });
}

static void menu_button_handler(r::ecs::EventReader<r::UiClick> click_reader, r::ecs::Query<r::ecs::Ref<MenuButton>> buttons,
    r::ecs::ResMut<r::NextState<GameState>> next_state, r::ecs::ResMut<PreviousGameState> prev_game_state,
    r::ecs::Res<r::State<GameState>> current_state)
{
    for (const auto &click : click_reader) {
        const r::ecs::Entity clicked_entity = click.entity;
        if (clicked_entity == r::ecs::NULL_ENTITY) {
            continue;
        }

        MenuButton::Action action = MenuButton::Action::None;
        for (auto it = buttons.begin(); it != buttons.end(); ++it) {
            auto [btn] = *it;
            if (it.entity() == clicked_entity && btn.ptr) {
                action = btn.ptr->action;
                break;
            }
        }

        switch (action) {
            case MenuButton::Action::Play:
                r::Logger::info("Starting game...");
                next_state.ptr->set(GameState::EnemiesBattle);
                break;
            case MenuButton::Action::Options:
                r::Logger::info("Options clicked, opening settings menu...");
                prev_game_state.ptr->state = current_state.ptr->current();
                next_state.ptr->set(GameState::SettingsMenu);
                break;
            case MenuButton::Action::Quit:
                r::Logger::info("Quitting game...");
                r::Application::quit.store(true, std::memory_order_relaxed);
                break;
            default:
                break;
        }
    }
}

static void cleanup_menu(r::ecs::Commands &cmds, r::ecs::Query<r::ecs::Ref<MenuRoot>> menu_entities)
{
    for (auto it = menu_entities.begin(); it != menu_entities.end(); ++it) {
        cmds.despawn(it.entity());
    }
}

static void show_game_over_ui(r::ecs::Commands &cmds, r::ecs::Res<PlayerScore> score)
{
    cmds.spawn(GameOverRoot{}, r::UiNode{},
            r::Style{
                .width_pct = 100.f,
                .height_pct = 100.f,
                .background = r::Color{0, 0, 0, 150}, /* Semi-transparent black background */
                .direction = r::LayoutDirection::Column,
                .justify = r::JustifyContent::Center,
                .align = r::AlignItems::Center,
                .gap = 20.f,
            },
            r::ComputedLayout{}, r::Visibility::Visible)
        .with_children([&](r::ecs::ChildBuilder &parent) {
            parent.spawn(r::UiNode{},
                r::UiText{
                    .content = "GAME OVER",
                    .font_size = 80,
                    .color = {255, 50, 50, 255},
                    .font_path = {},
                },
                r::Style{
                    .height = 90.f,
                },
                r::ComputedLayout{}, r::Visibility::Visible);
            parent.spawn(r::UiNode{},
                r::UiText{
                    .content = "Final Score: " + std::to_string(score.ptr->value),
                    .font_size = 30,
                    .color = {200, 200, 200, 255},
                    .font_path = {},
                },
                r::Style{
                    .height = 40.f,
                },
                r::ComputedLayout{}, r::Visibility::Visible);
            parent.spawn(r::UiNode{},
                r::UiText{
                    .content = "Press ENTER to Restart",
                    .font_size = 30,
                    .color = {200, 200, 200, 255},
                    .font_path = {},
                },
                r::Style{
                    .height = 40.f,
                },
                r::ComputedLayout{}, r::Visibility::Visible);
        });
}

static void cleanup_game_over_ui(r::ecs::Commands &cmds, r::ecs::Query<r::ecs::With<GameOverRoot>> query)
{
    for (auto it = query.begin(); it != query.end(); ++it) {
        cmds.despawn(it.entity());
    }
}

/* Menu music tag and resource */
struct MenuMusicTag {};

/* Centralized resource tracking menu music handle + entity */
struct MenuMusicResource {
    r::AudioHandle handle = r::AudioInvalidHandle;
    r::ecs::Entity entity = r::ecs::NULL_ENTITY;
};

/* Startup: load menu music and spawn the entity once. We then control play/pause
   depending on GameState, avoiding repeated load/spawn. */
static void menu_music_startup(r::ecs::Commands &commands, r::ecs::ResMut<r::AudioManager> audio,
    r::ecs::Res<r::State<GameState>> current_state, r::ecs::ResMut<MenuMusicResource> menu_music)
{
    if (menu_music.ptr->handle != r::AudioInvalidHandle)
        return; /* already initialized */

    const std::string &path = r::path::get("assets/sounds/title.mp3");
    const auto handle = audio.ptr->load(path);
    if (handle == r::AudioInvalidHandle) {
        r::Logger::warn("Failed to load " + path);
        return;
    }
    menu_music.ptr->handle = handle;
    r::Logger::info(std::string{"Menu music handle="} + std::to_string(handle));

    /* Spawn AudioSink paused if we're not currently in MainMenu to avoid playing in-game */
    const bool start_paused = (current_state.ptr->current() != GameState::MainMenu);
    const auto ent = commands.spawn(MenuMusicTag{}, r::AudioPlayer{handle}, r::AudioSink{1.0f, 1.0f, start_paused, false}).id();
    menu_music.ptr->entity = ent;
}

[[maybe_unused]] static void cleanup_menu_music_system(r::ecs::Commands &commands, r::ecs::Query<r::ecs::With<MenuMusicTag>> query,
    r::ecs::ResMut<MenuMusicResource> menu_music)
{
    for (auto it = query.begin(); it != query.end(); ++it) {
        commands.despawn(it.entity());
        if (menu_music.ptr->entity == it.entity()) {
            menu_music.ptr->entity = r::ecs::NULL_ENTITY;
        }
    }
}

/* Stop menu music when entering any non-main-menu state */
/* Pause the menu music (don't unload) */
static void pause_menu_music_system(r::ecs::Query<r::ecs::Mut<r::AudioSink>, r::ecs::With<MenuMusicTag>> query)
{
    for (auto [sink, _] : query) {
        r::Logger::info("pause_menu_music_system: pausing menu music, was_playing=" + std::to_string(sink.ptr->is_playing()));
        sink.ptr->pause();
        r::Logger::info("pause_menu_music_system: paused, is_playing=" + std::to_string(sink.ptr->is_playing()));
    }
}

/* Immediately stop menu music when entering gameplay to guarantee silence in-game. */
static void stop_menu_music_immediate(r::ecs::Query<r::ecs::Mut<r::AudioSink>, r::ecs::With<MenuMusicTag>> query)
{
    for (auto [sink, _] : query) {
        r::Logger::info("stop_menu_music_immediate: stopping menu music, was_playing=" + std::to_string(sink.ptr->is_playing()));
        sink.ptr->stop();
        r::Logger::info("stop_menu_music_immediate: stopped, is_playing=" + std::to_string(sink.ptr->is_playing()));
    }
}

/* Resume the menu music (play) */
static void resume_menu_music_system(r::ecs::Commands &commands, r::ecs::ResMut<r::AudioManager> audio,
    r::ecs::ResMut<MenuMusicResource> menu_music, r::ecs::Query<r::ecs::Mut<r::AudioSink>, r::ecs::With<MenuMusicTag>> query)
{
    r::Logger::info("resume_menu_music_system: invoked");

    /* If we don't have a loaded handle, load it now */
    if (menu_music.ptr->handle == r::AudioInvalidHandle) {
        const std::string &path = r::path::get("assets/sounds/title.mp3");
        const auto handle = audio.ptr->load(path);
        if (handle == r::AudioInvalidHandle) {
            r::Logger::warn("resume_menu_music_system: failed to load " + path);
            return;
        }
        menu_music.ptr->handle = handle;
        r::Logger::info(std::string{"resume_menu_music_system: loaded menu music handle="} + std::to_string(handle));
    }

    /* If there's no spawned entity, spawn one (it will start playing by default) */
    if (menu_music.ptr->entity == r::ecs::NULL_ENTITY) {
        const auto ent = commands.spawn(MenuMusicTag{}, r::AudioPlayer{menu_music.ptr->handle}, r::AudioSink{}).id();
        menu_music.ptr->entity = ent;
        r::Logger::info(std::string{"resume_menu_music_system: spawned menu music entity="} + std::to_string(ent));
        return; /* newly spawned entity will play automatically */
    }

    /* Otherwise, ensure existing sinks are playing */
    for (auto [sink, _] : query) {
        r::Logger::info("resume_menu_music_system: resuming existing sink, was_playing=" + std::to_string(sink.ptr->is_playing()));
        sink.ptr->play();
        r::Logger::info("resume_menu_music_system: resumed, is_playing=" + std::to_string(sink.ptr->is_playing()));
    }
}

/* Ensure menu music exists and is playing while we're in the Main Menu. This runs every frame
   while in MainMenu and will (re)spawn and play the title music if needed. */
[[maybe_unused]] static void ensure_menu_music_active_system(r::ecs::Commands &commands, r::ecs::ResMut<MenuMusicResource> menu_music,
    r::ecs::ResMut<r::AudioManager> audio, r::ecs::Res<r::State<GameState>> current_state,
    r::ecs::Query<r::ecs::Mut<r::AudioSink>, r::ecs::With<MenuMusicTag>> sink_query)
{
    if (current_state.ptr->current() != GameState::MainMenu)
        return;

    /* Load handle if missing */
    if (menu_music.ptr->handle == r::AudioInvalidHandle) {
        const std::string &path = r::path::get("assets/sounds/title.mp3");
        const auto handle = audio.ptr->load(path);
        if (handle == r::AudioInvalidHandle) {
            r::Logger::warn("ensure_menu_music_active_system: failed to load " + path);
            return;
        }
        menu_music.ptr->handle = handle;
        r::Logger::info(std::string{"ensure_menu_music_active_system: loaded menu music handle="} + std::to_string(handle));
    }

    /* Spawn entity if missing */
    if (menu_music.ptr->entity == r::ecs::NULL_ENTITY) {
        const auto ent = commands.spawn(MenuMusicTag{}, r::AudioPlayer{menu_music.ptr->handle}, r::AudioSink{}).id();
        menu_music.ptr->entity = ent;
        r::Logger::info(std::string{"ensure_menu_music_active_system: spawned menu music entity="} + std::to_string(ent));
        return; /* newly spawned will play automatically */
    }

    /* Ensure sinks are playing */
    for (auto [sink, _] : sink_query) {
        if (!sink.ptr->is_playing()) {
            sink.ptr->play();
        }
    }
}

/* Force reload + play title when entering MainMenu (despawn/unload any previous instances first). */
[[maybe_unused]] static void play_title_on_enter_mainmenu(r::ecs::Commands &commands, r::ecs::ResMut<MenuMusicResource> menu_music,
    r::ecs::ResMut<r::AudioManager> audio, r::ecs::Query<r::ecs::With<MenuMusicTag>> query)
{
    /* Despawn existing entities */
    for (auto it = query.begin(); it != query.end(); ++it) {
        commands.despawn(it.entity());
        if (menu_music.ptr->entity == it.entity())
            menu_music.ptr->entity = r::ecs::NULL_ENTITY;
    }

    /* Unload previous handle if any */
    if (menu_music.ptr->handle != r::AudioInvalidHandle) {
        audio.ptr->unload(menu_music.ptr->handle);
        menu_music.ptr->handle = r::AudioInvalidHandle;
    }

    /* Load and spawn fresh */
    const std::string &path = r::path::get("assets/sounds/title.mp3");
    const auto handle = audio.ptr->load(path);
    if (handle == r::AudioInvalidHandle) {
        r::Logger::warn("play_title_on_enter_mainmenu: failed to load " + path);
        return;
    }
    menu_music.ptr->handle = handle;
    const auto ent = commands.spawn(MenuMusicTag{}, r::AudioPlayer{handle}, r::AudioSink{}).id();
    menu_music.ptr->entity = ent;
    r::Logger::info(std::string{"play_title_on_enter_mainmenu: spawned menu music entity="} + std::to_string(ent));
}

/* Previously we unloaded/despawned menu music on game enter; keep it loaded and simply pause the sink
   to avoid reload/unload races. The pause system handles stopping audio when entering gameplay. */

/* Safety system: every update, if we're not in MainMenu, ensure no MenuMusicTag exists. This
   handles ordering issues where other OnEnter systems might spawn the menu music unexpectedly. */
/* No-op safety left out; we'll control pause/resume explicitly on state transitions. */

static void game_over_system(r::ecs::Res<r::UserInput> user_input, r::ecs::ResMut<r::NextState<GameState>> next_state)
{
    if (user_input.ptr->isKeyPressed(KEY_ESCAPE)) {
        r::Application::quit.store(true, std::memory_order_relaxed);
    }

    if (user_input.ptr->isKeyPressed(KEY_ENTER)) {
        r::Logger::info("Restarting game...");
        next_state.ptr->set(GameState::EnemiesBattle);
    }
}

static void show_you_win_ui(r::ecs::Commands &cmds, r::ecs::Res<PlayerScore> score)
{
    cmds.spawn(YouWinRoot{}, r::UiNode{},
            r::Style{
                .width_pct = 100.f,
                .height_pct = 100.f,
                .background = r::Color{0, 20, 50, 180}, /* Semi-transparent blue background */
                .direction = r::LayoutDirection::Column,
                .justify = r::JustifyContent::Center,
                .align = r::AlignItems::Center,
                .gap = 20.f,
            },
            r::ComputedLayout{}, r::Visibility::Visible)
        .with_children([&](r::ecs::ChildBuilder &parent) {
            parent.spawn(r::UiNode{},
                r::UiText{
                    .content = "YOU WIN!",
                    .font_size = 80,
                    .color = {98, 221, 255, 255},
                    .font_path = {},
                },
                r::Style{
                    .height = 90.f,
                },
                r::ComputedLayout{}, r::Visibility::Visible);
            parent.spawn(r::UiNode{},
                r::UiText{
                    .content = "Final Score: " + std::to_string(score.ptr->value),
                    .font_size = 30,
                    .color = {200, 200, 200, 255},
                    .font_path = {},
                },
                r::Style{.height = 40.f}, r::ComputedLayout{}, r::Visibility::Visible);
            parent.spawn(r::UiNode{},
                r::UiText{
                    .content = "Press ENTER to return to Main Menu",
                    .font_size = 30,
                    .color = {200, 200, 200, 255},
                    .font_path = {},
                },
                r::Style{.height = 40.f}, r::ComputedLayout{}, r::Visibility::Visible);
        });
}

static void cleanup_you_win_ui(r::ecs::Commands &cmds, r::ecs::Query<r::ecs::With<YouWinRoot>> query)
{
    for (auto it = query.begin(); it != query.end(); ++it) {
        cmds.despawn(it.entity());
    }
}

static void you_win_system(r::ecs::Res<r::UserInput> user_input, r::ecs::ResMut<r::NextState<GameState>> next_state)
{
    if (user_input.ptr->isKeyPressed(KEY_ENTER)) {
        next_state.ptr->set(GameState::MainMenu);
    }
}

static void camera_follow_player_system(r::ecs::ResMut<r::Camera3d> camera,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::With<Player>> player_query)
{
    if (player_query.size() == 0) {
        return;
    }

    auto [player_transform, _] = *player_query.begin();

    camera.ptr->position.x = player_transform.ptr->position.x;
    camera.ptr->target.x = player_transform.ptr->position.x;
}

void MenuPlugin::build(r::Application &app)
{
    app
        /* Global UI setup */
        .insert_resource(PreviousGameState{})
        .insert_resource(MenuMusicResource{})
        .add_systems<setup_ui_theme>(r::Schedule::STARTUP)

    /* Main Menu State */
    .add_systems<build_main_menu>(r::OnEnter{GameState::MainMenu})
    /* Startup menu music once at plugin start */
    .add_systems<menu_music_startup>(r::Schedule::STARTUP)
    /* Play/resume title theme when entering main menu */
    .add_systems<resume_menu_music_system>(r::OnEnter{GameState::MainMenu})
    /* Pause title theme when leaving main menu (entering other states) */
    /* Ensure title music is stopped immediately when entering gameplay states */
    .add_systems<stop_menu_music_immediate>(r::OnEnter{GameState::EnemiesBattle})
    .add_systems<stop_menu_music_immediate>(r::OnEnter{GameState::BossBattle})
    /* Also keep pause handler for other transitions */
    .add_systems<pause_menu_music_system>(r::OnEnter{GameState::Paused})
    .add_systems<pause_menu_music_system>(r::OnEnter{GameState::SettingsMenu})
    .add_systems<pause_menu_music_system>(r::OnEnter{GameState::GameOver})
    .add_systems<pause_menu_music_system>(r::OnEnter{GameState::YouWin})
    .add_systems<pause_menu_music_system>(r::OnEnter{GameState::Paused})
    .add_systems<pause_menu_music_system>(r::OnEnter{GameState::SettingsMenu})
    .add_systems<pause_menu_music_system>(r::OnEnter{GameState::GameOver})
    .add_systems<pause_menu_music_system>(r::OnEnter{GameState::YouWin})
    .add_systems<menu_button_handler>(r::Schedule::UPDATE)
    .run_if<r::run_conditions::on_event<r::UiClick>>()
    /* Play/resume title theme when entering main menu */
    /* Keep menu music entity alive across state transitions; pause/resume will control playback. */
    .add_systems<cleanup_menu>(r::OnExit{GameState::MainMenu})
    /* No per-frame ensure; rely on startup + explicit resume/pause on transitions */

        /* In-Game HUD */
        .add_systems<cleanup_game_hud>(r::OnEnter{GameState::EnemiesBattle})
        .add_systems<cleanup_game_hud>(r::OnEnter{GameState::BossBattle})
        .add_systems<build_game_hud>(r::OnEnter{GameState::EnemiesBattle})
        .add_systems<build_game_hud>(r::OnEnter{GameState::BossBattle})
        .add_systems<update_game_hud>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()
        .run_or<r::run_conditions::in_state<GameState::BossBattle>>()
        .add_systems<cleanup_game_hud>(r::OnExit{GameState::EnemiesBattle})
        .add_systems<cleanup_game_hud>(r::OnExit{GameState::BossBattle})

        /* GameOver State */
        .add_systems<show_game_over_ui>(r::OnEnter{GameState::GameOver})
        .add_systems<cleanup_game_over_ui>(r::OnExit{GameState::GameOver})
        .add_systems<game_over_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::GameOver>>()

        /* YouWin State (New) */
        .add_systems<show_you_win_ui>(r::OnEnter{GameState::YouWin})
        .add_systems<cleanup_you_win_ui>(r::OnExit{GameState::YouWin})
        .add_systems<you_win_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::YouWin>>()

        .add_systems<camera_follow_player_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::MainMenu>>();
}
