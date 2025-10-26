#include <components.hpp>
#include <events.hpp>
#include <plugins/gameplay.hpp>
#include <resources.hpp>
#include <state.hpp>

#include "R-Engine/Components/Transform3d.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/FrameTime.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Event.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Plugins/MeshPlugin.hpp>
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>

/* ================================================================================= */
/* Constants */
/* ================================================================================= */

static constexpr float BULLET_SPEED = 8.0f;
static constexpr float BOSS_VERTICAL_SPEED = 4.5f;
static constexpr float BOSS_UPPER_BOUND = 4.0f;
static constexpr float BOSS_LOWER_BOUND = -15.0f;

/* ================================================================================= */
/* Gameplay Systems */
/* ================================================================================= */

static void setup_level_timers_system(r::ecs::Res<CurrentLevel> current_level, r::ecs::Res<GameLevels> game_levels,
    r::ecs::ResMut<EnemySpawnTimer> enemy_timer, r::ecs::ResMut<BossSpawnTimer> boss_timer)
{
    const auto &level_data = game_levels.ptr->levels[static_cast<size_t>(current_level.ptr->index)];
    enemy_timer.ptr->time_left = level_data.enemy_spawn_interval;
    boss_timer.ptr->time_left = level_data.boss_spawn_time;
    boss_timer.ptr->spawned = false;
    r::Logger::info("Setting up timers for level " + std::to_string(level_data.id));
}

static void enemy_spawner_system(r::ecs::Commands &commands, r::ecs::ResMut<EnemySpawnTimer> spawn_timer,
    r::ecs::Res<r::core::FrameTime> time, r::ecs::ResMut<r::Meshes> meshes, r::ecs::Res<CurrentLevel> current_level,
    r::ecs::Res<GameLevels> game_levels)
{
    spawn_timer.ptr->time_left -= time.ptr->delta_time;
    if (spawn_timer.ptr->time_left <= 0.0f) {
        const auto &level_data = game_levels.ptr->levels[static_cast<size_t>(current_level.ptr->index)];
        spawn_timer.ptr->time_left = level_data.enemy_spawn_interval;

        if (level_data.enemy_types.empty()) {
            r::Logger::warn("No enemy types defined for the current level!");
            return;
        }

        /* Pick a random enemy type from the current level's list */
        const auto &enemy_to_spawn = level_data.enemy_types[static_cast<size_t>(rand()) % level_data.enemy_types.size()];

        float random_y = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 10.0f - 5.0f;

        r::MeshHandle enemy_mesh_handle = meshes.ptr->add(enemy_to_spawn.model_path);
        if (enemy_mesh_handle != r::MeshInvalidHandle) {
            auto enemy_cmds = commands.spawn(Enemy{}, Health{enemy_to_spawn.health, enemy_to_spawn.health},
                r::Transform3d{
                    .position = {15.0f, random_y, 0.0f},
                    .scale = {1.0f, 1.0f, 1.0f},
                },
                Velocity{
                    {-enemy_to_spawn.speed, 0.0f, 0.0f},
                },
                Collider{0.5f},
                r::Mesh3d{
                    .id = enemy_mesh_handle,
                    .color = r::Color{255, 255, 255, 255},
                    .rotation_offset = {0.0f, -(static_cast<float>(M_PI) / 2.0f), 0.0f},
                });

            /* Add the correct behavior component based on level data */
            switch (enemy_to_spawn.behavior) {
                case EnemyBehaviorType::Straight:
                    /* Default behavior, no component needed */
                    break;
                case EnemyBehaviorType::SineWave:
                    enemy_cmds.insert(SineWaveEnemy{});
                    break;
                case EnemyBehaviorType::Homing:
                    enemy_cmds.insert(HomingEnemy{});
                    break;
                default:
                    /* Safely do nothing for unhandled cases */
                    break;
            }

        } else {
            r::Logger::error("Failed to queue enemy model for loading: " + enemy_to_spawn.model_path);
        }
    }
}

static void setup_boss_fight_system(r::ecs::EventWriter<BossTimeReachedEvent> writer, r::ecs::Res<r::core::FrameTime> time,
    r::ecs::ResMut<BossSpawnTimer> spawn_timer)
{
    if (spawn_timer.ptr->spawned) {
        return;
    }
    spawn_timer.ptr->time_left -= time.ptr->delta_time;
    if (spawn_timer.ptr->time_left <= 0.0f) {
        spawn_timer.ptr->spawned = true;
        writer.send({});
    }
}

static void boss_spawn_system(r::ecs::Commands &commands, r::ecs::ResMut<r::Meshes> meshes, r::ecs::Res<CurrentLevel> current_level,
    r::ecs::Res<GameLevels> game_levels)
{
    const auto &level_data = game_levels.ptr->levels[static_cast<size_t>(current_level.ptr->index)];
    const auto &boss_data = level_data.boss_data;

    r::Logger::info("Spawning boss for Level " + std::to_string(current_level.ptr->index + 1));

    r::MeshHandle boss_mesh_handle = meshes.ptr->add(boss_data.model_path);
    if (boss_mesh_handle != r::MeshInvalidHandle) {
        auto boss_cmds = commands.spawn(Boss{}, BossShootTimer{},
            Health{
                boss_data.max_health,
                boss_data.max_health,
            },
            r::Transform3d{
                .position = {12.0f, -10.0f, 0.0f},
                .scale = {0.5f, 0.5f, 0.5f},
            },
            Velocity{
                {0.0f, BOSS_VERTICAL_SPEED, 0.0f},
            },
            Collider{
                .radius = 5.5f,
                .offset = {-2.5f, 4.0f, 0.0f},
            },
            r::Mesh3d{
                .id = boss_mesh_handle,
                .color = r::Color{255, 255, 255, 255},
                .rotation_offset = {0.0f, -(static_cast<float>(M_PI) / 2.0f), 0.0f},
            });

        /* Add the correct behavior component based on level data */
        switch (boss_data.behavior) {
            case BossBehaviorType::VerticalPatrol:
                boss_cmds.insert(VerticalPatrolBoss{});
                break;
            case BossBehaviorType::HomingAttack:
                boss_cmds.insert(HomingAttackBoss{});
                break;
            case BossBehaviorType::Turret:
                boss_cmds.insert(TurretBoss{});
                break;
            default:
                r::Logger::warn("Unknown or unsupported boss behavior type, defaulting to VerticalPatrol.");
                boss_cmds.insert(VerticalPatrolBoss{});
                break;
        }

    } else {
        r::Logger::error("Failed to queue boss model for loading: " + boss_data.model_path);
    }
}

/* ================================================================================= */
/* Enemy Behavior Systems */
/* ================================================================================= */

static void enemy_movement_sine_wave_system(r::ecs::Res<r::core::FrameTime> time,
    r::ecs::Query<r::ecs::Mut<Velocity>, r::ecs::Mut<SineWaveEnemy>> query)
{
    for (auto [velocity, sine_wave] : query) {
        /* Update the angle for the sine calculation */
        sine_wave.ptr->angle += sine_wave.ptr->frequency * time.ptr->delta_time;

        /* The horizontal speed is constant (set at spawn), we only modify the vertical speed */
        float base_horizontal_speed = velocity.ptr->value.x;
        velocity.ptr->value.y = std::sin(sine_wave.ptr->angle) * sine_wave.ptr->amplitude;

        /* Ensure horizontal speed is maintained */
        velocity.ptr->value.x = base_horizontal_speed;
    }
}

static void enemy_movement_homing_system(r::ecs::Res<r::core::FrameTime> time,
    r::ecs::Query<r::ecs::Mut<Velocity>, r::ecs::Ref<r::Transform3d>, r::ecs::Ref<HomingEnemy>> enemy_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::With<Player>> player_query)
{
    if (player_query.size() == 0) {
        return; /* No player to home in on */
    }
    auto [player_transform, _p] = *player_query.begin();

    for (auto [velocity, enemy_transform, homing] : enemy_query) {
        /* Calculate direction towards the player */
        r::Vec3f direction_to_player = (player_transform.ptr->position - enemy_transform.ptr->position);
        if (direction_to_player.length_sq() > 0) {
            direction_to_player = direction_to_player.normalize();
        }

        /* Get the current velocity's direction and speed */
        float current_speed = velocity.ptr->value.length();
        r::Vec3f current_direction = {0, 0, 0};
        if (current_speed > 0) {
            current_direction = velocity.ptr->value / current_speed;
        }

        /* Interpolate towards the target direction to create a turning effect */
        r::Vec3f new_direction = current_direction.lerp(direction_to_player, time.ptr->delta_time * homing.ptr->turn_speed);
        if (new_direction.length_sq() > 0) {
            new_direction = new_direction.normalize();
        }

        /* Apply the new direction, maintaining the original speed */
        velocity.ptr->value = new_direction * current_speed;
    }
}

/* ================================================================================= */
/* Boss Behavior Systems */
/* ================================================================================= */

static void boss_movement_vertical_patrol_system(
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Mut<Velocity>, r::ecs::With<VerticalPatrolBoss>> query)
{
    for (auto [transform, velocity, _] : query) {
        if (transform.ptr->position.y > BOSS_UPPER_BOUND && velocity.ptr->value.y > 0) {
            velocity.ptr->value.y *= -1;
        } else if (transform.ptr->position.y < BOSS_LOWER_BOUND && velocity.ptr->value.y < 0) {
            velocity.ptr->value.y *= -1;
        }
    }
}

static void boss_shooting_vertical_patrol_system(r::ecs::Commands &commands, r::ecs::Res<r::core::FrameTime> time,
    r::ecs::Res<BossBulletAssets> bullet_assets,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Mut<BossShootTimer>, r::ecs::Ref<Health>, r::ecs::With<VerticalPatrolBoss>> query)
{
    for (auto [transform, timer, health, _] : query) {
        timer.ptr->time_left -= time.ptr->delta_time;

        if (timer.ptr->time_left <= 0.0f) {
            timer.ptr->time_left = BossShootTimer::FIRE_RATE;

            commands.spawn(EnemyBullet{},
                r::Transform3d{
                    .position = transform.ptr->position - r::Vec3f{1.6f, 0.0f, 0.0f},
                    .rotation = {-(static_cast<float>(M_PI) / 2.0f), 0.0f, static_cast<float>(M_PI) / 2.0f},
                    .scale = {1.0f, 1.0f, 1.0f},
                },
                Velocity{
                    {-BULLET_SPEED, 0.0f, 0.0f},
                },
                Collider{
                    .radius = 0.4f,
                    .offset = {-1.0f, 0.0f, 0.0f},
                },
                r::Mesh3d{
                    .id = bullet_assets.ptr->small_missile,
                    .color = r::Color{255, 255, 255, 255},
                    .rotation_offset = {-(static_cast<float>(M_PI) / 2.0f), 0.0f, -static_cast<float>(M_PI) / 2.0f},
                });
            if (health.ptr->current <= health.ptr->max / 2) {
                commands.spawn(EnemyBullet{},
                    r::Transform3d{
                        .position = transform.ptr->position + r::Vec3f{0.0f, 5.5f, 0.0f},
                        .rotation = {-(static_cast<float>(M_PI) / 2.0f), 0.0f, static_cast<float>(M_PI) / 2.0f},
                        .scale = {0.5f, 0.5f, 0.5f},
                    },
                    Velocity{
                        {-BULLET_SPEED, 0.0f, 0.0f},
                    },
                    Collider{
                        .radius = 0.8f,
                    },
                    r::Mesh3d{
                        .id = bullet_assets.ptr->big_missile,
                        .color = r::Color{255, 255, 255, 255},
                        .rotation_offset = {-(static_cast<float>(M_PI) / 2.0f), 0.0f, -static_cast<float>(M_PI) / 2.0f},
                    });
            }
        }
    }
}

static void boss_movement_homing_system([[maybe_unused]] r::ecs::Query<r::ecs::With<HomingAttackBoss>> query)
{
    /* TODO: Implement homing movement logic for the boss */
}

static void boss_movement_turret_system([[maybe_unused]] r::ecs::Query<r::ecs::With<TurretBoss>> query)
{
    /* TODO: Implement turret movement logic for the boss (for example, stationary but rotates) */
}

/* ================================================================================= */

static void movement_system(r::ecs::Res<r::core::FrameTime> time, r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::Ref<Velocity>> query)
{
    for (auto [transform, velocity] : query) {
        transform.ptr->position = transform.ptr->position + velocity.ptr->value * time.ptr->delta_time;
    }
}

static void setup_missile_assets_system(r::ecs::Commands &commands, r::ecs::ResMut<r::Meshes> meshes)
{
    BossBulletAssets bullet_assets;

    bullet_assets.big_missile = meshes.ptr->add("assets/models/BigMissiles.glb");
    if (bullet_assets.big_missile == r::MeshInvalidHandle) {
        r::Logger::error("Failed to queue big missile model !");
    }

    bullet_assets.small_missile = meshes.ptr->add("assets/models/BossRegularMissile.glb");
    if (bullet_assets.small_missile == r::MeshInvalidHandle) {
        r::Logger::error("Failed to queue regular boss missile model !");
    }

    commands.insert_resource(bullet_assets);
}

void GameplayPlugin::build(r::Application &app)
{
    app.insert_resource(EnemySpawnTimer{})
        .insert_resource(BossSpawnTimer{})

        .add_systems<movement_system>(r::Schedule::UPDATE)
        .add_systems<setup_missile_assets_system>(r::OnEnter{GameState::EnemiesBattle})

        .add_systems<setup_level_timers_system>(r::OnEnter{GameState::EnemiesBattle})

        .add_systems<setup_boss_fight_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()

        .add_systems<enemy_spawner_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()

        .add_systems<enemy_movement_sine_wave_system, enemy_movement_homing_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()

        .add_systems<boss_spawn_system>(r::OnEnter(GameState::BossBattle))

        .add_systems<boss_movement_vertical_patrol_system, boss_shooting_vertical_patrol_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::BossBattle>>()

        .add_systems<boss_movement_homing_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::BossBattle>>()

        .add_systems<boss_movement_turret_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::BossBattle>>();
}
