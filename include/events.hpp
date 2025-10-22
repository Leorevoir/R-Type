#pragma once

#include <R-Engine/ECS/Entity.hpp>
#include <R-Engine/Maths/Vec.hpp>

/**
 * @brief Fired when the player's ship is destroyed.
 */
struct PlayerDiedEvent {
};

/**
 * @brief Fired when the timer for the boss fight is reached.
 */
struct BossTimeReachedEvent {
};

/**
 * @brief Fired when the boss's health reaches zero.
 */
struct BossDefeatedEvent {
};

/**
 * @brief A generic event fired when an entity is destroyed by combat actions.
 * @details Can be used for scoring, special effects, sound, etc.
 */
struct EntityDiedEvent {
        r::ecs::Entity entity;
};

/**
 * @brief Fired when a UI element is clicked.
 * @details This bridges the engine's custom UI input state to the standard event system.
 */
// struct UiClickEvent {
//         r::ecs::Entity entity;
// };
