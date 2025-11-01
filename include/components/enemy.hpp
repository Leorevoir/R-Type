#pragma once

#include "R-Engine/Maths/Vec.hpp"

/* -- Enemy Marker Components -- */

struct Enemy {
};
struct Boss {
};

/* -- Enemy Behavior Components -- */

struct SineWaveEnemy {
        float angle = 0.0f;
        float amplitude = 1.5f;
        float frequency = 3.0f;
};

struct HomingEnemy {
        float turn_speed = 1.5f; /* Controls how quickly the enemy can turn towards the player */
};

/* -- Boss Behavior "Tag" Components -- */

struct VerticalPatrolBoss {
};
struct HomingAttackBoss {
        enum class State {
            Entering,
            Repositioning,
            Attacking,
        };

        State current_state = State::Entering;
        float state_timer = 0.0f;///< Timer to control how long each state lasts
        r::Vec3f target_position;///< The position the boss is trying to move to
};
struct TurretBoss {
};
