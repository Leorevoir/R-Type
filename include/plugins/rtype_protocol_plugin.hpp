#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "../../external/R-Engine/include/R-Engine/Plugins/NetworkPlugin.hpp"

namespace rtype {
namespace protocol {


/* R-Type protocol command enum */
enum class RTypeCommand : uint8_t {
    CMD_INPUT = 1,
    CMD_SNAPSHOT = 2,
    CMD_CHAT = 3,
    CMD_PING = 4,
    CMD_PONG = 5,
    CMD_ACK = 6,
    CMD_JOIN = 7,
    CMD_KICK = 8,
    CMD_CHALLENGE = 9,
    CMD_AUTH = 10,
    CMD_AUTH_OK = 11,
    CMD_RESYNC = 12,
    CMD_FRAGMENT = 13,
    CMD_PLAYER_STATS = 14,
    CMD_PLAYER_DEATH = 15,
    CMD_PLAYER_SCORE = 16,
    CMD_GAME_END = 17,
    CMD_LEAVE = 18,
    CMD_READY = 19,
    CMD_NOT_READY = 20,
    CMD_CREATE = 21,
    CMD_CREATE_KO = 22,
    CMD_JOIN_KO = 23,
    CMD_PAUSE = 24,
    CMD_RESUME = 25,
    CMD_LEADERBOARD = 26,
    CMD_SPECTATE = 27
};

/* R-Type protocol flags enum */
enum class RTypeFlags : uint8_t {
    F_CONN = 1 << 0,
    F_RELIABLE = 1 << 1,
    F_FRAGMENT = 1 << 2,
    F_PING = 1 << 3,
    F_CLOSE = 1 << 4,
    F_ENCRYPTED = 1 << 5,
    F_COMPRESSED = 1 << 6
};

/* R-Type protocol channels enum */
enum class RTypeChannel : uint8_t {
    C_UU = 0b00,
    C_UO = 0b01,
    C_RU = 0b10,
    C_RO = 0b11
};

/* Standardized input enum */
enum class RTypeInput : uint8_t {
    I_FWD = 1
    // Ajouter d'autres inputs si besoin
};

/* R-Type UDP header structure */
struct RTypeHeader {
    uint16_t magic;      /* Magic number: 0x4254 */
    uint8_t version;     /* Protocol version: 0b1 */
    uint8_t flags;
    uint32_t seq;
    uint32_t ackBase;
    uint8_t ackBits;
    uint8_t channel;
    uint16_t size;
    uint32_t id;
    uint8_t command;
};

/* R-Type packet structure */
struct RTypePacket {
    RTypeHeader header;
    std::vector<uint8_t> payload;
};

/* TCP packet structure for Gateway */
enum class RTypeTCPMessage : uint8_t {
    GCMD_JOIN = 1,
    GCMD_JOIN_KO = 2,
    GCMD_CREATE = 3,
    GCMD_CREATE_KO = 4,
    GCMD_GAME_END = 5,
    GCMD_GS = 20,
    GCMD_GS_OK = 21,
    GCMD_GS_KO = 22,
    GCMD_OCCUPANCY = 23,
    GCMD_GID = 24
};

enum class RTypeGameType : uint8_t {
    G_RTYPE = 1
};

struct RTypeTCPPacket {
    uint16_t magic;      /* Magic number: 0x4257 */
    uint8_t version;     /* Protocol version: 0b1 */
    uint8_t flags;
    RTypeTCPMessage type;
    std::vector<uint8_t> payload;
};

/* Methods for each protocol interaction */
class RTypeProtocolePlugin : public rtype::network::NetworkPlugin {
public:
    RTypeProtocolePlugin();
    ~RTypeProtocolePlugin();

    /* Connection and Authentication */
    bool sendJoin(uint32_t id, uint8_t nonce, uint8_t version);
    bool sendChallenge(uint64_t timestamp, const std::vector<uint8_t>& cookie);
    bool sendAuth(uint8_t nonce, const std::vector<uint8_t>& cookie);
    bool sendAuthOk(uint32_t id, const std::vector<uint8_t>& sessionKey);
    bool sendKick(const std::string& message);

    /* Gameplay */
    bool sendInput(RTypeInput type, uint8_t value);
    bool sendSnapshot(uint32_t seq);
    bool sendPlayerStats(int hp, int score, int powerups);
    bool sendPlayerDeath();
    bool sendPlayerScore(int score);

    /* Communication and Synchronization */
    bool sendChat(const std::string& msg);
    bool sendPing();
    bool sendPong();
    bool sendAck(uint32_t seq);
    bool sendResync();
    bool sendFragment(uint32_t seq, const std::vector<uint8_t>& payload);

    /* Game management */
    bool sendGameEnd(uint32_t gameId);
    bool sendLeave();
    bool sendReady();
    bool sendNotReady();
    bool sendCreate(RTypeGameType type);
    bool sendCreateKo();
    bool sendJoinKo();

    /* Miscellaneous */
    bool sendPause();
    bool sendResume();
    bool sendLeaderboard();
    bool sendSpectate();

    /* Generic packet reception */
    bool receivePacket(RTypePacket& packet);

private:
    std::string serverAddress;
    uint16_t serverPort;
};

class RTypeProtocolePlugin : public rtype::network::NetworkPlugin {
public:
    RTypeProtocolePlugin();
    ~RTypeProtocolePlugin();

    bool connectToServer(const std::string &address, uint16_t port);
    void disconnect();
    bool sendPacket(const RTypePacket &packet);
    bool receivePacket(RTypePacket &packet);

    /* Game-specific methods */
    bool sendPlayerConnect(const std::string &playerName);
    bool sendPlayerStats(int hp, int score);
    bool sendPlayerDeath();
    bool sendPlayerScore(int score);
    // ...

private:
    std::string serverAddress;
    uint16_t serverPort;
};

} // namespace protocol
} // namespace rtype
