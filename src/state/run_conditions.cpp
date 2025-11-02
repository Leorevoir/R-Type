#include <state/run_conditions.hpp>

namespace run_conditions {

/**
 * @brief Run condition that returns true if the game is transitioning from the Paused state.
 * @details This allows us to prevent level-resetting systems from running on resume.
 *
 * Implementation separated from the header to keep `.hpp` declaration-only.
 */
bool is_resuming_from_pause(r::ecs::Res<r::State<GameState>> state)
{
    return state.ptr && state.ptr->previous().has_value() && state.ptr->previous().value() == GameState::Paused;
}

} // namespace run_conditions