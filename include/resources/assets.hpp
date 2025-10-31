#pragma once

#include <R-Engine/Plugins/MeshPlugin.hpp>

struct PlayerBulletAssets {
        r::MeshHandle laser_beam_handle = r::MeshInvalidHandle;
        r::MeshHandle force_missile = r::MeshInvalidHandle;
};

struct BossBulletAssets {
        r::MeshHandle big_missile = r::MeshInvalidHandle;
        r::MeshHandle small_missile = r::MeshInvalidHandle;
};
