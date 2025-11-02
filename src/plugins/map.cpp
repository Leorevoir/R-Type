#include "plugins/map.hpp"
#include "R-Engine/Components/Transform3d.hpp"
#include "R-Engine/Maths/Maths.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Plugins/MeshPlugin.hpp>
#include <R-Engine/Plugins/RenderPlugin.hpp>
#include <cmath>
#include <cstdlib>

#include <components/map.hpp>
#include <resources/level.hpp>
#include <state/game_state.hpp>
#include <state/run_conditions.hpp>

static float random_float(float min, float max)
{
    if (min > max)
        std::swap(min, max);
    return min + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (max - min);
}

static void asteroid_field_system(r::ecs::Res<r::core::FrameTime> time, r::ecs::Res<r::Camera3d> camera,
    r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::Ref<Asteroid>> query)
{
    if (query.size() == 0)
        return;

    const float distance_camera = camera.ptr->position.z;
    const float view_height = 2.0f * distance_camera * tanf(camera.ptr->fovy * (r::R_PI / 180.0f) / 2.0f);
    const float view_width = view_height * (static_cast<float>(1280) / static_cast<float>(720));

    const float SCROLL_BUFFER_FACTOR = 2.0f;
    const float scroll_area_width = view_width * SCROLL_BUFFER_FACTOR;
    const float scroll_area_height = view_height * SCROLL_BUFFER_FACTOR;

    const float offscreen_limit_left = camera.ptr->position.x - (scroll_area_width / 2.0f);
    const float offscreen_limit_right = camera.ptr->position.x + (scroll_area_width / 2.0f);
    const float offscreen_limit_top = camera.ptr->position.y + (scroll_area_height / 2.0f);
    const float offscreen_limit_bottom = camera.ptr->position.y - (scroll_area_height / 2.0f);

    for (auto [transform, asteroid] : query) {
        transform.ptr->position += asteroid.ptr->velocity * time.ptr->delta_time;

        transform.ptr->rotation.x += asteroid.ptr->rotation_speed.x * time.ptr->delta_time;
        transform.ptr->rotation.y += asteroid.ptr->rotation_speed.y * time.ptr->delta_time;
        transform.ptr->rotation.z += asteroid.ptr->rotation_speed.z * time.ptr->delta_time;

        if (transform.ptr->position.x < offscreen_limit_left) {
            transform.ptr->position.x = offscreen_limit_right;
            transform.ptr->position.y = random_float(offscreen_limit_bottom, offscreen_limit_top);
            transform.ptr->position.z = random_float(-18.0f, -5.0f);
        }

        if (transform.ptr->position.y > offscreen_limit_top) {
            transform.ptr->position.y = offscreen_limit_bottom;
        } else if (transform.ptr->position.y < offscreen_limit_bottom) {
            transform.ptr->position.y = offscreen_limit_top;
        }
    }
}

static void spawn_scenery_system(r::ecs::Commands &commands, r::ecs::ResMut<r::Meshes> meshes, r::ecs::Res<r::Camera3d> camera,
    r::ecs::Res<CurrentLevel> current_level, r::ecs::Res<GameLevels> game_levels)
{
    const auto &level_data = game_levels.ptr->levels[static_cast<size_t>(current_level.ptr->index)];

    r::MeshHandle scenery_handle = meshes.ptr->add(level_data.scenery_model_path);
    if (scenery_handle == r::MeshInvalidHandle) {
        r::Logger::error("Failed to queue scenery mesh for loading: " + level_data.scenery_model_path);
        return;
    }

    const float distance_camera = camera.ptr->position.z;
    const float view_height = 2.0f * distance_camera * tanf(camera.ptr->fovy * (r::R_PI / 180.0f) / 2.0f);
    const float view_width = view_height * (static_cast<float>(1280) / static_cast<float>(720));

    const float SPAWN_BUFFER_FACTOR = 1.5f;
    const float spawn_area_width = view_width * SPAWN_BUFFER_FACTOR;

    if (current_level.ptr->index == 1) {
        const int num_asteroids = 20;
        for (int i = 0; i < num_asteroids; ++i) {
            float x = random_float(-spawn_area_width / 2.0f, spawn_area_width / 2.0f);
            float y = random_float(-view_height, view_height);
            float z = random_float(-18.0f, -5.0f);
            float scale = random_float(0.5f, 1.5f);

            float speed_factor = (z + 15.0f) / 20.0f;
            float base_speed = -2.0f;
            float scroll_speed = base_speed - (speed_factor * 4.0f);
            float y_velocity = random_float(-0.5f, 0.5f);

            commands.spawn(
                Asteroid{
                    .velocity = {scroll_speed, y_velocity, 0.0f},
                    .rotation_speed = {random_float(-1.f, 1.f), random_float(-1.f, 1.f), random_float(-1.f, 1.f)},
                },
                r::Transform3d{
                    .position = {x, y, z},
                    .rotation = {random_float(0.f, 2.f * r::R_PI), random_float(0.f, 2.f * r::R_PI), random_float(0.f, 2.f * r::R_PI)},
                    .scale = {scale, scale, scale},
                },
                r::Mesh3d{
                    .id = scenery_handle,
                    .color = r::Color{255, 255, 255, 255},
                    .rotation_offset = {0.0f, static_cast<float>(M_PI) / 2.0f, 0.0f},
                });
        }
    } else {
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

                commands.spawn(ScrollingScenery{}, r::Transform3d{.position = {current_x, random_y, -10.0f}, .scale = {2.0f, 2.0f, 2.0f}},
                    r::Mesh3d{.id = scenery_handle,
                        .color = r::Color{255, 255, 255, 255},
                        .rotation_offset = {0.0f, static_cast<float>(M_PI) / 2.0f, 0.0f}});
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
}

static void spawn_background_system(r::ecs::Commands &commands, r::ecs::ResMut<r::Meshes> meshes, r::ecs::Res<r::Camera3d> camera,
    r::ecs::Res<CurrentLevel> current_level, r::ecs::Res<GameLevels> game_levels)
{
    r::Logger::info("spawn_background_system: Running.");
    const auto &level_data = game_levels.ptr->levels[static_cast<size_t>(current_level.ptr->index)];
    const float BACKGROUND_Z_DEPTH = -20.0f;

    const float effective_distance = camera.ptr->position.z - BACKGROUND_Z_DEPTH;

    const float correct_view_height = 2.0f * effective_distance * tanf(camera.ptr->fovy * (r::R_PI / 180.0f) / 2.0f);
    const float correct_view_width = correct_view_height * (static_cast<float>(1280) / static_cast<float>(720));

    ::Mesh background_mesh_data = GenMeshPlane(correct_view_width, correct_view_height, 1, 1);
    r::Logger::info("spawn_background_system: Queuing background mesh with texture: " + level_data.background_texture_path);
    r::MeshHandle background_mesh_handle = meshes.ptr->add(std::move(background_mesh_data), level_data.background_texture_path);

    if (background_mesh_handle == r::MeshInvalidHandle) {
        r::Logger::error("Impossible to load texture: " + level_data.background_texture_path);
        return;
    }

    commands.spawn(Background{}, r::Transform3d{.position = {0.0f, 0.0f, BACKGROUND_Z_DEPTH}, .rotation = {r::R_PI / 2.0f, 0.0f, 0.0f}},
        r::Mesh3d{.id = background_mesh_handle, .color = r::Color{255, 255, 255, 255}});
}

static void follow_camera_background_system(r::ecs::Res<r::Camera3d> camera,
    r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::Ref<Background>> query)
{
    if (query.size() == 0)
        return;

    for (auto [transform, background] : query) {
        transform.ptr->position.x = camera.ptr->position.x;
        transform.ptr->position.y = camera.ptr->position.y;
    }
}

static void scroll_scenery_system(r::ecs::Res<r::core::FrameTime> time, r::ecs::Res<r::Camera3d> camera,
    r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::Ref<ScrollingScenery>> query)
{
    if (query.size() == 0)
        return;

    const float distance_camera = camera.ptr->position.z;
    const float view_height = 2.0f * distance_camera * tanf(camera.ptr->fovy * (r::R_PI / 180.0f) / 2.0f);
    const float view_width = view_height * (static_cast<float>(1880) / static_cast<float>(720));

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

static void cleanup_map_system(r::ecs::Commands &commands, r::ecs::Query<r::ecs::With<Background>> background_query,
    r::ecs::Query<r::ecs::With<ScrollingScenery>> scenery_query, r::ecs::Query<r::ecs::With<Asteroid>> asteroid_query)
{
    for (auto it = background_query.begin(); it != background_query.end(); ++it) {
        commands.despawn(it.entity());
    }
    for (auto it = scenery_query.begin(); it != scenery_query.end(); ++it) {
        commands.despawn(it.entity());
    }
    for (auto it = asteroid_query.begin(); it != asteroid_query.end(); ++it) {
        commands.despawn(it.entity());
    }
}

void MapPlugin::build(r::Application &app)
{
    app.add_systems<cleanup_map_system>(r::OnEnter{GameState::MainMenu})
        .add_systems<cleanup_map_system>(r::OnEnter{GameState::EnemiesBattle})
        .run_unless<run_conditions::is_resuming_from_pause>()

        .add_systems<spawn_scenery_system>(r::OnEnter{GameState::MainMenu})
        .add_systems<spawn_background_system>(r::OnEnter{GameState::MainMenu})

        .add_systems<follow_camera_background_system, scroll_scenery_system, asteroid_field_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::MainMenu>>()
        .run_or<r::run_conditions::in_state<GameState::EnemiesBattle>>()
        .run_or<r::run_conditions::in_state<GameState::BossBattle>>()

        .add_systems<spawn_scenery_system>(r::OnEnter{GameState::EnemiesBattle})
        .run_unless<run_conditions::is_resuming_from_pause>()
        .add_systems<spawn_background_system>(r::OnEnter{GameState::EnemiesBattle})
        .run_unless<run_conditions::is_resuming_from_pause>();
}
