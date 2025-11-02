#include <plugins/enemy.hpp>

#include "R-Engine/Components/Transform3d.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/FrameTime.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Plugins/MeshPlugin.hpp>
#include <cmath>
#include <cstdlib>

#include <components/common.hpp>
#include <components/enemy.hpp>
#include <components/player.hpp>
#include <components/projectiles.hpp>
#include <resources/assets.hpp>
#include <resources/level.hpp>
#include <state/game_state.hpp>
#include <state/run_conditions.hpp>

/* ================================================================================= */
/* Constants */
/* ================================================================================= */

static constexpr float BULLET_SPEED = 8.0f;
static constexpr float HOMING_MISSILE_SPEED = 5.5f;
static constexpr float BOSS_VERTICAL_SPEED = 4.5f;
static constexpr float BOSS_HOMING_MOVE_SPEED = 3.0f;
static constexpr float BOSS_UPPER_BOUND = 4.0f;
static constexpr float BOSS_LOWER_BOUND = -15.0f;

/* ================================================================================= */
/* Enemy Spawning */
/* ================================================================================= */

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
            auto enemy_cmds =
                commands.spawn(Enemy{}, Health{enemy_to_spawn.health, enemy_to_spawn.health}, ScoreValue{enemy_to_spawn.score_value},
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

static void boss_spawn_system(r::ecs::Commands &commands, r::ecs::ResMut<r::Meshes> meshes, r::ecs::Res<CurrentLevel> current_level,
    r::ecs::Res<GameLevels> game_levels)
{
    const auto &level_data = game_levels.ptr->levels[static_cast<size_t>(current_level.ptr->index)];
    const auto &boss_data = level_data.boss_data;

    r::Logger::info("Spawning boss for Level " + std::to_string(current_level.ptr->index + 1));

    r::MeshHandle boss_mesh_handle = meshes.ptr->add(boss_data.model_path);
    if (boss_mesh_handle != r::MeshInvalidHandle) {
        /* Prepare component variables that differ between boss types */
        r::Transform3d initial_transform;
        Velocity initial_velocity;
        Collider initial_collider;

        /* Set values based on the boss's behavior type */
        switch (boss_data.behavior) {
            case BossBehaviorType::VerticalPatrol:
                initial_transform = {
                    .position = {12.0f, -10.0f, 0.0f},
                    .scale = {0.5f, 0.5f, 0.5f},
                };
                initial_velocity = {
                    {0.0f, BOSS_VERTICAL_SPEED, 0.0f},
                };
                initial_collider = {
                    .radius = 5.5f,
                    .offset = {-2.5f, 4.0f, 0.0f},
                };
                break;
            case BossBehaviorType::HomingAttack:
            default: /* Default to HomingAttack behavior if unknown */
                initial_transform = {
                    .position = {20.0f, 0.0f, 0.0f},
                    .scale = {0.4f, 0.4f, 0.4f},
                };
                initial_velocity = {
                    {-BOSS_HOMING_MOVE_SPEED, 0.0f, 0.0f},
                };
                initial_collider = {
                    .radius = 2.0f,
                    .offset = {0.0f, 0.0f, 0.0f},
                };
                break;
        }

        /* Spawn the boss with the right components */
        auto boss_cmds = commands.spawn(Boss{}, BossShootTimer{}, ScoreValue{boss_data.score_value},
            Health{
                boss_data.max_health,
                boss_data.max_health,
            },
            initial_transform, initial_velocity, initial_collider,
            r::Mesh3d{
                .id = boss_mesh_handle,
                .color = r::Color{255, 255, 255, 255},
                .rotation_offset = {0.0f, -(static_cast<float>(M_PI) / 2.0f), 0.0f},
            });

        /* If this is Level 2 (index == 1), spawn the shield as a small, destructible unit in front of the boss */
        if (current_level.ptr->index == 1) {
        r::MeshHandle shield_handle = meshes.ptr->add("assets/models/Shield.glb");
            if (shield_handle != r::MeshInvalidHandle) {
                /* Spawn as a child so it follows the boss, but place it in front and much smaller.
                   Make it an Enemy with its own Health/Collider so the player must destroy it first. */
                boss_cmds.with_children([&](r::ecs::ChildBuilder &child) {
                    /* Center/front shield */
                    child.spawn(
                        Enemy{}, Shield{}, /* treat shield as an enemy unit to be targetable */
                        Health{350, 350},
                        ScoreValue{100},
                        r::Transform3d{
                            /* place the shield a bit in front of the boss (local space) */
                            .position = {-20.0f, 0.0f, 0.0f},
                            .scale = {3.0f, 3.0f, 3.0f},
                        },
                        Collider{.radius = 1.1f},
                        r::Mesh3d{.id = shield_handle, .color = r::Color{255, 255, 255, 255}, .rotation_offset = {0.0f, -(static_cast<float>(M_PI) / 2.0f), 0.0f}});

                    /* Top/front shield */
                    child.spawn(
                        Enemy{}, Shield{},
                        Health{350, 350},
                        ScoreValue{100},
                        r::Transform3d{
                            .position = {-15.0f, 5.0f, 0.0f},
                            .scale = {2.5f, 2.5f, 2.5f},
                        },
                        Collider{.radius = 1.0f},
                        r::Mesh3d{.id = shield_handle, .color = r::Color{255, 255, 255, 255}, .rotation_offset = {0.0f, -(static_cast<float>(M_PI) / 2.0f), 0.0f}});

                    /* Bottom/front shield */
                    child.spawn(
                        Enemy{}, Shield{},
                        Health{350, 350},
                        ScoreValue{100},
                        r::Transform3d{
                            .position = {-15.0f, -5.0f, 0.0f},
                            .scale = {2.5f, 2.5f, 2.5f},
                        },
                        Collider{.radius = 1.0f},
                        r::Mesh3d{.id = shield_handle, .color = r::Color{255, 255, 255, 255}, .rotation_offset = {0.0f, -(static_cast<float>(M_PI) / 2.0f), 0.0f}});
                });
            } else {
                r::Logger::error("Failed to queue shield model for loading: assets/models/Shield.glb");
            }
        }

        /* Add the behavior "tag" component after spawning */
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
                r::Logger::warn("Unknown or unsupported boss behavior type, defaulting to HomingAttack.");
                boss_cmds.insert(HomingAttackBoss{});
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

static void boss_shield_color_system(
    r::ecs::Query<r::ecs::Mut<r::Mesh3d>, r::ecs::With<Boss>> boss_query,
    r::ecs::Query<r::ecs::Ref<Health>, r::ecs::With<Shield>> shield_query)
{
    bool shields_alive = false;
    for (auto [health, _s] : shield_query) {
        if (health.ptr->current > 0) {
            shields_alive = true;
            break;
        }
    }

    for (auto [mesh, _b] : boss_query) {
        if (shields_alive) {
            /* Tint the boss while shields are up (light blue tint) */
            mesh.ptr->color = r::Color{100, 180, 255, 255};
        } else {
            /* Restore base color */
            mesh.ptr->color = r::Color{255, 255, 255, 255};
        }
    }
}


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
                commands
                    .spawn(EnemyBullet{},
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
                        })
                    .insert(Unblockable{});
            }
        }
    }
}

static void boss_movement_homing_attack_system(r::ecs::Res<r::core::FrameTime> time,
    r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::Mut<Velocity>, r::ecs::Mut<HomingAttackBoss>> query)
{
    const float BATTLE_POSITION_X = 8.0f;
    const float VERTICAL_BOUND = 4.0f;

    for (auto [transform, velocity, behavior] : query) {
        behavior.ptr->state_timer -= time.ptr->delta_time;

        switch (behavior.ptr->current_state) {
            case HomingAttackBoss::State::Entering: {
                if (transform.ptr->position.x <= BATTLE_POSITION_X) {
                    transform.ptr->position.x = BATTLE_POSITION_X;
                    velocity.ptr->value = {0.0f, 0.0f, 0.0f};
                    behavior.ptr->current_state = HomingAttackBoss::State::Repositioning;
                    behavior.ptr->state_timer = 0.0f;///< Immediately reposition
                }
                break;
            }
            case HomingAttackBoss::State::Repositioning: {
                /* First, check if we need to select a new target position.
                -> This happens if the timer has expired (or was set to 0 on purpose). */
                if (behavior.ptr->state_timer <= 0.0f) {
                    float target_y = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * (VERTICAL_BOUND * 2.0f) - VERTICAL_BOUND;
                    behavior.ptr->target_position = {BATTLE_POSITION_X, target_y, 0.0f};
                    behavior.ptr->state_timer = 3.0f;///< Give it 3 seconds to reach the destination
                }

                /* Then, handle the movement towards the target */
                r::Vec3f direction = behavior.ptr->target_position - transform.ptr->position;
                float distance = direction.length();

                if (distance < 0.1f) {
                    velocity.ptr->value = {0.0f, 0.0f, 0.0f};
                    behavior.ptr->current_state = HomingAttackBoss::State::Attacking;
                    behavior.ptr->state_timer = 4.0f;///< Attack for 4 seconds
                } else {
                    velocity.ptr->value = direction.normalize() * BOSS_HOMING_MOVE_SPEED;
                }
                break;
            }
            case HomingAttackBoss::State::Attacking: {
                velocity.ptr->value = {0.0f, 0.0f, 0.0f};
                if (behavior.ptr->state_timer <= 0.0f) {
                    behavior.ptr->current_state = HomingAttackBoss::State::Repositioning;
                    /* Set timer to 0 to force an immediate new target selection in the Repositioning state. */
                    behavior.ptr->state_timer = 0.0f;
                }
                break;
            }
            default: {
                break;
            }
        }
    }
}

static void boss_shooting_homing_attack_system(r::ecs::Commands &commands, r::ecs::Res<r::core::FrameTime> time,
    r::ecs::Res<BossBulletAssets> bullet_assets,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Mut<BossShootTimer>, r::ecs::Ref<HomingAttackBoss>, r::ecs::Ref<Health>> query)
{
    for (auto [transform, timer, behavior, health] : query) {
        if (behavior.ptr->current_state != HomingAttackBoss::State::Attacking) {
            continue;
        }

        timer.ptr->time_left -= time.ptr->delta_time;

        if (timer.ptr->time_left <= 0.0f) {
            float fire_rate = 1.5f;
            /* Enraged state: fire faster when health is low */
            if (health.ptr->current <= health.ptr->max / 2) {
                fire_rate = 0.8f;
            }
            timer.ptr->time_left = fire_rate;

            /* Spawn a homing missile */
            commands.spawn(EnemyBullet{}, HomingEnemy{1.8f}, TimedDespawn{4.0f},
                r::Transform3d{
                    .position = transform.ptr->position,
                    .scale = {0.7f, 0.7f, 0.7f},
                },
                Velocity{
                    {-HOMING_MISSILE_SPEED, 0.0f, 0.0f},
                },
                Collider{
                    .radius = 0.5f,
                },
                r::Mesh3d{
                    .id = bullet_assets.ptr->small_missile,
                    .color = r::Color{255, 255, 255, 255},
                    .rotation_offset = {(static_cast<float>(M_PI) / 2.0f), 0.0f, -static_cast<float>(M_PI) / 2.0f},
                });
        }
    }
}

static void boss_movement_turret_system([[maybe_unused]] r::ecs::Query<r::ecs::With<TurretBoss>> query)
{
    /* TODO: Implement turret movement logic for the boss (for example, stationary but rotates) */
}

void EnemyPlugin::build(r::Application &app)
{
    app.add_systems<enemy_spawner_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()

        .add_systems<enemy_movement_homing_system, enemy_movement_sine_wave_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()
        .run_or<r::run_conditions::in_state<GameState::BossBattle>>()

        .add_systems<boss_spawn_system>(r::OnEnter(GameState::BossBattle))
        .run_unless<run_conditions::is_resuming_from_pause>()

        /* Systems for the Level 1 Boss */
        .add_systems<boss_movement_vertical_patrol_system, boss_shooting_vertical_patrol_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::BossBattle>>()

        /* Systems for the Level 2 Boss (Homing Attack) */
        .add_systems<boss_movement_homing_attack_system, boss_shooting_homing_attack_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::BossBattle>>()

    .add_systems<boss_movement_turret_system>(r::Schedule::UPDATE)
    .run_if<r::run_conditions::in_state<GameState::BossBattle>>()

    /* Tint boss while shields are alive */
    .add_systems<boss_shield_color_system>(r::Schedule::UPDATE)
    .run_if<r::run_conditions::in_state<GameState::BossBattle>>();
}
