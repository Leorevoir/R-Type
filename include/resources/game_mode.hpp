#pragma once

#include <string>

enum class GameMode {
    Offline,
    Online,
};

struct NetworkConfig {
        std::string server_address = "0.0.0.0";
        unsigned short server_port = 4000;
};
