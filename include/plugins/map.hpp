#pragma once
#include <R-Engine/Plugins/Plugin.hpp>

class MapPlugin final : public r::Plugin
{
    public:
        void build(r::Application &app) override;
};
