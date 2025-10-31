#pragma once

#include "R-Engine/Application.hpp"
#include "R-Engine/Plugins/Plugin.hpp"
#include "R-Engine/Plugins/NetworkPlugin.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace rtype::protocol {

/**
 * @brief R-Type protocol commands
 */
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

/**
 * @brief R-Type protocol flags
 */
enum class RTypeFlags : uint8_t {
    F_CONN = 1 << 0,
    F_RELIABLE = 1 << 1,
    F_FRAGMENT = 1 << 2,
    F_PING = 1 << 3,
    F_CLOSE = 1 << 4,
    F_ENCRYPTED = 1 << 5,
    F_COMPRESSED = 1 << 6
};

/**
 * @brief R-Type protocol channels
 */
enum class RTypeChannel : uint8_t {
    C_UU = 0b00,
    C_UO = 0b01,
    C_RU = 0b10,
    C_RO = 0b11
};

/**
 * @brief Standardized inputs for R-Type protocol
 */
enum class RTypeInput : uint8_t {
    I_FWD = 1
    // Ajouter d'autres inputs si besoin
};

/**
 * @brief R-Type UDP header structure
 */
struct RTypeHeader {
    uint16_t magic; ///< Magic number: 0x4254
    uint8_t version; ///< Protocol version: 0b1
    uint8_t flags;
    uint32_t seq;
    uint32_t ackBase;
    uint8_t ackBits;
    uint8_t channel;
    uint16_t size;
    uint32_t id;
    uint8_t command;
};

/**
 * @brief R-Type UDP packet structure
 */
struct RTypePacket {
    RTypeHeader header;
    std::vector<uint8_t> payload;
};

/**
 * @brief TCP commands for R-Type Gateway
 */
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

/**
 * @brief R-Type game types
 */
enum class RTypeGameType : uint8_t { G_RTYPE = 1 };

/**
 * @brief TCP packet structure for R-Type Gateway
 */
struct RTypeTCPPacket {
    uint16_t magic; ///< Magic number: 0x4257
    uint8_t version; ///< Protocol version: 0b1
    uint8_t flags;
    RTypeTCPMessage type;
    std::vector<uint8_t> payload;
};

/**
 * @brief Event for sending R-Type packet
 */
struct SendRTypePacket {
    RTypePacket packet;
};

/**
 * @brief Event for receiving R-Type packet
 */
struct ReceivedRTypePacket {
    RTypePacket packet;
};

/**
 * @brief R-Type protocol plugin (UDP)
 */
class RTypeProtocolPlugin final : public r::Plugin {
public:
    RTypeProtocolPlugin() noexcept = default;
    ~RTypeProtocolPlugin() override = default;

    /**
     * @brief Adds R-Type protocol systems and events to the application.
     * @param app Application to configure
     */
    void build(r::Application &app) override;
};

/**
 * @brief System for sending R-Type packets via the NetworkPlugin.
 */
void send_rtype_packet_system(
    r::ecs::EventReader<SendRTypePacket> send_events,
    r::ecs::EventWriter<r::net::NetworkSendEvent> network_send_events);

/**
 * @brief System for receiving R-Type packets from the NetworkPlugin.
 */
void receive_rtype_packet_system(
    r::ecs::EventReader<r::net::NetworkMessageEvent> network_message_events,
    r::ecs::EventWriter<ReceivedRTypePacket> received_events);

} // namespace rtype::protocol

