#include <components.hpp>
#include <plugins/player.hpp>
#include <state.hpp>

#include "R-Engine/Components/Transform3d.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/FrameTime.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/Maths/Quaternion.hpp>
#include <R-Engine/Plugins/InputPlugin.hpp>
#include <R-Engine/Plugins/MeshPlugin.hpp>
#include <R-Engine/Plugins/RenderPlugin.hpp>
#include <R-Engine/Plugins/WindowPlugin.hpp>
#include <cmath>

// clang-format off

/* ================================================================================= */
/* Constants */
/* ================================================================================= */

static constexpr float PLAYER_SPEED = 3.5f;
static constexpr float BULLET_SPEED = 8.0f;
static constexpr float PLAYER_FIRE_RATE = 0.15f;
static constexpr float PLAYER_BOUNDS_PADDING = 0.5f;
static constexpr float FORCE_FRONT_OFFSET_X = 1.75f;

/* ================================================================================= */
/* Run Condition */
/* ================================================================================= */

static bool is_in_gameplay_state(r::ecs::Res<r::State<GameState>> state)
{
    if (!state.ptr) {
        return false;
    }
    auto current_state = state.ptr->current();
    return current_state == GameState::EnemiesBattle || current_state == GameState::BossBattle;
}

/* ================================================================================= */
/* Player Systems */
/* ================================================================================= */

static void spawn_player_system(r::ecs::Commands& commands, r::ecs::ResMut<r::Meshes> meshes)
{
    ::Model player_model_data = r::Mesh3d::Glb("assets/models/R-9.glb");
    if (player_model_data.meshCount > 0) {
        r::MeshHandle player_mesh_handle = meshes.ptr->add(player_model_data);

        if (player_mesh_handle != r::MeshInvalidHandle) {
            auto player_cmds = commands.spawn(
                Player{},
                r::Transform3d{
                    .position = {-5.0f, 0.0f, 0.0f},
                    .scale = {3.0f, 3.0f, 3.0f}
                },
                Velocity{{0.0f, 0.0f, 0.0f}},
                Collider{.radius = 0.8f, .offset = {1.6f, 0.0f, 0.0f}},
                FireCooldown{},
                r::Mesh3d{
                    .id = player_mesh_handle,
                    .color = r::Color{255, 255, 255, 255},
                    .rotation_offset = {0.0f, static_cast<float>(M_PI) / 2.0f, 0.0f} /* Visual rotation 90 degrees Y */
                }
            );
            r::MeshHandle force_mesh_handle = r::MeshInvalidHandle;
            ::Model force_model_data = r::Mesh3d::Glb("assets/models/force.glb");
            if (force_model_data.meshCount > 0) {
                force_mesh_handle = meshes.ptr->add(force_model_data);
            }

            if (force_mesh_handle != r::MeshInvalidHandle) {
                player_cmds.with_children([&](r::ecs::ChildBuilder& parent) {
                    parent.spawn(
                        Force{
                            .is_attached = true,
                            .is_front_attachment = true,
                            .owner = player_cmds.id()
                        },
                        FireCooldown{},
                        r::Transform3d{
                            .position = {FORCE_FRONT_OFFSET_X, 0.0f, 0.0f},
                            .scale = {0.3f, 0.3f, 0.3f}
                        },
                        Collider{.radius = 1.0f},
                        r::Mesh3d{
                            .id = force_mesh_handle,
                            .color = r::Color{255, 120, 0, 255},
                            .rotation_offset = {-(static_cast<float>(M_PI) / 2.0f), 0.0f, 0.0f}
                        }
                    );
                });
            }
        }
    }
}

static void link_force_to_player_system(
    r::ecs::Query<r::ecs::Mut<Player>, r::ecs::Ref<r::ecs::Children>> player_query,
    r::ecs::Query<r::ecs::With<Force>> is_force_query)
{
    for (auto it = player_query.begin(); it != player_query.end(); ++it) {
        auto [player, children] = *it;

        if (player.ptr->force_entity != r::ecs::NULL_ENTITY) {
            continue; /* Already linked, skip. */
        }

        if (children.ptr->entities.empty()) continue;

        for (const auto child_entity : children.ptr->entities) {
            for (auto force_it = is_force_query.begin(); force_it != is_force_query.end(); ++force_it) {
                if (force_it.entity() == child_entity) {
                    player.ptr->force_entity = child_entity;
                    goto next_player;
                }
            }
        }
    next_player:;
    }
}

static void player_input_system(
    r::ecs::Commands& commands, r::ecs::Res<r::UserInput> user_input, r::ecs::Res<r::InputMap> input_map,
    r::ecs::ResMut<r::Meshes> meshes, r::ecs::Res<r::core::FrameTime> time,
    r::ecs::Query<r::ecs::Mut<Velocity>, r::ecs::Ref<r::Transform3d>, r::ecs::Mut<FireCooldown>, r::ecs::With<Player>> query)
{
    for (auto [velocity, transform, cooldown, _] : query) {
        /* --- Cooldown --- */
        if (cooldown.ptr->timer > 0.0f) {
            cooldown.ptr->timer -= time.ptr->delta_time;
        }

        /* --- Movement --- */
        r::Vec3f direction = {0.0f, 0.0f, 0.0f};
        if (input_map.ptr->isActionPressed("MoveUp", *user_input.ptr)) {
            direction.y += 1.0f;
        }
        if (input_map.ptr->isActionPressed("MoveDown", *user_input.ptr)) {
            direction.y -= 1.0f;
        }
        if (input_map.ptr->isActionPressed("MoveLeft", *user_input.ptr)) {
            direction.x -= 1.0f;
        }
        if (input_map.ptr->isActionPressed("MoveRight", *user_input.ptr)) {
            direction.x += 1.0f;
        }

        velocity.ptr->value = (direction.length() > 0.0f) ? direction.normalize() * PLAYER_SPEED : r::Vec3f{0.0f, 0.0f, 0.0f};

        /* --- Firing --- */
        if (input_map.ptr->isActionPressed("Fire", *user_input.ptr) && cooldown.ptr->timer <= 0.0f) {
            cooldown.ptr->timer = PLAYER_FIRE_RATE;
            ::Mesh bullet_mesh_data = r::Mesh3d::Circle(0.5f, 16);
            if (bullet_mesh_data.vertexCount > 0 && bullet_mesh_data.vertices) {
                r::MeshHandle bullet_mesh_handle = meshes.ptr->add(bullet_mesh_data);
                if (bullet_mesh_handle != r::MeshInvalidHandle) {
                    commands.spawn(
                        PlayerBullet{},
                        r::Transform3d{
                            .position = transform.ptr->position + r::Vec3f{0.6f, 0.0f, 0.0f},
                            .scale = {0.5f, 0.5f, 0.5f}
                        },
                        Velocity{{BULLET_SPEED, 0.0f, 0.0f}},
                        Collider{0.2f},
                        r::Mesh3d{
                            .id = bullet_mesh_handle,
                            .color = r::Color{255, 200, 80, 255}, /* Yellow color for bullets */
                            .rotation_offset = {-(static_cast<float>(M_PI) / 2.0f), 0.0f, static_cast<float>(M_PI) / 2.0f}
                        }
                    );
                }
            }
        }
    }
}

static void screen_bounds_system(r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::With<Player>> query, r::ecs::Res<r::Camera3d> camera,
    r::ecs::Res<r::WindowPluginConfig> window_config)
{
    if (!camera.ptr || !window_config.ptr) {
        return;
    }

    const float distance = camera.ptr->position.z;
    const float aspect_ratio =
        static_cast<float>(window_config.ptr->size.width) / static_cast<float>(window_config.ptr->size.height);
    const float fovy_rad = camera.ptr->fovy * (r::R_PI / 180.0f);

    const float view_height = 2.0f * distance * std::tan(fovy_rad / 2.0f);
    const float view_width = view_height * aspect_ratio;

    const float half_height = view_height / 2.0f;
    const float half_width = view_width / 2.0f;

    for (auto [transform, _] : query) {
        /* Clamp X position */
        if (transform.ptr->position.x < -half_width + PLAYER_BOUNDS_PADDING) {
            transform.ptr->position.x = -half_width + PLAYER_BOUNDS_PADDING;
        }
        if (transform.ptr->position.x > half_width - PLAYER_BOUNDS_PADDING) {
            transform.ptr->position.x = half_width - PLAYER_BOUNDS_PADDING;
        }

        /* Clamp Y position */
        if (transform.ptr->position.y < -half_height + PLAYER_BOUNDS_PADDING) {
            transform.ptr->position.y = -half_height + PLAYER_BOUNDS_PADDING;
        }
        if (transform.ptr->position.y > half_height - PLAYER_BOUNDS_PADDING) {
            transform.ptr->position.y = half_height - PLAYER_BOUNDS_PADDING;
        }
    }
}

void PlayerPlugin::build(r::Application& app)
{
    app.add_systems<spawn_player_system>(r::OnEnter{GameState::EnemiesBattle})
        .add_systems<link_force_to_player_system,
                     player_input_system, screen_bounds_system>(r::Schedule::UPDATE)
        .run_if<is_in_gameplay_state>();
}
// clang-format on
