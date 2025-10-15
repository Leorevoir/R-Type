#include <R-Type/Game.hpp>
#include <R-Type/Startup.hpp>

// clang-format off

void r::startup_load_player(ecs::Commands &commands) noexcept
{
    commands.spawn(
        Controllable{},
        Player{},
        Transform3d{
            .position = {0, 0, 0}
        },
        Velocity{
            {.0f, .0f, .0f}
        }
    );
}
