#pragma once

#include <R-Engine/Maths/Vec.hpp>

/**
 * @brief Represents the velocity of an entity.
 * @details The movement_system in GameplayPlugin uses this to update positions.
 */
struct Velocity {
        r::Vec3f value;
};

/**
 * @brief Defines a spherical collider for collision detection.
 */
struct Collider {
        float radius;
        r::Vec3f offset = {0.0f, 0.0f, 0.0f};
};

/**
 * @brief Manages the health of an entity.
 */
struct Health {
        int current;
        int max;
};

/**
 * @brief The number of points an entity is worth when destroyed.
 */
struct ScoreValue {
        int points = 0;
};
