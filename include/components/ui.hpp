#pragma once

struct MenuButton {
        enum class Action { None, Play, Options, Quit };
        Action action = Action::None;
};

/* --- UI Root Marker Components --- */

struct MenuRoot {
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
