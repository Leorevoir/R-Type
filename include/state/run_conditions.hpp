#pragma once

#include <R-Engine/Core/States.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <state/game_state.hpp>

namespace run_conditions {

/**
 * @brief Run condition that returns true if the game is transitioning from the Paused state.
 * @details This allows us to prevent level-resetting systems from running on resume.
 *
 * Implementation lives in the source file `src/state/run_conditions.cpp`.
 */
bool is_resuming_from_pause(r::ecs::Res<r::State<GameState>> state);

} // namespace run_conditions
