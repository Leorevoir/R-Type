#pragma once
#include <R-Engine/Plugins/Plugin.hpp>

class UiSfxPlugin final : public r::Plugin
{
    public:
        void build(r::Application &app) override;
};

// Shared components/resources used by the UI SFX plugin and other plugins
struct UiSfxTag {};

struct UiSfxBorn {
    std::uint64_t frame = 0;
};

struct UiSfxCounter {
    std::uint64_t frame = 0;
};
