#include "R-Engine/Components/Transform3d.hpp"
#include "R-Engine/Maths/Maths.hpp"
#include <plugins/map.hpp>
#include <components.hpp>
#include <events.hpp>
#include <state.hpp>

#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Event.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Plugins/MeshPlugin.hpp>
#include <R-Engine/Plugins/RenderPlugin.hpp>
#include <cmath>
#include <cstdlib>

static void spawn_scenery_system(r::ecs::Commands& commands, r::ecs::ResMut<r::Meshes> meshes, r::ecs::Res<r::Camera3d> camera)
{
    ::Model building_model = r::Mesh3d::Glb("assets/models/BlackBuilding.glb");
    if (building_model.meshCount == 0) {
        r::Logger::error("Cannot load black building model");
        return;
    }
    r::MeshHandle building_handle = meshes.ptr->add(building_model);
    if (building_handle == r::MeshInvalidHandle) {
        r::Logger::error("Failed to load black building mesh.");
        return;
    }

    const float distance_camera = camera.ptr->position.z;
    const float view_height = 2.0f * distance_camera * std::tanf(camera.ptr->fovy * (r::R_PI / 180.0f) / 2.0f);
    const float view_width = view_height * (static_cast<float>(1280) / static_cast<float>(720));

    const float SPAWN_BUFFER_FACTOR = 1.5f;
    const float spawn_area_width = view_width * SPAWN_BUFFER_FACTOR;

    const float building_width = 2.0f;
    const float gap = 1.5f;

    int buildings_in_a_row = 0;
    int max_buildings_in_group = 5;
    int gap_size = 3;

    for (float current_x = -spawn_area_width / 2.0f; current_x < spawn_area_width / 2.0f; current_x += (building_width + gap)) {
        if (buildings_in_a_row < max_buildings_in_group) {
            const float MIN_BUILDING_Y = -25.0f;
            const float Y_VARIATION = 3.0f;
            float random_y = MIN_BUILDING_Y - Y_VARIATION * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));

            commands.spawn(
                BlackBuilding{},
                ScrollingScenery{},
                r::Transform3d{
                    .position = {current_x, random_y, -10.0f},
                    .scale = {2.0f, 2.0f, 2.0f}
                },
                r::Mesh3d{
                    .id = building_handle,
                    .color = r::Color{255, 255, 255, 255},
                    .rotation_offset = {0.0f, static_cast<float>(M_PI) / 2.0f, 0.0f}
                }
            );
            buildings_in_a_row++;
        } else {
            gap_size--;
            if (gap_size <= 0) {
                buildings_in_a_row = 0;
                max_buildings_in_group = 2 + (rand() % 4);
                gap_size = 2 + (rand() % 3);
            }
        }
    }
}


static void spawn_background_system(r::ecs::Commands& commands, r::ecs::ResMut<r::Meshes> meshes, r::ecs::Res<r::Camera3d> camera)
{
    const float BACKGROUND_Z_DEPTH = -20.0f;

    const float effective_distance = camera.ptr->position.z - BACKGROUND_Z_DEPTH;

    const float correct_view_height = 2.0f * effective_distance * std::tanf(camera.ptr->fovy * (r::R_PI / 180.0f) / 2.0f);
    const float correct_view_width = correct_view_height * (static_cast<float>(1280) / static_cast<float>(720));

    ::Mesh background_mesh_data = GenMeshPlane(correct_view_width, correct_view_height, 1, 1);
    if (background_mesh_data.vertexCount == 0) {
        r::Logger::error("Impossible to load background.");
        return;
    }

    r::MeshHandle background_mesh_handle = meshes.ptr->add(background_mesh_data, "assets/textures/background.png");
    
    if (background_mesh_handle == r::MeshInvalidHandle) {
        r::Logger::error("Impossible to load texture : assets/textures/back2.png");
        return;
    }

    commands.spawn(
        Background{},
        r::Transform3d{ 
            .position = {0.0f, 0.0f, BACKGROUND_Z_DEPTH},
            .rotation = {r::R_PI / 2.0f, 0.0f, 0.0f}
        },
        r::Mesh3d{ .id = background_mesh_handle, .color = r::Color{255, 255, 255, 255} }
    );
}

static void follow_camera_background_system(
    r::ecs::Res<r::Camera3d> camera,
    r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::Ref<Background>> query)
{
    if (query.size() == 0) return;

    for (auto [transform, background] : query) {
        transform.ptr->position.x = camera.ptr->position.x;
        transform.ptr->position.y = camera.ptr->position.y;
    }
}

// static void scroll_background_system(
//     r::ecs::Res<r::core::FrameTime> time,
//     r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::Ref<Background>> query)
// {
//     if (query.size() < 2)
//         return;

//     const float bg_width = 20.0f * (static_cast<float>(GetScreenWidth()) / static_cast<float>(GetScreenHeight()));

//     for (auto [transform, background] : query) {
//         transform.ptr->position.x -= background.ptr->scroll_speed * time.ptr->delta_time;

//         if (transform.ptr->position.x <= -bg_width) {
//             transform.ptr->position.x += 2.0f * bg_width;
//         }
//     }
// }


static void scroll_scenery_system(
    r::ecs::Res<r::core::FrameTime> time,
    r::ecs::Res<r::Camera3d> camera,
    r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::Ref<ScrollingScenery>> query)
{
    if (query.size() == 0) return;

    const float distance_camera = camera.ptr->position.z;
    const float view_height = 2.0f * distance_camera * std::tanf(camera.ptr->fovy * (r::R_PI / 180.0f) / 2.0f);
    const float view_width = view_height * (static_cast<float>(1880) / static_cast<float>(720));
    
    //const float building_width = 3.0f;
    //const float gap = 1.5f;
    //const float step_width = building_width + gap;

    //const int num_steps_needed = static_cast<int>(std::ceil((view_width + building_width) / step_width)) + 1;
    //const float total_scrolling_width = static_cast<float>(num_steps_needed) * step_width;

    const float SCROLL_BUFFER_FACTOR = 1.5f;
    const float scroll_area_width = view_width * SCROLL_BUFFER_FACTOR;

    const float offscreen_limit = camera.ptr->position.x - (scroll_area_width / 2.0f);

    for (auto [transform, scenery] : query) {
        transform.ptr->position.x -= scenery.ptr->scroll_speed * time.ptr->delta_time;

        if (transform.ptr->position.x < offscreen_limit) {
            transform.ptr->position.x += scroll_area_width;
            const float MIN_BUILDING_Y = -25.0f;
            const float Y_VARIATION = 3.0f;
            transform.ptr->position.y = MIN_BUILDING_Y - Y_VARIATION * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
        }
    }
}

static void cleanup_map_system(r::ecs::Commands& commands, 
    r::ecs::Query<r::ecs::With<Background>> background_query,
    r::ecs::Query<r::ecs::With<ScrollingScenery>> scenery_query)
{
    for (auto it = background_query.begin(); it != background_query.end(); ++it) {
        commands.despawn(it.entity());
    }
    for (auto it = scenery_query.begin(); it != scenery_query.end(); ++it) {
        commands.despawn(it.entity());
    }
}

void MapPlugin::build(r::Application& app)
{
    app.add_systems<spawn_scenery_system>(r::OnEnter{GameState::MainMenu})
        .add_systems<spawn_background_system>(r::OnEnter{GameState::MainMenu})

        .add_systems<follow_camera_background_system, scroll_scenery_system>(r::Schedule::UPDATE)
            .run_if<r::run_conditions::in_state<GameState::MainMenu>>()
            .run_or<r::run_conditions::in_state<GameState::EnemiesBattle>>()
            .run_or<r::run_conditions::in_state<GameState::BossBattle>>()

        .add_systems<cleanup_map_system>(r::OnExit{GameState::MainMenu})

        .add_systems<spawn_scenery_system>(r::OnEnter{GameState::EnemiesBattle})
        .add_systems<spawn_background_system>(r::OnEnter{GameState::EnemiesBattle});
}