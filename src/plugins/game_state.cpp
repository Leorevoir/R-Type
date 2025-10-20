#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/ECS/Event.hpp>
#include <R-Engine/ECS/RunConditions.hpp>
#include <events.hpp>
#include <plugins/game_state.hpp>
#include <state.hpp>

static void handle_player_death_system(r::ecs::ResMut<r::NextState<GameState>> next_state)
{
    r::Logger::warn("PlayerDiedEvent received! Game Over.");
    next_state.ptr->set(GameState::GameOver);
}

static void handle_boss_spawn_trigger_system(r::ecs::ResMut<r::NextState<GameState>> next_state)
{
    r::Logger::info("BossTimeReachedEvent received! Transitioning to BossBattle.");
    next_state.ptr->set(GameState::BossBattle);
}

static void handle_boss_defeated_system()
{
    /* Later, we'll of course transition to a "You Win" screen or the next level */
    r::Logger::info("BossDefeatedEvent received! Congratulations!");
}

void GameStatePlugin::build(r::Application &app)
{
    app.init_state(GameState::MainMenu)
        .add_systems<handle_player_death_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::on_event<PlayerDiedEvent>>()

        .add_systems<handle_boss_spawn_trigger_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::on_event<BossTimeReachedEvent>>()

        .add_systems<handle_boss_defeated_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::on_event<BossDefeatedEvent>>();
}
