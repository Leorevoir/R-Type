#include "plugins/game_state.hpp"
#include "resources.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Event.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <components.hpp>
#include <events.hpp>
#include <state.hpp>
#include <string>

static void handle_player_death_system(r::ecs::ResMut<r::NextState<GameState>> next_state, r::ecs::ResMut<PlayerLives> lives,
    r::ecs::Commands &commands, r::ecs::Query<r::ecs::With<Player>> player_query)
{
    lives.ptr->count--;
    r::Logger::info("Player died. Lives remaining: " + std::to_string(lives.ptr->count));

    /* Despawn the player entity. Its children (like the Force) will be despawned automatically. */
    for (auto it = player_query.begin(); it != player_query.end(); ++it) {
        commands.despawn(it.entity());
    }

    if (lives.ptr->count > 0) {
        r::Logger::info("Restarting level...");
        next_state.ptr->set(GameState::EnemiesBattle);
    } else {
        r::Logger::warn("No lives remaining! Game Over.");
        next_state.ptr->set(GameState::GameOver);
    }
}

static void handle_boss_spawn_trigger_system(r::ecs::ResMut<r::NextState<GameState>> next_state)
{
    r::Logger::info("BossTimeReachedEvent received! Transitioning to BossBattle.");
    next_state.ptr->set(GameState::BossBattle);
}

static void handle_boss_defeated_system(r::ecs::ResMut<r::NextState<GameState>> next_state, r::ecs::ResMut<CurrentLevel> current_level,
    r::ecs::Res<GameLevels> game_levels)
{
    r::Logger::info("BossDefeatedEvent received! Level " + std::to_string(current_level.ptr->index + 1) + " complete!");

    current_level.ptr->index++;

    if (static_cast<size_t>(current_level.ptr->index) < game_levels.ptr->levels.size()) {
        r::Logger::info("Proceeding to Level " + std::to_string(current_level.ptr->index + 1));
        next_state.ptr->set(GameState::EnemiesBattle);
    } else {
        r::Logger::info("All levels completed! Congratulations!");
        next_state.ptr->set(GameState::YouWin);
    }
}

static void reset_player_lives_system(r::ecs::ResMut<PlayerLives> lives)
{
    lives.ptr->count = 3;
}

void GameStatePlugin::build(r::Application &app)
{
    app.init_state(GameState::MainMenu)
        .insert_resource(PlayerLives{})
        .add_systems<reset_player_lives_system>(r::OnEnter{GameState::EnemiesBattle})
        .add_systems<handle_player_death_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::on_event<PlayerDiedEvent>>()

        .add_systems<handle_boss_spawn_trigger_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::on_event<BossTimeReachedEvent>>()

        .add_systems<handle_boss_defeated_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::on_event<BossDefeatedEvent>>();
}
