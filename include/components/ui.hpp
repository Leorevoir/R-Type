#pragma once

struct MenuButton {
        enum class Action { None, Play, Options, Quit };
        Action action = Action::None;
};

struct PauseMenuButton {
        enum class Action { None, Resume, Options, BackToMenu };
        Action action = Action::None;
};

struct SettingsMenuButton {
        enum class Action { None, Video, Audio, Controls, Accessibility, Back };
        Action action = Action::None;
};

/* --- UI Root Marker Components --- */

struct MenuRoot {
};
struct PauseRoot {
};
struct SettingsRoot {
};
struct GameOverRoot {
};
struct YouWinRoot {
};
struct HudRoot {
};

/* --- HUD Element Marker Components --- */

struct ScoreText {
};
struct LivesText {
};

/* --- Settings Menu Element Marker Components --- */

struct SettingsTitleText {
};
struct VideoSettingsRoot {
};
struct AudioSettingsRoot {
};
struct ControlsSettingsRoot {
};
struct AccessibilitySettingsRoot {
};

struct DisplayModeDropdown {
};
struct ResolutionDropdown {
};
struct VSyncToggle {
};
struct FramerateLimitSlider {
};
struct PostProcessingToggle {
};
