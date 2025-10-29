#pragma once

struct PlayerBullet {
};

struct WaveCannonBeam {
        float charge_level = 0.0f;
        int damage = 1;
};

struct EnemyBullet {
};

/**
 * @brief This component marks a projectile as unblockable by the Force shield.
 */
struct Unblockable {
};
