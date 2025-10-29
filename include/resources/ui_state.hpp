#pragma once

#include <state/game_state.hpp>

struct PreviousGameState {
        GameState state = GameState::MainMenu;
};

struct StateBeforePause {
        GameState state = GameState::EnemiesBattle;
};
