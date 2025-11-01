#pragma once

#include <R-Engine/Maths/Vec.hpp>

struct Background {
        float scroll_speed = 2.0f;
};

struct ScrollingScenery {
        float scroll_speed = 4.0f;
};

struct Asteroid {
        r::Vec3f velocity;
        r::Vec3f rotation_speed;
};
