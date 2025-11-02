#pragma once

#include <R-Engine/Core/States.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <resources/game_mode.hpp>
#include <state/game_state.hpp>

namespace run_conditions {

/**
 * @brief Run condition that returns true if the game is transitioning from the Paused state.
 * @details This allows us to prevent level-resetting systems from running on resume.
 *
 * Implementation lives in the source file `src/state/run_conditions.cpp`.
 */
bool is_resuming_from_pause(r::ecs::Res<r::State<GameState>> state);

/**
 * @brief Run condition that returns true if the game is in Online mode.
 * @details Used to enable networking-specific systems.
 */
bool is_online_mode(r::ecs::Res<GameMode> mode);

}// namespace run_conditions
