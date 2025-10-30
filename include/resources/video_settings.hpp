#pragma once

#include <R-Engine/Maths/Vec.hpp>
#include <R-Engine/Plugins/PostProcessingPlugin.hpp>

enum class DisplayMode {
    Fullscreen,
    Windowed,
    BorderlessWindowed,
};

struct VideoSettings {
        DisplayMode display_mode = DisplayMode::Windowed;
        r::Vec2u resolution = {1280, 720};
        bool vsync = true;
        int framerate_limit = 60;
        bool post_processing_effects = false;
        r::PostProcessingState selected_effect = r::PostProcessingState::Disabled;
};
