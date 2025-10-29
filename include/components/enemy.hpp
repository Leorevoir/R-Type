#pragma once

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
};
struct TurretBoss {
};
