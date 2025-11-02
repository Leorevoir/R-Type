#include "plugins/player.hpp"
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
#include <R-Engine/Core/Filepath.hpp>
#include <algorithm>
#include <cmath>

#include <components/common.hpp>
#include <components/player.hpp>
#include <components/projectiles.hpp>
#include <resources/assets.hpp>
#include <state/game_state.hpp>
#include <state/run_conditions.hpp>
#include <plugins/ui_sfx.hpp>
#include <R-Engine/Plugins/AudioPlugin.hpp>

/* Player-specific SFX handles */
struct PlayerSfxHandles {
    r::AudioHandle laser = r::AudioInvalidHandle;
    r::AudioHandle launch = r::AudioInvalidHandle;
};

/* ================================================================================= */
/* Constants */
/* ================================================================================= */

static constexpr float PLAYER_SPEED = 6.0f;
static constexpr float BULLET_SPEED = 8.0f;
static constexpr float PLAYER_FIRE_RATE = 0.45f;
static constexpr float PLAYER_BOUNDS_PADDING = 0.5f;
static constexpr float WAVE_CANNON_CHARGE_START_DELAY = 0.2f;
static constexpr float FORCE_FRONT_OFFSET_X = 1.75f;

/* ================================================================================= */
/* Player Systems :: Helpers */
/* ================================================================================= */

static void spawn_player_force(r::ecs::ChildBuilder &parent, r::ecs::ResMut<r::Meshes> &meshes, r::ecs::Entity owner_id)
{
    r::MeshHandle force_mesh_handle = meshes.ptr->add("assets/models/force.glb");
    if (force_mesh_handle != r::MeshInvalidHandle) {
        parent.spawn(
            Force{
                .is_attached = true,
                .is_front_attachment = true,
                .owner = owner_id,
            },
            FireCooldown{},
            r::Transform3d{
                .position = {FORCE_FRONT_OFFSET_X, 0.0f, 0.0f},
                .scale = {0.3f, 0.3f, 0.3f},
            },
            Collider{
                .radius = 1.0f,
            },
            r::Mesh3d{
                .id = force_mesh_handle,
                .color = r::Color{255, 120, 0, 255},
                .rotation_offset = {-(static_cast<float>(M_PI) / 2.0f), 0.0f, 0.0f},
            });
    } else {
        r::Logger::error("Failed to queue force model for loading: assets/models/force.glb");
    }
}

static void fire_standard_shot(r::ecs::Commands &commands, r::ecs::ResMut<PlayerBulletAssets> &bullet_assets,
    r::ecs::Ref<r::Transform3d> transform, r::ecs::Res<PlayerSfxHandles> sfx, r::ecs::Res<UiSfxCounter> counter)
{
    /* --- Firing --- */
    commands.spawn(PlayerBullet{},
        r::Transform3d{
            .position = transform.ptr->position + r::Vec3f{0.6f, 0.0f, 0.0f},
            .scale = {0.2f, 0.2f, 0.2f},
        },
        Velocity{{BULLET_SPEED, 0.0f, 0.0f}}, Collider{0.2f},
        r::Mesh3d{
            .id = bullet_assets.ptr->laser_beam_handle,
            .color = r::Color{255, 255, 255, 255}, /* Yellow color for bullets */
            .rotation_offset = {-(static_cast<float>(M_PI) / 2.0f), 0.0f, -static_cast<float>(M_PI) / 2.0f},
        });

        /* Play launch SFX when a standard missile is spawned */
        if (sfx.ptr && sfx.ptr->launch != r::AudioInvalidHandle) {
            commands.spawn(UiSfxTag{}, UiSfxBorn{counter.ptr->frame}, r::AudioPlayer{sfx.ptr->launch}, r::AudioSink{});
        }

    /* Standard shot: no SFX here. Laser SFX is played only on release (wave cannon). */
}


static void fire_wave_cannon(r::ecs::Commands &commands, r::ecs::ResMut<r::Meshes> &meshes, r::ecs::Ref<r::Transform3d> transform,
    float charge_timer, r::ecs::Res<PlayerSfxHandles> sfx, r::ecs::Res<UiSfxCounter> counter)
{
    float charge_duration = charge_timer - WAVE_CANNON_CHARGE_START_DELAY;
    charge_duration = std::min(charge_duration, 2.0f); /* Max charge of 2s */

    float size_multiplier = 1.0f + (charge_duration / 2.0f);  /* Max charge -> 2x size */
    int damage = 10 + static_cast<int>(charge_duration * 45); /* Max charge -> 100 damage */

    ::Mesh beam_mesh_data = r::Mesh3d::Cube(1.0f);
    if (beam_mesh_data.vertexCount > 0) {
        r::MeshHandle beam_mesh_handle = meshes.ptr->add(std::move(beam_mesh_data));
        if (beam_mesh_handle != r::MeshInvalidHandle) {
            commands.spawn(
                WaveCannonBeam{
                    .charge_level = charge_duration,
                    .damage = damage,
                },
                r::Transform3d{
                    .position = transform.ptr->position + r::Vec3f{2.0f * size_multiplier, 0.0f, 0.0f},
                    .scale = {2.5f * size_multiplier, 0.4f * size_multiplier, 1.0f},
                },
                Velocity{{15.0f, 0.0f, 0.0f}},
                Collider{
                    .radius = 0.2f * size_multiplier,
                },
                r::Mesh3d{
                    .id = beam_mesh_handle,
                    .color = r::Color{98, 221, 255, 255}, /* R-Type cyan */
                });
            /* Play laser SFX at the same moment the beam is spawned (on release). */
            if (sfx.ptr && sfx.ptr->laser != r::AudioInvalidHandle) {
                commands.spawn(UiSfxTag{}, UiSfxBorn{counter.ptr->frame}, r::AudioPlayer{sfx.ptr->laser}, r::AudioSink{});
            }
        }
    }
}

static void handle_player_movement(r::ecs::Mut<Velocity> &velocity, r::ecs::Res<r::InputMap> const &input_map,
    r::ecs::Res<r::UserInput> const &user_input)
{
    r::Vec3f direction = {0.0f, 0.0f, 0.0f};
    if (input_map.ptr->isActionPressed("MoveUp", *user_input.ptr))
        direction.y += 1.0f;
    if (input_map.ptr->isActionPressed("MoveDown", *user_input.ptr))
        direction.y -= 1.0f;
    if (input_map.ptr->isActionPressed("MoveLeft", *user_input.ptr))
        direction.x -= 1.0f;
    if (input_map.ptr->isActionPressed("MoveRight", *user_input.ptr))
        direction.x += 1.0f;

    r::Vec2f axis_movement = user_input.ptr->getGamepadAxis(0);
    float deadzone = 0.2f;
    if (std::abs(axis_movement.x) > deadzone || std::abs(axis_movement.y) > deadzone) {
        direction.x = axis_movement.x;
        direction.y = -axis_movement.y;
    }

    velocity.ptr->value = (direction.length() > 0.0f) ? direction.normalize() * PLAYER_SPEED : r::Vec3f{0.0f, 0.0f, 0.0f};
}

static void handle_player_firing(r::ecs::Commands &commands, r::ecs::ResMut<r::Meshes> &meshes, r::ecs::Res<r::core::FrameTime> const &time,
    r::ecs::Ref<r::Transform3d> transform, r::ecs::Mut<FireCooldown> cooldown, r::ecs::Mut<Player> player,
    r::ecs::ResMut<PlayerBulletAssets> &bullet_assets, bool is_fire_pressed, r::ecs::Res<PlayerSfxHandles> sfx, r::ecs::Res<UiSfxCounter> counter)
{
    if (cooldown.ptr->timer > 0.0f) {
        cooldown.ptr->timer -= time.ptr->delta_time;
    }

    if (is_fire_pressed) {
        player.ptr->wave_cannon_charge_timer += time.ptr->delta_time;

            if (player.ptr->wave_cannon_charge_timer < WAVE_CANNON_CHARGE_START_DELAY && cooldown.ptr->timer <= 0.0f) {
            cooldown.ptr->timer = PLAYER_FIRE_RATE;
            fire_standard_shot(commands, bullet_assets, transform, sfx, counter);
        }
    } else { /* Fire button was released */
        if (player.ptr->wave_cannon_charge_timer >= WAVE_CANNON_CHARGE_START_DELAY) {
            fire_wave_cannon(commands, meshes, transform, player.ptr->wave_cannon_charge_timer, sfx, counter);
        }
        player.ptr->wave_cannon_charge_timer = 0.0f; /* Reset timer on release */
    }
}

/* ================================================================================= */
/* Player Systems */
/* ================================================================================= */

static void spawn_player_system(r::ecs::Commands &commands, r::ecs::ResMut<r::Meshes> meshes)
{
    r::MeshHandle player_mesh_handle = meshes.ptr->add("assets/models/R-9.glb");
    if (player_mesh_handle != r::MeshInvalidHandle) {
        auto player_cmds = commands.spawn(Player{}, r::Transform3d{.position = {-5.0f, 0.0f, 0.0f}, .scale = {3.0f, 3.0f, 3.0f}},
            Velocity{{0.0f, 0.0f, 0.0f}},
            Collider{
                .radius = 0.8f,
                .offset = {1.6f, 0.0f, 0.0f},
            },
            FireCooldown{},
            r::Mesh3d{
                .id = player_mesh_handle,
                .color = r::Color{255, 255, 255, 255},
                .rotation_offset = {0.0f, static_cast<float>(M_PI) / 2.0f, 0.0f},
            });
        player_cmds.with_children([&](r::ecs::ChildBuilder &parent) { spawn_player_force(parent, meshes, player_cmds.id()); });
    } else {
        r::Logger::error("Failed to queue player model for loading: assets/models/R-9.glb");
    }
}

static r::ecs::Entity find_force_entity_in_children(const r::ecs::Children &children, r::ecs::Query<r::ecs::With<Force>> &is_force_query)
{
    for (const auto child_entity : children.entities) {
        for (auto force_it = is_force_query.begin(); force_it != is_force_query.end(); ++force_it) {
            if (force_it.entity() == child_entity) {
                return child_entity;
            }
        }
    }
    return r::ecs::NULL_ENTITY;
}

static void link_force_to_player_system(r::ecs::Query<r::ecs::Mut<Player>, r::ecs::Ref<r::ecs::Children>> player_query,
    r::ecs::Query<r::ecs::With<Force>> is_force_query)
{
    for (auto [player, children] : player_query) {
        if (player.ptr->force_entity != r::ecs::NULL_ENTITY) {
            continue; /* Already linked, skip. */
        }

        if (children.ptr->entities.empty()) {
            continue;
        }

        r::ecs::Entity force_entity = find_force_entity_in_children(*children.ptr, is_force_query);
        if (force_entity != r::ecs::NULL_ENTITY) {
            player.ptr->force_entity = force_entity;
        }
    }
}

static void setup_bullet_assets_system(r::ecs::Commands &commands, r::ecs::ResMut<r::Meshes> meshes,
    r::ecs::ResMut<r::AudioManager> audio)
{
    PlayerBulletAssets bullet_assets;

    bullet_assets.laser_beam_handle = meshes.ptr->add("assets/models/PlayerMissile.glb");
    if (bullet_assets.laser_beam_handle == r::MeshInvalidHandle) {
        r::Logger::error("Failed to queue player missile model !");
    }

    bullet_assets.force_missile = meshes.ptr->add("assets/models/SmallMissile.glb");
    if (bullet_assets.force_missile == r::MeshInvalidHandle) {
        r::Logger::error("Failed to queue force small missile model !");
    }

    commands.insert_resource(bullet_assets);

    /* Load player SFX */
    PlayerSfxHandles sfx;
    sfx.laser = audio.ptr->load(r::path::get("assets/sounds/laser_beam.mp3"));
    if (sfx.laser == r::AudioInvalidHandle) {
        r::Logger::warn("Failed to load assets/sounds/laser_beam.mp3");
    } else {
        r::Logger::info(std::string{"PlayerSfx: laser handle="} + std::to_string(sfx.laser));
    }
    sfx.launch = audio.ptr->load(r::path::get("assets/sounds/launch.mp3"));
    if (sfx.launch == r::AudioInvalidHandle) {
        r::Logger::warn("Failed to load assets/sounds/launch.mp3");
    } else {
        r::Logger::info(std::string{"PlayerSfx: launch handle="} + std::to_string(sfx.launch));
    }
    commands.insert_resource(sfx);
}

static void player_input_system(r::ecs::Commands &commands, r::ecs::Res<r::UserInput> user_input, r::ecs::Res<r::InputMap> input_map,
    r::ecs::ResMut<PlayerBulletAssets> bullet_assets, r::ecs::Res<r::core::FrameTime> time, r::ecs::ResMut<r::Meshes> meshes,
    r::ecs::Res<PlayerSfxHandles> sfx, r::ecs::Res<UiSfxCounter> counter,
    r::ecs::Query<r::ecs::Mut<Velocity>, r::ecs::Ref<r::Transform3d>, r::ecs::Mut<FireCooldown>, r::ecs::Mut<Player>> query)
{
    const bool is_fire_pressed = input_map.ptr->isActionPressed("Fire", *user_input.ptr);

    for (auto [velocity, transform, cooldown, player] : query) {
        handle_player_movement(velocity, input_map, user_input);
        handle_player_firing(commands, meshes, time, transform, cooldown, player, bullet_assets, is_fire_pressed, sfx, counter);
    }
}

static void screen_bounds_system(r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::With<Player>> query, r::ecs::Res<r::Camera3d> camera,
    r::ecs::Res<r::WindowPluginConfig> window_config)
{
    if (!camera.ptr || !window_config.ptr) {
        return;
    }

    const float distance = camera.ptr->position.z;
    const float aspect_ratio = static_cast<float>(window_config.ptr->size.width) / static_cast<float>(window_config.ptr->size.height);
    const float fovy_rad = camera.ptr->fovy * (r::R_PI / 180.0f);

    const float view_height = 2.0f * distance * tanf(fovy_rad / 2.0f);
    const float view_width = view_height * aspect_ratio;

    const float half_height = view_height / 2.0f;
    const float half_width = view_width / 2.0f;

    for (auto [transform, _] : query) {
        transform.ptr->position.x =
            std::clamp(transform.ptr->position.x, -half_width + PLAYER_BOUNDS_PADDING, half_width - PLAYER_BOUNDS_PADDING);
        transform.ptr->position.y =
            std::clamp(transform.ptr->position.y, -half_height + PLAYER_BOUNDS_PADDING, half_height - PLAYER_BOUNDS_PADDING);
    }
}

static void autoplay_player_system(r::ecs::Query<r::ecs::Mut<Velocity>, r::ecs::With<Player>> query)
{
    for (auto [velocity, _] : query) {
        velocity.ptr->value = {0.5f, 0.0f, 0.0f};
    }
}

static void cleanup_player_system(r::ecs::Commands &commands, r::ecs::Query<r::ecs::With<Player>> query)
{
    for (auto it = query.begin(); it != query.end(); ++it) {
        commands.despawn(it.entity());
    }
}

void PlayerPlugin::build(r::Application &app)
{
    app.add_systems<spawn_player_system>(r::OnEnter{GameState::EnemiesBattle})
        .run_unless<run_conditions::is_resuming_from_pause>()
        .add_systems<link_force_to_player_system, player_input_system, screen_bounds_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()
        .run_or<r::run_conditions::in_state<GameState::BossBattle>>()
        .add_systems<spawn_player_system>(r::OnEnter{GameState::MainMenu})
        .add_systems<setup_bullet_assets_system>(r::OnEnter{GameState::EnemiesBattle})
        .run_unless<run_conditions::is_resuming_from_pause>()

        .add_systems<autoplay_player_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::MainMenu>>()
        .add_systems<cleanup_player_system>(r::OnExit{GameState::MainMenu});
}
