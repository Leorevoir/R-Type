#pragma once

struct PlayerLives {
        int count = 3;
};

struct PlayerScore {
        int value = 0;
        int next_life_threshold = 20000;
};
