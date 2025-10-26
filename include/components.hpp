#pragma once

#include <R-Engine/ECS/Entity.hpp>
#include <R-Engine/Maths/Vec.hpp>

/* ================================================================================= */
/* Menu Components */
/* ================================================================================= */

struct MenuButton {
        enum class Action { None, Play, Options, Quit };
        Action action = Action::None;
};

struct MenuRoot {
};
struct GameOverRoot {
};
struct YouWinRoot {
};

/* ================================================================================= */
/* Game Components */
/* Components are simple data structures that define the properties of an entity. */
/* ================================================================================= */

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
struct Boss {
};
struct Enemy {
};
struct PlayerBullet {
};
struct WaveCannonBeam {
        float charge_level = 0.0f;
        int damage = 1;
};
struct EnemyBullet {
};

struct FireCooldown {
        float timer = 0.0f;
};

struct Velocity {
        r::Vec3f value;
};

struct Collider {
        float radius;
        r::Vec3f offset = {0.0f, 0.0f, 0.0f};
};

struct Health {
        int current;
        int max;
};

struct BlackBuilding {
};

struct Background {
        float scroll_speed = 2.0f;
};

struct ScrollingScenery {
        float scroll_speed = 4.0f;
};

/* ================================================================================= */
/* Boss Behavior Components */
/* These are "tag" components used to assign specific behaviors to a boss entity. */
/* ================================================================================= */

struct VerticalPatrolBoss {
};
struct HomingAttackBoss {
};
struct TurretBoss {
};
