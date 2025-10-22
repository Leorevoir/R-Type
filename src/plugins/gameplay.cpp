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
#include <cstdlib>

// clang-format off

/* ================================================================================= */
/* Constants */
/* ================================================================================= */

static constexpr float ENEMY_SPEED = 2.0f;
static constexpr float BULLET_SPEED = 8.0f;
static constexpr float BOSS_VERTICAL_SPEED = 4.5f;
static constexpr float BOSS_UPPER_BOUND = 4.0f;
static constexpr float BOSS_LOWER_BOUND = -15.0f;

/* ================================================================================= */
/* Gameplay Systems */
/* ================================================================================= */

static void enemy_spawner_system(r::ecs::Commands& commands, r::ecs::ResMut<EnemySpawnTimer> spawn_timer,
                                 r::ecs::Res<r::core::FrameTime> time, r::ecs::ResMut<r::Meshes> meshes)
{
    spawn_timer.ptr->time_left -= time.ptr->delta_time;
    if (spawn_timer.ptr->time_left <= 0.0f) {
        spawn_timer.ptr->time_left = ENEMY_SPAWN_INTERVAL;

        float random_y = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 10.0f - 5.0f;

        ::Model enemy_model_data = r::Mesh3d::Glb("assets/models/enemy.glb");
        if (enemy_model_data.meshCount > 0) {
            r::MeshHandle enemy_mesh_handle = meshes.ptr->add(enemy_model_data);
            if (enemy_mesh_handle != r::MeshInvalidHandle) {
                commands.spawn(
                    Enemy{},
                    r::Transform3d{
                        .position = {15.0f, random_y, 0.0f},
                        .scale = {1.0f, 1.0f, 1.0f}
                    },
                    Velocity{{-ENEMY_SPEED, 0.0f, 0.0f}},
                    Collider{0.5f},
                    r::Mesh3d{
                        .id = enemy_mesh_handle,
                        .color = r::Color{255, 255, 255, 255},
                        .rotation_offset = {0.0f, -(static_cast<float>(M_PI) / 2.0f), 0.0f}
                    }
                );
            }
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

static void boss_spawn_system(r::ecs::Commands& commands, r::ecs::ResMut<r::Meshes> meshes)
{
    ::Model boss_model_data = r::Mesh3d::Glb("assets/models/Boss.glb");

    if (boss_model_data.meshCount > 0) {
        r::MeshHandle boss_mesh_handle = meshes.ptr->add(boss_model_data);
        if (boss_mesh_handle != r::MeshInvalidHandle) {
            commands.spawn(
                Boss{},
                BossShootTimer{},
                Health{400, 400},
                r::Transform3d{
                    .position = {12.0f, -10.0f, 0.0f},
                    .scale = {0.5f, 0.5f, 0.5f}
                },
                Velocity{{0.0f, BOSS_VERTICAL_SPEED, 0.0f}},
                Collider{.radius = 5.5f, .offset = {-2.5f, 4.0f, 0.0f}},
                r::Mesh3d{
                    .id = boss_mesh_handle,
                    .color = r::Color{255, 255, 255, 255},
                    .rotation_offset = {0.0f, -(static_cast<float>(M_PI) / 2.0f), 0.0f}
                }
            );
        }
    }
}

static void boss_movement_system(r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Mut<Velocity>, r::ecs::With<Boss>> query)
{
    for (auto [transform, velocity, _] : query) {
        if (transform.ptr->position.y > BOSS_UPPER_BOUND && velocity.ptr->value.y > 0) {
            velocity.ptr->value.y *= -1;
        } else if (transform.ptr->position.y < BOSS_LOWER_BOUND && velocity.ptr->value.y < 0) {
            velocity.ptr->value.y *= -1;
        }
    }
}

static void boss_shooting_system(r::ecs::Commands& commands, r::ecs::Res<r::core::FrameTime> time, r::ecs::ResMut<r::Meshes> meshes,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Mut<BossShootTimer>, r::ecs::Ref<Health>, r::ecs::With<Boss>> query)
{
    for (auto [transform, timer, health, _] : query) {
        timer.ptr->time_left -= time.ptr->delta_time;

        if (timer.ptr->time_left <= 0.0f) {
            timer.ptr->time_left = BossShootTimer::FIRE_RATE;

            ::Mesh bullet_mesh_data = r::Mesh3d::Circle(2.0f, 16);
            if (bullet_mesh_data.vertexCount == 0 || !bullet_mesh_data.vertices) continue;

            r::MeshHandle bullet_mesh_handle = meshes.ptr->add(bullet_mesh_data);
            if (bullet_mesh_handle == r::MeshInvalidHandle) continue;

            /* Main gun */
            commands.spawn(
                EnemyBullet{},
                r::Transform3d{
                    .position = transform.ptr->position - r::Vec3f{1.6f, 0.0f, 0.0f},
                    .rotation = {-(static_cast<float>(M_PI) / 2.0f), 0.0f, static_cast<float>(M_PI) / 2.0f},
                    .scale = {0.3f, 0.3f, 0.3f}
                },
                Velocity{{-BULLET_SPEED, 0.0f, 0.0f}},
                Collider{.radius = 0.4f},
                r::Mesh3d{bullet_mesh_handle, r::Color{255, 80, 220, 255}}
            );

            /* Second gun when damaged */
            if (health.ptr->current <= health.ptr->max / 2) {
                commands.spawn(
                    EnemyBullet{},
                    r::Transform3d{
                        .position = transform.ptr->position + r::Vec3f{0.0f, 5.5f, 0.0f},
                        .rotation = {-(static_cast<float>(M_PI) / 2.0f), 0.0f, static_cast<float>(M_PI) / 2.0f},
                        .scale = {0.6f, 0.6f, 0.6f}
                    },
                    Velocity{{-BULLET_SPEED, 0.0f, 0.0f}},
                    Collider{.radius = 0.8f},
                    r::Mesh3d{bullet_mesh_handle, r::Color{255, 150, 50, 255}}
                );
            }
        }
    }
}

static void movement_system(r::ecs::Res<r::core::FrameTime> time, r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::Ref<Velocity>> query)
{
    for (auto [transform, velocity] : query) {
        transform.ptr->position = transform.ptr->position + velocity.ptr->value * time.ptr->delta_time;
    }
}

void GameplayPlugin::build(r::Application& app)
{
    app.insert_resource(EnemySpawnTimer{})
        .insert_resource(BossSpawnTimer{})

        .add_systems<movement_system>(r::Schedule::UPDATE)

        .add_systems<setup_boss_fight_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()

        .add_systems<enemy_spawner_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()

        .add_systems<boss_spawn_system>(r::OnEnter(GameState::BossBattle))
        .add_systems<boss_movement_system, boss_shooting_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::BossBattle>>();
}
// clang-format on
