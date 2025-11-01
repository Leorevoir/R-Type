#include "plugins/debug.hpp"
#include "R-Engine/Components/Transform3d.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Backend.hpp>
#include <R-Engine/ECS/Event.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Plugins/InputPlugin.hpp>

#include <components/common.hpp>
#include <events/debug.hpp>
#include <state/game_state.hpp>

/* ================================================================================= */
/* Debug Systems */
/* ================================================================================= */

/**
 * @brief Draws a wireframe sphere for every entity with a Collider.
 */
static void debug_draw_colliders_system(r::ecs::Query<r::ecs::Ref<r::GlobalTransform3d>, r::ecs::Ref<Collider>> query)
{
    for (auto [transform, collider] : query) {
        r::Vec3f final_center = transform.ptr->position + collider.ptr->offset;
        ::Vector3 center = {final_center.x, final_center.y, final_center.z};
        ::Color color = {255, 0, 0, 255}; /* Red color for visibility */

        DrawSphereWires(center, collider.ptr->radius, 16, 16, color);
    }
}

/**
 * @brief Allows fast switching between levels using function keys.
 * @details F1 for Level 1, F2 for Level 2, etc.
 */
static void debug_level_switch_system(r::ecs::Res<r::UserInput> user_input, r::ecs::EventWriter<DebugSwitchLevelEvent> writer)
{
    if (user_input.ptr->isKeyPressed(KEY_F1)) {
        writer.send({0}); /* Switch to level index 0 */
    }
    if (user_input.ptr->isKeyPressed(KEY_F2)) {
        writer.send({1}); /* Switch to level index 1 */
    }
    if (user_input.ptr->isKeyPressed(KEY_F3)) {
        writer.send({2}); /* Switch to level index 2 */
    }
}

void DebugPlugin::build(r::Application &app)
{
    app.add_systems<debug_draw_colliders_system>(r::Schedule::RENDER_3D)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()
        .run_or<r::run_conditions::in_state<GameState::BossBattle>>();

    app.add_systems<debug_level_switch_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()
        .run_or<r::run_conditions::in_state<GameState::BossBattle>>();
}
