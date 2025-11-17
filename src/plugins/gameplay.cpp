#include "plugins/gameplay.hpp"
#include "R-Engine/Components/Transform3d.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/FrameTime.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Event.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <R-Engine/Plugins/MeshPlugin.hpp>
#include <R-Engine/Plugins/AudioPlugin.hpp>
#include <R-Engine/Core/Filepath.hpp>
#include <string>

#include <components/common.hpp>
#include <events/game_events.hpp>
#include <resources/assets.hpp>
#include <resources/game_state.hpp>
#include <resources/level.hpp>
#include <state/game_state.hpp>
#include <state/run_conditions.hpp>

/* ================================================================================= */
/* Gameplay Systems */
/* ================================================================================= */

static void scoring_system(r::ecs::EventReader<EntityDiedEvent> reader, r::ecs::Query<r::ecs::Ref<ScoreValue>> query,
    r::ecs::ResMut<PlayerScore> score)
{
    for (const auto &event : reader) {
        for (auto it = query.begin(); it != query.end(); ++it) {
            if (it.entity() == event.entity) {
                auto [score_value] = *it;
                score.ptr->value += score_value.ptr->points;
                r::Logger::info("Score: " + std::to_string(score.ptr->value));
                break;
            }
        }
    }
}

static void setup_level_timers_system(r::ecs::Res<CurrentLevel> current_level, r::ecs::Res<GameLevels> game_levels,
    r::ecs::ResMut<EnemySpawnTimer> enemy_timer, r::ecs::ResMut<BossSpawnTimer> boss_timer)
{
    const auto &level_data = game_levels.ptr->levels[static_cast<size_t>(current_level.ptr->index)];
    enemy_timer.ptr->time_left = level_data.enemy_spawn_interval;
    boss_timer.ptr->time_left = level_data.boss_spawn_time;
    boss_timer.ptr->spawned = false;
    r::Logger::info("Setting up timers for level " + std::to_string(level_data.id));
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

static void movement_system(r::ecs::Res<r::core::FrameTime> time, r::ecs::Query<r::ecs::Mut<r::Transform3d>, r::ecs::Ref<Velocity>> query)
{
    for (auto [transform, velocity] : query) {
        transform.ptr->position = transform.ptr->position + velocity.ptr->value * time.ptr->delta_time;
    }
}

static void setup_missile_assets_system(r::ecs::Commands &commands, r::ecs::ResMut<r::Meshes> meshes)
{
    BossBulletAssets bullet_assets;

    bullet_assets.big_missile = meshes.ptr->add("assets/models/BigMissiles.glb");
    if (bullet_assets.big_missile == r::MeshInvalidHandle) {
        r::Logger::error("Failed to queue big missile model !");
    }

    bullet_assets.small_missile = meshes.ptr->add("assets/models/BossRegularMissile.glb");
    if (bullet_assets.small_missile == r::MeshInvalidHandle) {
        r::Logger::error("Failed to queue regular boss missile model !");
    }

    commands.insert_resource(bullet_assets);
}

/* Tag component for background music entity so we can stop/remove it on exit */
struct BackgroundMusicTag {};

static void setup_background_music_system(r::ecs::Commands &commands, r::ecs::ResMut<r::AudioManager> audio,
    r::ecs::Query<r::ecs::With<BackgroundMusicTag>> existing)
{
    /* If a background music entity already exists, don't spawn another one. */
    if (existing.size() > 0) {
        return;
    }

    const std::string &path = r::path::get("assets/sounds/theme.mp3");
    const auto handle = audio.ptr->load(path);
    if (handle == r::AudioInvalidHandle) {
        r::Logger::warn("Failed to load " + path);
        return;
    }
    r::Logger::info(std::string{"Background music handle="} + std::to_string(handle));

    /* Spawn a persistent music entity with AudioPlayer + AudioSink to play the theme */
    commands.spawn(BackgroundMusicTag{}, r::AudioPlayer{handle}, r::AudioSink{});
}

static void pause_background_music_system(r::ecs::Query<r::ecs::Mut<r::AudioSink>, r::ecs::With<BackgroundMusicTag>> query)
{
    for (auto [sink, _] : query) {
        r::Logger::info("Pausing background music (AudioSink) - was_playing=" + std::to_string(sink.ptr->is_playing()));
        sink.ptr->pause();
        r::Logger::info("Background music paused - is_playing=" + std::to_string(sink.ptr->is_playing()));
    }
}

static void resume_background_music_system(r::ecs::Query<r::ecs::Mut<r::AudioSink>, r::ecs::With<BackgroundMusicTag>> query)
{
    for (auto [sink, _] : query) {
        r::Logger::info("Resuming background music (AudioSink) - was_playing=" + std::to_string(sink.ptr->is_playing()));
        sink.ptr->play();
        r::Logger::info("Background music resumed - is_playing=" + std::to_string(sink.ptr->is_playing()));
    }
}

void GameplayPlugin::build(r::Application &app)
{
    app.insert_resource(EnemySpawnTimer{})
        .insert_resource(BossSpawnTimer{})

        .add_systems<movement_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()
        .run_or<r::run_conditions::in_state<GameState::BossBattle>>()
    .add_systems<setup_missile_assets_system>(r::OnEnter{GameState::EnemiesBattle})
    .run_unless<run_conditions::is_resuming_from_pause>()

    /* Background music: load & play on entering gameplay, pause on pause/menu, resume on re-entering gameplay */
    .add_systems<setup_background_music_system>(r::OnEnter{GameState::EnemiesBattle})
    .run_unless<run_conditions::is_resuming_from_pause>()
    /* Resume music should run even when resuming from pause */
    .add_systems<resume_background_music_system>(r::OnEnter{GameState::EnemiesBattle})
    .add_systems<pause_background_music_system>(r::OnEnter{GameState::Paused})
    .add_systems<pause_background_music_system>(r::OnEnter{GameState::MainMenu})
    .add_systems<pause_background_music_system>(r::OnEnter{GameState::SettingsMenu})

        .add_systems<setup_level_timers_system>(r::OnEnter{GameState::EnemiesBattle})
        .run_unless<run_conditions::is_resuming_from_pause>()

        .add_systems<setup_boss_fight_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()

        .add_systems<scoring_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::on_event<EntityDiedEvent>>();
}
