#include <components.hpp>
#include <plugins/player.hpp>
#include <resources.hpp>
#include <state.hpp>

#include "R-Engine/Components/Transform3d.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/FrameTime.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Maths/Quaternion.hpp>
#include <R-Engine/Plugins/InputPlugin.hpp>
#include <R-Engine/Plugins/MeshPlugin.hpp>
#include <R-Engine/Plugins/RenderPlugin.hpp>
#include <R-Engine/Plugins/WindowPlugin.hpp>
#include <algorithm>
#include <cmath>

// clang-format off

/* ================================================================================= */
/* Constants */
/* ================================================================================= */

static constexpr float PLAYER_SPEED = 3.5f;
static constexpr float BULLET_SPEED = 8.0f;
static constexpr float PLAYER_FIRE_RATE = 0.45f;
static constexpr float PLAYER_BOUNDS_PADDING = 0.5f;
static constexpr float WAVE_CANNON_CHARGE_START_DELAY = 0.2f;
static constexpr float FORCE_FRONT_OFFSET_X = 1.75f;

/* ================================================================================= */
/* Player Systems :: Helpers */
/* ================================================================================= */

static void spawn_player_force(r::ecs::ChildBuilder& parent, r::ecs::ResMut<r::Meshes>& meshes, r::ecs::Entity owner_id)
{
    ::Model force_model_data = r::Mesh3d::Glb("assets/models/force.glb");
    if (force_model_data.meshCount > 0) {
        r::MeshHandle force_mesh_handle = meshes.ptr->add(force_model_data);
        if (force_mesh_handle != r::MeshInvalidHandle) {
            parent.spawn(
                Force{
                    .is_attached = true,
                    .is_front_attachment = true,
                    .owner = owner_id
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
        }
    }
}

static void fire_standard_shot(r::ecs::Commands& commands, r::ecs::ResMut<PlayerBulletAssets>& bullet_assets, r::ecs::Ref<r::Transform3d> transform)
{
    /* --- Firing --- */
    commands.spawn(
        PlayerBullet{},
        r::Transform3d{
            .position = transform.ptr->position + r::Vec3f{0.6f, 0.0f, 0.0f},
            .scale = {0.2f, 0.2f, 0.2f}
        },
        Velocity{{BULLET_SPEED, 0.0f, 0.0f}},
        Collider{0.2f},
        r::Mesh3d{
            .id = bullet_assets.ptr->laser_beam_handle,
            .color = r::Color{255, 255, 255, 255}, /* Yellow color for bullets */
            .rotation_offset = {-(static_cast<float>(M_PI) / 2.0f), 0.0f, -static_cast<float>(M_PI) / 2.0f}
        }
    );
}

static void fire_wave_cannon(r::ecs::Commands& commands, r::ecs::ResMut<r::Meshes>& meshes, r::ecs::Ref<r::Transform3d> transform, float charge_timer)
{
    float charge_duration = charge_timer - WAVE_CANNON_CHARGE_START_DELAY;
    charge_duration = std::min(charge_duration, 2.0f); /* Max charge of 2s */

    float size_multiplier = 1.0f + (charge_duration / 2.0f); /* Max charge -> 2x size */
    int damage = 10 + static_cast<int>(charge_duration * 45); /* Max charge -> 100 damage */

    ::Mesh beam_mesh_data = r::Mesh3d::Cube(1.0f);
    r::MeshHandle beam_mesh_handle = meshes.ptr->add(beam_mesh_data);
    if (beam_mesh_handle != r::MeshInvalidHandle) {
        commands.spawn(WaveCannonBeam{.charge_level = charge_duration, .damage = damage},
            r::Transform3d{.position = transform.ptr->position + r::Vec3f{2.0f * size_multiplier, 0.0f, 0.0f},
                .scale = {2.5f * size_multiplier, 0.4f * size_multiplier, 1.0f}},
            Velocity{{15.0f, 0.0f, 0.0f}}, Collider{.radius = 0.2f * size_multiplier},
            r::Mesh3d{
                .id = beam_mesh_handle,
                .color = r::Color{98, 221, 255, 255}, /* R-Type cyan */
            });
    }
}

static void handle_player_movement(r::ecs::Mut<Velocity>& velocity, r::ecs::Res<r::InputMap> const & input_map, r::ecs::Res<r::UserInput> const & user_input)
{
    r::Vec3f direction = {0.0f, 0.0f, 0.0f};
    if (input_map.ptr->isActionPressed("MoveUp", *user_input.ptr)) direction.y += 1.0f;
    if (input_map.ptr->isActionPressed("MoveDown", *user_input.ptr)) direction.y -= 1.0f;
    if (input_map.ptr->isActionPressed("MoveLeft", *user_input.ptr)) direction.x -= 1.0f;
    if (input_map.ptr->isActionPressed("MoveRight", *user_input.ptr)) direction.x += 1.0f;

    velocity.ptr->value = (direction.length() > 0.0f) ? direction.normalize() * PLAYER_SPEED : r::Vec3f{0.0f, 0.0f, 0.0f};
}

static void handle_player_firing(r::ecs::Commands& commands, r::ecs::ResMut<r::Meshes>& meshes, r::ecs::Res<r::core::FrameTime> const & time,
    r::ecs::Ref<r::Transform3d> transform, r::ecs::Mut<FireCooldown> cooldown, r::ecs::Mut<Player> player, r::ecs::ResMut<PlayerBulletAssets>& bullet_assets, bool is_fire_pressed)
{
    if (cooldown.ptr->timer > 0.0f) {
        cooldown.ptr->timer -= time.ptr->delta_time;
    }

    if (is_fire_pressed) {
        player.ptr->wave_cannon_charge_timer += time.ptr->delta_time;

        if (player.ptr->wave_cannon_charge_timer < WAVE_CANNON_CHARGE_START_DELAY && cooldown.ptr->timer <= 0.0f) {
            cooldown.ptr->timer = PLAYER_FIRE_RATE;
            fire_standard_shot(commands, bullet_assets, transform);
        }
    } else { /* Fire button was released */
        if (player.ptr->wave_cannon_charge_timer >= WAVE_CANNON_CHARGE_START_DELAY) {
            fire_wave_cannon(commands, meshes, transform, player.ptr->wave_cannon_charge_timer);
        }
        player.ptr->wave_cannon_charge_timer = 0.0f; /* Reset timer on release */
    }
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
                    .rotation_offset = {0.0f, static_cast<float>(M_PI) / 2.0f, 0.0f}
                }
            );
            player_cmds.with_children([&](r::ecs::ChildBuilder& parent) {
                spawn_player_force(parent, meshes, player_cmds.id());
            });
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

static void setup_bullet_assets_system(r::ecs::Commands& commands, r::ecs::ResMut<r::Meshes> meshes)
{
    PlayerBulletAssets bullet_assets;

    ::Model missile_model_data = r::Mesh3d::Glb("assets/models/PlayerMissile.glb");
    if (missile_model_data.meshCount > 0) {
        bullet_assets.laser_beam_handle = meshes.ptr->add(missile_model_data);
    }

    if (bullet_assets.laser_beam_handle == r::MeshInvalidHandle) {
        r::Logger::error("Failed to load player missile model !");
    }

    ::Model missile_force_model_data = r::Mesh3d::Glb("assets/models/SmallMissile.glb");
    if (missile_force_model_data.meshCount > 0) {
        bullet_assets.force_missile = meshes.ptr->add(missile_force_model_data);
    }

    if (bullet_assets.force_missile == r::MeshInvalidHandle) {
        r::Logger::error("Failed to load force small missile model !");
    }

    commands.insert_resource(bullet_assets);
}

static void player_input_system(
    r::ecs::Commands& commands, r::ecs::Res<r::UserInput> user_input, r::ecs::Res<r::InputMap> input_map,
    r::ecs::ResMut<PlayerBulletAssets> bullet_assets, r::ecs::Res<r::core::FrameTime> time, r::ecs::ResMut<r::Meshes> meshes,
    r::ecs::Query<r::ecs::Mut<Velocity>, r::ecs::Ref<r::Transform3d>, r::ecs::Mut<FireCooldown>, r::ecs::Mut<Player>> query)
{
    const bool is_fire_pressed = input_map.ptr->isActionPressed("Fire", *user_input.ptr);

    for (auto [velocity, transform, cooldown, player] : query) {
        handle_player_movement(velocity, input_map, user_input);
        handle_player_firing(commands, meshes, time, transform, cooldown, player, bullet_assets, is_fire_pressed);
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

    const float view_height = 2.0f * distance * std::tanf(fovy_rad / 2.0f);
    const float view_width = view_height * aspect_ratio;

    const float half_height = view_height / 2.0f;
    const float half_width = view_width / 2.0f;

    for (auto [transform, _] : query) {
        transform.ptr->position.x = std::clamp(transform.ptr->position.x, -half_width + PLAYER_BOUNDS_PADDING, half_width - PLAYER_BOUNDS_PADDING);
        transform.ptr->position.y = std::clamp(transform.ptr->position.y, -half_height + PLAYER_BOUNDS_PADDING, half_height - PLAYER_BOUNDS_PADDING);
    }
}


static void autoplay_player_system(r::ecs::Query<r::ecs::Mut<Velocity>, r::ecs::With<Player>> query)
{
    for (auto [velocity, _] : query) {
        velocity.ptr->value = {0.5f, 0.0f, 0.0f};
    }
}

static void cleanup_player_system(r::ecs::Commands& commands, r::ecs::Query<r::ecs::With<Player>> query)
{
    for (auto it = query.begin(); it != query.end(); ++it) {
        commands.despawn(it.entity());
    }
}

void PlayerPlugin::build(r::Application& app)
{
    app.add_systems<spawn_player_system>(r::OnEnter{GameState::EnemiesBattle})
        .add_systems<link_force_to_player_system,
                     player_input_system, screen_bounds_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()
        .run_or<r::run_conditions::in_state<GameState::BossBattle>>()
        .add_systems<spawn_player_system>(r::OnEnter{GameState::MainMenu})
        .add_systems<setup_bullet_assets_system>(r::OnEnter{GameState::EnemiesBattle})

        .add_systems<autoplay_player_system>(r::Schedule::UPDATE)
            .run_if<r::run_conditions::in_state<GameState::MainMenu>>()
        .add_systems<cleanup_player_system>(r::OnExit{GameState::MainMenu});
}
// clang-format on
