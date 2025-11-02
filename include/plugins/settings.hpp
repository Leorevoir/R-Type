#pragma once

#include <R-Engine/Plugins/Plugin.hpp>

class SettingsPlugin final : public r::Plugin
{
    public:
        void build(r::Application &app) override;
};
