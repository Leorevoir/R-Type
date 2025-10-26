#pragma once

#include <R-Engine/Plugins/MeshPlugin.hpp>
#include <string>
#include <vector>

/* ================================================================================= */
/* Level Configuration */
/* ================================================================================= */

enum class EnemyBehaviorType {
    Straight,
    SineWave,
    Homing,
};

struct EnemyData {
        std::string model_path;
        int health;
        float speed;
        EnemyBehaviorType behavior;
};

enum class BossBehaviorType {
    VerticalPatrol,
    HomingAttack,
    Turret,
};

struct BossData {
        std::string model_path;
        int max_health;
        BossBehaviorType behavior;
};

struct LevelData {
        int id;
        float enemy_spawn_interval;
        float boss_spawn_time;
        std::string background_texture_path;
        std::string scenery_model_path;
        std::vector<EnemyData> enemy_types;
        BossData boss_data;
};

struct GameLevels {
        std::vector<LevelData> levels;
};

struct CurrentLevel {
        int index = 0;
};

/* ================================================================================= */
/* Resources */
/* Resources are global data structures. */
/* ================================================================================= */

struct EnemySpawnTimer {
        float time_left = 1.0f; /* Default value, will be overwritten by level data */
};

struct BossSpawnTimer {
        float time_left = 4.0f; /* Default value, will be overwritten by level data */
        bool spawned = false;
};

struct BossShootTimer {
        float time_left = 2.0f;
        static constexpr float FIRE_RATE = 2.0f;
};

struct PlayerBulletAssets {
        r::MeshHandle laser_beam_handle = r::MeshInvalidHandle;
        r::MeshHandle force_missile = r::MeshInvalidHandle;
};

struct BossBulletAssets {
        r::MeshHandle big_missile = r::MeshInvalidHandle;
        r::MeshHandle small_missile = r::MeshInvalidHandle;
};
