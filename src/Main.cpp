#include <plugins/combat.hpp>
#include <plugins/debug.hpp>
#include <plugins/enemy.hpp>
#include <plugins/force.hpp>
#include <plugins/game_state.hpp>
#include <plugins/gameplay.hpp>
#include <plugins/map.hpp>
#include <plugins/menu.hpp>
#include <plugins/ui_sfx.hpp>
#include <plugins/pause.hpp>
#include <plugins/player.hpp>
#include <plugins/rtype_protocol_plugin.hpp>
#include <plugins/settings.hpp>

#include <events/debug.hpp>
#include <events/game_events.hpp>
#include <resources/game_mode.hpp>
#include <resources/level.hpp>
#include <state/game_state.hpp>

#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Backend.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/Plugins/DefaultPlugins.hpp>
#include <R-Engine/Plugins/InputPlugin.hpp>
#include <R-Engine/Plugins/NetworkPlugin.hpp>
#include <R-Engine/Plugins/PostProcessingPlugin.hpp>
#include <R-Engine/Plugins/RenderPlugin.hpp>
#include <R-Engine/Plugins/WindowPlugin.hpp>

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>

/**
 * @brief Disables the default ESC key behavior for closing the window.
 * @details This allows the game to handle ESC for pausing without quitting.
 */
static void disable_escape_key_system()
{
    SetExitKey(KEY_NULL);
}

/**
 * @brief (STARTUP) Sets up the game world.
 * @details This system runs once when entering the game. It configures the camera
 * and binds input actions.
 */
static void setup_core_game_system(r::ecs::ResMut<r::Camera3d> camera, r::ecs::ResMut<r::InputMap> input_map)
{
    /* --- Configure Camera --- */
    /* Position the camera for a side-scrolling view on the XY plane (Z is depth). */
    camera.ptr->position = {0.0f, 0.0f, 20.0f};
    camera.ptr->target = {0.0f, 0.0f, 0.0f};
    camera.ptr->up = {0.0f, 1.0f, 0.0f};
    camera.ptr->fovy = 45.0f;

    /* --- Bind Inputs --- */
    input_map.ptr->bindAction("MoveUp", r::KEYBOARD, KEY_W);
    input_map.ptr->bindAction("MoveDown", r::KEYBOARD, KEY_S);
    input_map.ptr->bindAction("MoveLeft", r::KEYBOARD, KEY_A);
    input_map.ptr->bindAction("MoveRight", r::KEYBOARD, KEY_D);
    input_map.ptr->bindAction("Fire", r::KEYBOARD, KEY_SPACE);
    input_map.ptr->bindAction("Force", r::KEYBOARD, KEY_LEFT_SHIFT);
    input_map.ptr->bindAction("Pause", r::KEYBOARD, KEY_ESCAPE);

    /* Gamepad Bindings */
    input_map.ptr->bindAction("Fire", r::GAMEPAD, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);  ///< 'A' on Xbox, 'X' on PS
    input_map.ptr->bindAction("Force", r::GAMEPAD, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);///< 'B' on Xbox, 'Circle' on PS
    input_map.ptr->bindAction("Pause", r::GAMEPAD, GAMEPAD_BUTTON_MIDDLE_RIGHT);    ///< 'Start' button
}

static void setup_levels_system(r::ecs::Commands &commands)
{
    GameLevels game_levels;
    game_levels.levels = {
        {
            .id = 1,
            .enemy_spawn_interval = 0.75f,
            .boss_spawn_time = 10.0f,
            .background_texture_path = "assets/textures/background.png",
            .scenery_model_path = "assets/models/BlackBuilding.glb",
            .enemy_types =
                {
                    {
                        "assets/models/enemy.glb",
                        1,
                        2.0f,
                        EnemyBehaviorType::Straight,
                        100,
                    },
                    {
                        "assets/models/enemy.glb",
                        2,
                        1.5f,
                        EnemyBehaviorType::Straight,
                        150,
                    },
                },
            .boss_data =
                {
                    .model_path = "assets/models/Boss.glb",
                    .max_health = 500,
                    .behavior = BossBehaviorType::VerticalPatrol,
                    .score_value = 5000,
                },
        },
        {
            .id = 2,
            .enemy_spawn_interval = 0.5f,
            .boss_spawn_time = 15.0f,
            .background_texture_path = "assets/textures/background_level2.png",
            .scenery_model_path = "assets/models/Asteroid.glb",
            .enemy_types =
                {
                    {
                        "assets/models/enemy_2.glb",
                        2,
                        3.0f,
                        EnemyBehaviorType::SineWave,
                        200,
                    },
                },
            .boss_data =
                {
                    .model_path = "assets/models/boss_2.glb",
                    .max_health = 750,
                    .behavior = BossBehaviorType::HomingAttack,
                    .score_value = 7500,
                },
        },
        {
            .id = 3,
            .enemy_spawn_interval = 0.3f,
            .boss_spawn_time = 20.0f,
            .background_texture_path = "assets/textures/background_level3.png",
            .scenery_model_path = "assets/models/FortressWall.glb",
            .enemy_types =
                {
                    {
                        "assets/models/enemy.glb",
                        3,
                        2.0f,
                        EnemyBehaviorType::Homing,
                        300,
                    },
                    {
                        "assets/models/enemy.glb",
                        1,
                        4.0f,
                        EnemyBehaviorType::Straight,
                        100,
                    },
                },
            .boss_data =
                {
                    .model_path = "assets/models/Boss.glb",
                    .max_health = 1000,
                    .behavior = BossBehaviorType::Turret,
                    .score_value = 10000,
                },
        },
    };
    commands.insert_resource(game_levels);
    commands.insert_resource(CurrentLevel{0}); /* Start at level 0 */
}

/**
 * @brief (STARTUP) Loads network settings from `network.cfg`.
 * @details If the file doesn't exist, it creates it with default values.
 * This allows players to configure the server address and port externally.
 */
static void load_network_config_system(r::ecs::Commands &commands)
{
    NetworkConfig config;
    const std::string filename = "network.cfg";
    std::ifstream config_file(filename);

    if (config_file.is_open()) {
        std::string line;
        while (std::getline(config_file, line)) {
            std::string key;
            std::string value;
            size_t separator_pos = line.find('=');
            if (separator_pos != std::string::npos) {
                key = line.substr(0, separator_pos);
                value = line.substr(separator_pos + 1);

                if (key == "address") {
                    config.server_address = value;
                } else if (key == "port") {
                    try {
                        config.server_port = static_cast<unsigned short>(std::stoi(value));
                    } catch (...) {
                        /* Keep default port if parsing fails */
                    }
                }
            }
        }
        r::Logger::info("Loaded network config from " + filename);
    } else {
        // File doesn't exist, create it with defaults
        std::ofstream new_config_file(filename);
        if (new_config_file.is_open()) {
            new_config_file << "address=" << config.server_address << std::endl;
            new_config_file << "port=" << config.server_port << std::endl;
            r::Logger::info(filename + " not found. Created with default settings.");
        }
    }

    commands.insert_resource(config);
}

int main()
{
    /* Seed random for enemy spawn positions */
    srand(static_cast<unsigned int>(time(nullptr)));

    r::Application{}
        .add_plugins(r::DefaultPlugins{}
                .set(r::WindowPlugin{r::WindowPluginConfig{
                    .size = {1280, 720},
                    .title = "R-Type",
                    .cursor = r::WindowCursorState::Visible,
                }})
                .set(r::PostProcessingPlugin{r::PostProcessingPluginConfig{
                    .engine_assets_prefix = "external/R-Engine/assets/",
                }}))

        /* Register all custom game events */
        .add_events<PlayerDiedEvent, BossTimeReachedEvent, BossDefeatedEvent, EntityDiedEvent, DebugSwitchLevelEvent>()

        /* Insert game-wide resources */
        .insert_resource(GameMode::Offline)

        /* Add network plugins first */
        .add_plugins(r::net::NetworkPlugin{})
        .add_plugins(rtype::protocol::RTypeProtocolPlugin{})

        /* Add all our custom game plugins */
        .add_plugins(GameStatePlugin{})
        .add_plugins(MenuPlugin{})
    .add_plugins(UiSfxPlugin{})
        .add_plugins(PausePlugin{})
        .add_plugins(SettingsPlugin{})
        .add_plugins(MapPlugin{})
        .add_plugins(PlayerPlugin{})
        .add_plugins(ForcePlugin{})
        .add_plugins(EnemyPlugin{})
        .add_plugins(GameplayPlugin{})
        .add_plugins(CombatPlugin{})
        .add_plugins(DebugPlugin{})

        /* Add the remaining core setup */
        .add_systems<disable_escape_key_system>(r::Schedule::STARTUP)
        .add_systems<load_network_config_system>(r::Schedule::STARTUP)
        .add_systems<setup_core_game_system>(r::Schedule::STARTUP)
        .add_systems<setup_levels_system>(r::Schedule::STARTUP)

        .run();

    return 0;
}
