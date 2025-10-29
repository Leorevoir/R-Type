#include "plugins/combat.hpp"
#include "R-Engine/Components/Transform3d.hpp"
#include <R-Engine/Application.hpp>
#include <R-Engine/Core/Logger.hpp>
#include <R-Engine/Core/States.hpp>
#include <R-Engine/ECS/Command.hpp>
#include <R-Engine/ECS/Event.hpp>
#include <R-Engine/ECS/Query.hpp>
#include <R-Engine/ECS/RunConditions.hpp>

#include <components/common.hpp>
#include <components/enemy.hpp>
#include <components/player.hpp>
#include <components/projectiles.hpp>
#include <events/game_events.hpp>
#include <resources/level.hpp>
#include <state/game_state.hpp>
#include <state/run_conditions.hpp>

/* ================================================================================= */
/* Event Handlers */
/* ================================================================================= */

/**
 * @brief Listens for EntityDiedEvent and despawns the corresponding entity.
 * @details This decouples the act of destroying an entity from the logic that decides it should be destroyed.
 */
static void handle_entity_death(r::ecs::Commands &commands, r::ecs::EventReader<EntityDiedEvent> reader)
{
    for (const auto &event : reader) {
        commands.despawn(event.entity);
    }
}

/* ================================================================================= */
/* Combat Systems :: Helpers */
/* ================================================================================= */

static bool process_bullet_enemy_collision(r::ecs::EventWriter<EntityDiedEvent> &entity_death_writer, r::ecs::Entity bullet_entity,
    const r::Vec3f &bullet_center, float bullet_radius,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::Mut<Health>, r::ecs::With<Enemy>> &enemy_query)
{
    for (auto enemy_it = enemy_query.begin(); enemy_it != enemy_query.end(); ++enemy_it) {
        auto [enemy_transform, enemy_collider, health, _e] = *enemy_it;
        r::Vec3f enemy_center = enemy_transform.ptr->position + enemy_collider.ptr->offset;
        float distance = (bullet_center - enemy_center).length();
        float radii_sum = bullet_radius + enemy_collider.ptr->radius;

        if (distance < radii_sum) {
            entity_death_writer.send({bullet_entity});
            health.ptr->current -= 1;

            if (health.ptr->current <= 0) {
                entity_death_writer.send({enemy_it.entity()});
            }
            return true; /* Bullet hit an enemy, processed */
        }
    }
    return false; /* No collision occurred */
}

static void handle_bullet_vs_enemy_collisions(r::ecs::EventWriter<EntityDiedEvent> &entity_death_writer,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::With<PlayerBullet>> &bullet_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::Mut<Health>, r::ecs::With<Enemy>> &enemy_query)
{
    for (auto bullet_it = bullet_query.begin(); bullet_it != bullet_query.end(); ++bullet_it) {
        auto [bullet_transform, bullet_collider, _b] = *bullet_it;
        r::Vec3f bullet_center = bullet_transform.ptr->position + bullet_collider.ptr->offset;

        process_bullet_enemy_collision(entity_death_writer, bullet_it.entity(), bullet_center, bullet_collider.ptr->radius, enemy_query);
    }
}

static void handle_bullet_vs_boss_collisions(r::ecs::EventWriter<EntityDiedEvent> &entity_death_writer,
    r::ecs::EventWriter<BossDefeatedEvent> &boss_death_writer,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::With<PlayerBullet>> &bullet_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::Mut<Health>, r::ecs::With<Boss>> &boss_query)
{
    for (auto bullet_it = bullet_query.begin(); bullet_it != bullet_query.end(); ++bullet_it) {
        auto [bullet_transform, bullet_collider, _b] = *bullet_it;
        for (auto boss_it = boss_query.begin(); boss_it != boss_query.end(); ++boss_it) {
            auto [boss_transform, boss_collider, health, _boss] = *boss_it;
            r::Vec3f bullet_center = bullet_transform.ptr->position + bullet_collider.ptr->offset;
            r::Vec3f boss_center = boss_transform.ptr->position + boss_collider.ptr->offset;
            float distance = (bullet_center - boss_center).length();
            float radii_sum = bullet_collider.ptr->radius + boss_collider.ptr->radius;

            if (distance < radii_sum) {
                entity_death_writer.send({bullet_it.entity()});
                health.ptr->current -= 1;

                if (health.ptr->current <= 0) {
                    entity_death_writer.send({boss_it.entity()});
                    boss_death_writer.send({});
                }
                break; /* A bullet can only hit one boss */
            }
        }
    }
}

static bool process_beam_boss_collision(r::ecs::EventWriter<EntityDiedEvent> &entity_death_writer,
    r::ecs::EventWriter<BossDefeatedEvent> &boss_death_writer, r::ecs::Entity beam_entity, const r::Vec3f &beam_center, float beam_radius,
    int beam_damage, r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::Mut<Health>, r::ecs::With<Boss>> &boss_query)
{
    for (auto boss_it = boss_query.begin(); boss_it != boss_query.end(); ++boss_it) {
        auto [boss_transform, boss_collider, health, _boss] = *boss_it;
        r::Vec3f boss_center = boss_transform.ptr->position + boss_collider.ptr->offset;
        if ((beam_center - boss_center).length() < beam_radius + boss_collider.ptr->radius) {
            health.ptr->current -= beam_damage;
            entity_death_writer.send({beam_entity});

            if (health.ptr->current <= 0) {
                entity_death_writer.send({boss_it.entity()});
                boss_death_writer.send({});
            }
            return true; /* Beam destroyed upon hitting boss */
        }
    }
    return false; /* No boss collision */
}

static void handle_beam_collisions(r::ecs::EventWriter<EntityDiedEvent> &entity_death_writer,
    r::ecs::EventWriter<BossDefeatedEvent> &boss_death_writer,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::Ref<WaveCannonBeam>> &beam_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::Mut<Health>, r::ecs::With<Enemy>> &enemy_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::Mut<Health>, r::ecs::With<Boss>> &boss_query)
{
    for (auto beam_it = beam_query.begin(); beam_it != beam_query.end(); ++beam_it) {
        auto [beam_transform, beam_collider, beam] = *beam_it;
        r::Vec3f beam_center = beam_transform.ptr->position + beam_collider.ptr->offset;

        /* Collision with enemies (penetrating) */
        for (auto enemy_it = enemy_query.begin(); enemy_it != enemy_query.end(); ++enemy_it) {
            auto [enemy_transform, enemy_collider, health, _e] = *enemy_it;
            r::Vec3f enemy_center = enemy_transform.ptr->position + enemy_collider.ptr->offset;
            if ((beam_center - enemy_center).length() < beam_collider.ptr->radius + enemy_collider.ptr->radius) {
                /* Beam is powerful, for now it one-shots regular enemies */
                entity_death_writer.send({enemy_it.entity()});
            }
        }

        /* Collision with boss (not penetrating) */
        process_beam_boss_collision(entity_death_writer, boss_death_writer, beam_it.entity(), beam_center, beam_collider.ptr->radius,
            beam.ptr->damage, boss_query);
    }
}

/* ================================================================================= */
/* Combat Systems */
/* ================================================================================= */

static void collision_system(r::ecs::EventWriter<EntityDiedEvent> entity_death_writer,
    r::ecs::EventWriter<BossDefeatedEvent> boss_death_writer,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::With<PlayerBullet>> bullet_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::Ref<WaveCannonBeam>> beam_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::Mut<Health>, r::ecs::With<Enemy>> enemy_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::Mut<Health>, r::ecs::With<Boss>> boss_query)
{
    handle_bullet_vs_enemy_collisions(entity_death_writer, bullet_query, enemy_query);
    handle_bullet_vs_boss_collisions(entity_death_writer, boss_death_writer, bullet_query, boss_query);
    handle_beam_collisions(entity_death_writer, boss_death_writer, beam_query, enemy_query, boss_query);
}

static void player_collision_system(r::ecs::EventWriter<PlayerDiedEvent> death_writer,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::With<Player>> player_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::With<Enemy>> enemy_query)
{
    for (auto [player_transform, player_collider, _p] : player_query) {
        for (auto [enemy_transform, enemy_collider, _e] : enemy_query) {
            r::Vec3f player_center = player_transform.ptr->position + player_collider.ptr->offset;
            r::Vec3f delta = player_center - (enemy_transform.ptr->position + enemy_collider.ptr->offset);
            float distance = delta.length();

            float sum_radii = player_collider.ptr->radius + enemy_collider.ptr->radius;
            if (distance < sum_radii) {
                death_writer.send({});
                return;
            }
        }
    }
}

static void player_bullet_collision_system(r::ecs::EventWriter<PlayerDiedEvent> death_writer,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::With<Player>> player_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::With<EnemyBullet>> bullet_query)
{
    for (auto [player_transform, player_collider, _p] : player_query) {
        for (auto [bullet_transform, bullet_collider, _b] : bullet_query) {
            r::Vec3f player_center = player_transform.ptr->position + player_collider.ptr->offset;
            r::Vec3f delta = player_center - (bullet_transform.ptr->position + bullet_collider.ptr->offset);
            float distance = delta.length();

            float sum_radii = player_collider.ptr->radius + bullet_collider.ptr->radius;
            if (distance < sum_radii) {
                death_writer.send({});
                return;
            }
        }
    }
}

static void force_bullet_collision_system(r::ecs::EventWriter<EntityDiedEvent> entity_death_writer,
    r::ecs::Query<r::ecs::Ref<r::GlobalTransform3d>, r::ecs::Ref<Collider>, r::ecs::With<Force>> force_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::With<EnemyBullet>, r::ecs::Without<Unblockable>> bullet_query)
{
    if (force_query.size() == 0 || bullet_query.size() == 0) {
        return;
    }

    auto [force_transform, force_collider, _] = *force_query.begin();

    for (auto bullet_it = bullet_query.begin(); bullet_it != bullet_query.end(); ++bullet_it) {
        auto [bullet_transform, bullet_collider, __, ___] = *bullet_it;

        r::Vec3f force_center = force_transform.ptr->position + force_collider.ptr->offset;
        r::Vec3f bullet_center = bullet_transform.ptr->position + bullet_collider.ptr->offset;
        float distance = (force_center - bullet_center).length();
        float radii_sum = force_collider.ptr->radius + bullet_collider.ptr->radius;

        if (distance < radii_sum) {
            entity_death_writer.send({bullet_it.entity()});
        }
    }
}

static void force_enemy_collision_system(r::ecs::EventWriter<EntityDiedEvent> entity_death_writer,
    r::ecs::Query<r::ecs::Ref<r::GlobalTransform3d>, r::ecs::Ref<Collider>, r::ecs::With<Force>> force_query,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Ref<Collider>, r::ecs::With<Enemy>> enemy_query)
{
    if (force_query.size() == 0 || enemy_query.size() == 0) {
        return;
    }

    auto [force_transform, force_collider, _] = *force_query.begin();

    for (auto enemy_it = enemy_query.begin(); enemy_it != enemy_query.end(); ++enemy_it) {
        auto [enemy_transform, enemy_collider, __] = *enemy_it;

        r::Vec3f force_center = force_transform.ptr->position + force_collider.ptr->offset;
        r::Vec3f enemy_center = enemy_transform.ptr->position + enemy_collider.ptr->offset;
        float distance = (force_center - enemy_center).length();
        float radii_sum = force_collider.ptr->radius + enemy_collider.ptr->radius;

        if (distance < radii_sum) {
            entity_death_writer.send({enemy_it.entity()});
        }
    }
}

static void despawn_offscreen_system(r::ecs::Commands &commands,
    r::ecs::Query<r::ecs::Ref<r::Transform3d>, r::ecs::Without<Player>, r::ecs::Without<Boss>, r::ecs::Without<Force>> query)
{
    const float despawn_boundary_x = 100.0f;
    for (auto it = query.begin(); it != query.end(); ++it) {
        auto [transform, _, __, ___] = *it;
        if (std::abs(transform.ptr->position.x) > despawn_boundary_x) {
            commands.despawn(it.entity());
        }
    }
}

template<typename T>
static void despawn_all_entities_with(r::ecs::Commands &commands, r::ecs::Query<r::ecs::With<T>> &query)
{
    for (auto it = query.begin(); it != query.end(); ++it) {
        commands.despawn(it.entity());
    }
}

static void cleanup_battle_system(r::ecs::Commands &commands, r::ecs::Query<r::ecs::With<Enemy>> enemy_query,
    r::ecs::Query<r::ecs::With<PlayerBullet>> player_bullet_query, r::ecs::Query<r::ecs::With<EnemyBullet>> enemy_bullet_query,
    r::ecs::Query<r::ecs::With<WaveCannonBeam>> wave_cannon_query, r::ecs::Query<r::ecs::With<Player>> player_query,
    r::ecs::Query<r::ecs::With<Force>> force_query, r::ecs::Query<r::ecs::With<Boss>> boss_query)
{
    despawn_all_entities_with<Enemy>(commands, enemy_query);
    despawn_all_entities_with<PlayerBullet>(commands, player_bullet_query);
    despawn_all_entities_with<EnemyBullet>(commands, enemy_bullet_query);
    despawn_all_entities_with<WaveCannonBeam>(commands, wave_cannon_query);
    despawn_all_entities_with<Player>(commands, player_query);
    despawn_all_entities_with<Force>(commands, force_query);
    despawn_all_entities_with<Boss>(commands, boss_query);
}

static void reset_level_progress_system(r::ecs::ResMut<CurrentLevel> current_level)
{
    current_level.ptr->index = 0;
    r::Logger::info("Game progress reset. Starting at Level 1.");
}

void CombatPlugin::build(r::Application &app)
{
    app.add_systems<reset_level_progress_system>(r::OnTransition{GameState::MainMenu, GameState::EnemiesBattle})
        .add_systems<reset_level_progress_system>(r::OnTransition{GameState::GameOver, GameState::EnemiesBattle})
        .add_systems<reset_level_progress_system>(r::OnTransition{GameState::YouWin, GameState::EnemiesBattle})

        .add_systems<cleanup_battle_system>(r::OnEnter{GameState::EnemiesBattle})
        .run_unless<run_conditions::is_resuming_from_pause>()

        .add_systems<despawn_offscreen_system>(r::Schedule::UPDATE)

        /* New event handler for despawning. Only runs when events are present. */
        .add_systems<handle_entity_death>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::on_event<EntityDiedEvent>>()

        /* The collision systems now only send events */
        .add_systems<collision_system, player_collision_system, player_bullet_collision_system, force_bullet_collision_system,
            force_enemy_collision_system>(r::Schedule::UPDATE)
        .run_if<r::run_conditions::in_state<GameState::EnemiesBattle>>()
        .run_or<r::run_conditions::in_state<GameState::BossBattle>>();
}
