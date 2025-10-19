#include <components.hpp>
#include <plugins/debug.hpp>
#include <state.hpp>

#include "R-Engine/Components/Transform3d.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Backend.hpp>
#include <R-Engine/ECS/Query.hpp>

/* ================================================================================= */
/* Run Condition */
/* ================================================================================= */

static bool is_in_gameplay_state(r::ecs::Res<r::State<GameState>> state)
{
    if (!state.ptr)
        return false;
    auto current_state = state.ptr->current();
    return current_state == GameState::EnemiesBattle || current_state == GameState::BossBattle;
}

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
    app.add_systems<debug_draw_colliders_system>(r::Schedule::RENDER_3D).run_if<is_in_gameplay_state>();
}
