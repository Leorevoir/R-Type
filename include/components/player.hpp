#pragma once

#include <R-Engine/ECS/Entity.hpp>

struct Player {
        r::ecs::Entity force_entity = r::ecs::NULL_ENTITY;
        float force_cooldown = 0.f;
        float wave_cannon_charge_timer = 0.0f;
};

struct Force {
        bool is_attached = true;
        bool is_front_attachment = true; /* true = front, false = rear */
        r::ecs::Entity owner = r::ecs::NULL_ENTITY;
};

struct FireCooldown {
        float timer = 0.0f;
};
