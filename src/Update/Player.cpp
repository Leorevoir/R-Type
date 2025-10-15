#include <R-Type/Update.hpp>

// clang-format off

static constexpr r::Vec3f G_FORWARD{0.0f, 0.0f, 1.0f};

void r::update_player_position(
    const ecs::Res<core::FrameTime> time,
    const ecs::Query<ecs::Mut<Transform3d>, ecs::Ref<Velocity>> query,
    const ecs::ResMut<r::Camera3d> camera
) noexcept
{
    for (auto [transform, velocity] : query) {
        const Vec3f player_pos = transform.ptr->position += *velocity.ptr * time.ptr->delta_time;

        camera.ptr->position = player_pos + G_FORWARD;
        camera.ptr->target = player_pos;
        break;
    }
}
