#include <R-Type/Update.hpp>

// clang-format off

static constexpr f32 G_PLAYER_SPEED = 10.f;

void r::update_inputs(
    const ecs::Res<UserInput> user_input,
    const ecs::Res<InputMap> input_map,
    const ecs::Query<ecs::Mut<Velocity>, ecs::With<Controllable>> query) noexcept
{
    const auto *map = input_map.ptr;
    const auto &input = *user_input.ptr;

    for (auto [velocity, _] : query) {
        r::Vec3f dir = {0.0f, 0.0f, 0.0f};

        if (map->isActionPressed("MoveForward", input)) {
            dir.z -= 1.f;
        }
        if (map->isActionPressed("MoveBackward", input)) {
            dir.z += 1.f;
        }
        if (map->isActionPressed("MoveLeft", input)) {
            dir.x -= 1.f;
        }
        if (map->isActionPressed("MoveRight", input)) {
            dir.x += 1.f;
        }
        if (map->isActionPressed("MoveUp", input)) {
            dir.y += 1.f;
        }
        if (map->isActionPressed("MoveDown", input)) {
            dir.y -= 1.f;
        }

        if (dir.length() > 0.f) {
            dir = dir.normalize() * G_PLAYER_SPEED;
        }

        *velocity.ptr = dir;
    }
}
