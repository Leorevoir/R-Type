#include "plugins/debug.hpp"
#include "R-Engine/Components/Transform3d.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Backend.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>

#include <components/common.hpp>
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

void DebugPlugin::build(r::Application &app)
{
    app.add_systems<debug_draw_colliders_system>(r::Schedule::RENDER_3D)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()
        .run_or<r::run_conditions::in_state<GameState::BossBattle>>();
}
