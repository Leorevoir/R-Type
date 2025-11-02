#pragma once

/**
 * @brief Fired by debug systems to request a direct switch to a specific level.
 */
struct DebugSwitchLevelEvent {
        int level_index = 0;
};
