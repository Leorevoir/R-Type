#include <components.hpp>
#include <plugins/force.hpp>
#include <resources.hpp>
#include <state.hpp>

#include "R-Engine/Components/Transform3d.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/FrameTime.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Plugins/InputPlugin.hpp>
#include <R-Engine/Plugins/MeshPlugin.hpp>

// clang-format off

/* ================================================================================= */
/* Constants */
/* ================================================================================= */

static constexpr float FORCE_LAUNCH_SPEED = 8.0f;
static constexpr float FORCE_RECALL_SPEED = 15.0f;
static constexpr float FORCE_REATTACH_DISTANCE = 0.5f;
static constexpr float FORCE_ACTION_COOLDOWN = 0.5f;
static constexpr float FORCE_FRONT_OFFSET_X = 1.75f;
static constexpr float FORCE_FIRE_RATE = 0.25f;
static constexpr float FORCE_BULLET_SPEED = 10.0f;

static constexpr float FORCE_AUTONOMOUS_FOLLOW_STIFFNESS = 5.0f; /* How quickly it moves to the target position */
static constexpr float FORCE_AUTONOMOUS_DAMPING = 0.95f; /* Reduces oscillation and slows it down over time */
static constexpr float FORCE_TARGET_X_OFFSET = 4.0f; /* Horizontal distance from the screen center it hovers at */

/* ================================================================================= */
/* Force Systems */
/* ================================================================================= */

static void force_control_system(r::ecs::Commands &commands, r::ecs::Res<r::UserInput> user_input, r::ecs::Res<r::InputMap> input_map,
    r::ecs::Res<r::core::FrameTime> time, r::ecs::Query<r::ecs::Mut<Player>> player_query,
    r::ecs::Query<r::ecs::Mut<Force>, r::ecs::Mut<r::Transform3d>, r::ecs::Ref<r::GlobalTransform3d>,
        r::ecs::Optional<r::ecs::Ref<r::ecs::Parent>>>
        force_query)
{
    bool is_force_pressed = input_map.ptr->isActionPressed("Force", *user_input.ptr);

    for (auto it = player_query.begin(); it != player_query.end(); ++it) {
        auto [player] = *it;
        if (player.ptr->force_cooldown > 0.f) {
            player.ptr->force_cooldown -= time.ptr->delta_time;
        }

        if (!is_force_pressed) {
            continue;
        }

        if (player.ptr->force_cooldown > 0.f) {
            continue;
        }

        if (player.ptr->force_entity == r::ecs::NULL_ENTITY) {
            r::Logger::error("force_control_system: Player has no force_entity linked!");
            continue;
        }

        bool action_taken = false;
        for (auto force_it = force_query.begin(); force_it != force_query.end(); ++force_it) {
            if (force_it.entity() != player.ptr->force_entity)
                continue;

            action_taken = true;
            auto [force, transform, global_transform, parent] = *force_it;

            if (force.ptr->is_attached) {
                if (!parent.ptr) {
                    r::Logger::error("force_control_system: Force is 'attached' but has no Parent component!");
                }
                player.ptr->force_cooldown = FORCE_ACTION_COOLDOWN;
                force.ptr->is_attached = false;

                transform.ptr->position = global_transform.ptr->position;
                transform.ptr->rotation = global_transform.ptr->rotation;

                commands.entity(force_it.entity()).remove<r::ecs::Parent>();
                commands.entity(force_it.entity()).insert(Velocity{{FORCE_LAUNCH_SPEED, 0.0f, 0.0f}});
                break;
                break;
            }

            if (!force.ptr->is_attached) {
                player.ptr->force_cooldown = FORCE_ACTION_COOLDOWN;
                force.ptr->is_attached = true;
                commands.entity(force_it.entity()).remove<Velocity>();
                break;
            }
        }
        if (is_force_pressed && !action_taken) {
            r::Logger::error("force_control_system: Force button was pressed, but the player's force_entity "
                + std::to_string(player.ptr->force_entity) + " was not found in the force_query!");
        }
    }
}

static void force_recall_system(r::ecs::Commands &commands, r::ecs::Res<r::core::FrameTime> time,
    r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::Ref<Force>, r::ecs::Without<r::ecs::Parent>> force_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::With<Player>> player_query)
{
    if (player_query.size() == 0)
        return;
    auto [player_transform, _p] = *player_query.begin();

    for (auto force_it = force_query.begin(); force_it != force_query.end(); ++force_it) {
        auto [transform, force, __] = *force_it;

        if (force.ptr->is_attached) {
            r::Vec3f direction = player_transform.ptr->position - transform.ptr->position;
            float distance = direction.length();

            if (distance > FORCE_REATTACH_DISTANCE) {
                transform.ptr->position += direction.normalize() * FORCE_RECALL_SPEED * time.ptr->delta_time;
            } else {
                commands.entity(force_it.entity()).insert(r::ecs::Parent{force.ptr->owner});

                /* Reset its local position relative to the player */
                transform.ptr->position = {FORCE_FRONT_OFFSET_X, 0.0f, 0.0f};
                transform.ptr->rotation = {0.f, 0.f, 0.f};
            }
        }
    }
}

static void force_autonomous_movement_system(
    r::ecs::Res<r::core::FrameTime> time,
    r::ecs::Query<r::ecs::Mut<Velocity>, r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Force>, r::ecs::Without<r::ecs::Parent>> force_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::With<Player>> player_query)
{
    if (player_query.size() == 0 || force_query.size() == 0) {
        return;
    }

    auto [player_transform, _p] = *player_query.begin();

    for (auto [velocity, transform, force, __] : force_query) {
        /* When recalling, stop all autonomous movement */
        if (force.ptr->is_attached) {
            velocity.ptr->value = {0.f, 0.f, 0.f};
            continue;
        }

        float y_target = player_transform.ptr->position.y;

        float x_target = (player_transform.ptr->position.x < 0) ? FORCE_TARGET_X_OFFSET : -FORCE_TARGET_X_OFFSET;

        r::Vec3f target_pos = {x_target, y_target, 0.0f};
        r::Vec3f current_pos = transform.ptr->position;

        r::Vec3f distance_to_target = target_pos - current_pos;
        r::Vec3f acceleration = distance_to_target * FORCE_AUTONOMOUS_FOLLOW_STIFFNESS;

        velocity.ptr->value += acceleration * time.ptr->delta_time;

        velocity.ptr->value *= FORCE_AUTONOMOUS_DAMPING;
    }
}

static void force_shooting_system(r::ecs::Commands &commands, r::ecs::Res<PlayerBulletAssets> bullet_assets, r::ecs::Res<r::core::FrameTime> time,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Mut<FireCooldown>, r::ecs::Ref<Force>, r::ecs::Without<r::ecs::Parent>> query)
{
    for (auto [transform, cooldown, force, _] : query) {
        /* Don't shoot when attached or being recalled. */
        if (force.ptr->is_attached) {
            continue;
        }

        cooldown.ptr->timer -= time.ptr->delta_time;
        if (cooldown.ptr->timer <= 0.0f) {
            cooldown.ptr->timer = FORCE_FIRE_RATE;
            commands.spawn(PlayerBullet{},
                r::Transform3d{
                    .position = transform.ptr->position,/* Spawn at the Force's current world position */
                    .scale = {1.5f, 1.5f, 1.5f}},
                Velocity{{FORCE_BULLET_SPEED, 0.0f, 0.0f}},
                Collider{0.2f},
                r::Mesh3d{
                    .id = bullet_assets.ptr->force_missile,
                    .color = r::Color{255, 255, 255, 255}, /* Teal color for Force bullets */
                    .rotation_offset = {-(static_cast<float>(M_PI) / 2.0f), 0.0f, -static_cast<float>(M_PI) / 2.0f}
                });
        }
    }
}

void ForcePlugin::build(r::Application &app)
{
    app.add_systems<force_control_system, force_recall_system, force_autonomous_movement_system, force_shooting_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()
        .run_or<r::run_conditions::in_state<GameState::BossBattle>>();
}
// clang-format on
