
#include "../../include/plugins/rtype_protocol_plugin.hpp"
#include <cstring>
#include <algorithm>
#include <arpa/inet.h>


namespace rtype {
namespace protocol {

static uint16_t toBigEndian16(uint16_t value) { return htons(value); }
static uint32_t toBigEndian32(uint32_t value) { return htonl(value); }

RTypeProtocolePlugin::RTypeProtocolePlugin() : serverPort(0) {}
RTypeProtocolePlugin::~RTypeProtocolePlugin() { disconnect(); }

bool RTypeProtocolePlugin::connectToServer(const std::string &address, uint16_t port) {
    serverAddress = address;
    serverPort = port;
    try {
        rtype::network::Endpoint ep{address, port};
        this->connectToServer(ep, rtype::network::Protocol::TCP);
        return true;
    } catch (...) {
        return false;
    }
}

void RTypeProtocolePlugin::disconnect() {
    this->disconnectFromServer();
}

// Encodage du header UDP R-Type
static std::vector<uint8_t> encodeHeader(const RTypeHeader& header) {
    std::vector<uint8_t> out(21);
    uint16_t magic = toBigEndian16(header.magic);
    uint32_t seq = toBigEndian32(header.seq);
    uint32_t ackBase = toBigEndian32(header.ackBase);
    uint16_t size = toBigEndian16(header.size);
    uint32_t id = toBigEndian32(header.id);
    memcpy(&out[0], &magic, 2);
    out[2] = header.version;
    out[3] = header.flags;
    memcpy(&out[4], &seq, 4);
    memcpy(&out[8], &ackBase, 4);
    out[12] = header.ackBits;
    out[13] = header.channel;
    memcpy(&out[14], &size, 2);
    memcpy(&out[16], &id, 4);
    out[20] = header.command;
    return out;
}

// Encodage d'un paquet UDP R-Type
static std::vector<uint8_t> encodePacket(const RTypePacket& packet) {
    std::vector<uint8_t> out = encodeHeader(packet.header);
    out.insert(out.end(), packet.payload.begin(), packet.payload.end());
    return out;
}

// Envoi générique d'un paquet UDP
bool RTypeProtocolePlugin::sendPacket(const RTypePacket& packet) {
    std::vector<uint8_t> data = encodePacket(packet);
    rtype::network::Packet netPacket;
    netPacket.data = data;
    this->sendPacket(netPacket, rtype::network::Protocol::UDP);
    return true;
}

// Réception générique d'un paquet UDP
bool RTypeProtocolePlugin::receivePacket(RTypePacket& packet) {
    rtype::network::Packet netPacket = this->receivePacket(rtype::network::Protocol::UDP);
    if (netPacket.data.size() < 21) return false;
    memcpy(&packet.header.magic, &netPacket.data[0], 2);
    packet.header.magic = ntohs(packet.header.magic);
    packet.header.version = netPacket.data[2];
    packet.header.flags = netPacket.data[3];
    memcpy(&packet.header.seq, &netPacket.data[4], 4);
    packet.header.seq = ntohl(packet.header.seq);
    memcpy(&packet.header.ackBase, &netPacket.data[8], 4);
    packet.header.ackBase = ntohl(packet.header.ackBase);
    packet.header.ackBits = netPacket.data[12];
    packet.header.channel = netPacket.data[13];
    memcpy(&packet.header.size, &netPacket.data[14], 2);
    packet.header.size = ntohs(packet.header.size);
    memcpy(&packet.header.id, &netPacket.data[16], 4);
    packet.header.id = ntohl(packet.header.id);
    packet.header.command = netPacket.data[20];
    packet.payload.assign(netPacket.data.begin() + 21, netPacket.data.end());
    return true;
}

// Connexion et Authentification
bool RTypeProtocolePlugin::sendJoin(uint32_t id, uint8_t nonce, uint8_t version) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.version = version;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_JOIN);
    packet.payload.resize(6);
    uint32_t id_be = toBigEndian32(id);
    memcpy(&packet.payload[0], &id_be, 4);
    packet.payload[4] = nonce;
    packet.payload[5] = version;
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendChallenge(uint64_t timestamp, const std::vector<uint8_t>& cookie) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_CHALLENGE);
    packet.payload.resize(8 + cookie.size());
    uint64_t ts_be = htobe64(timestamp);
    memcpy(&packet.payload[0], &ts_be, 8);
    std::copy(cookie.begin(), cookie.end(), packet.payload.begin() + 8);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendAuth(uint8_t nonce, const std::vector<uint8_t>& cookie) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_AUTH);
    packet.payload.resize(1 + cookie.size());
    packet.payload[0] = nonce;
    std::copy(cookie.begin(), cookie.end(), packet.payload.begin() + 1);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendAuthOk(uint32_t id, const std::vector<uint8_t>& sessionKey) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_AUTH_OK);
    packet.payload.resize(4 + sessionKey.size());
    uint32_t id_be = toBigEndian32(id);
    memcpy(&packet.payload[0], &id_be, 4);
    std::copy(sessionKey.begin(), sessionKey.end(), packet.payload.begin() + 4);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendKick(const std::string& message) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_KICK);
    packet.payload.assign(message.begin(), message.end());
    return sendPacket(packet);
}

// Gameplay
bool RTypeProtocolePlugin::sendInput(RTypeInput type, uint8_t value) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_INPUT);
    packet.payload.push_back(static_cast<uint8_t>(type));
    packet.payload.push_back(value);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendSnapshot(uint32_t seq) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_SNAPSHOT);
    uint32_t seq_be = toBigEndian32(seq);
    packet.payload.resize(4);
    memcpy(&packet.payload[0], &seq_be, 4);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendPlayerStats(int hp, int score, int powerups) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_PLAYER_STATS);
    packet.payload.push_back(static_cast<uint8_t>(hp));
    packet.payload.push_back(static_cast<uint8_t>(score));
    packet.payload.push_back(static_cast<uint8_t>(powerups));
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendPlayerDeath() {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_PLAYER_DEATH);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendPlayerScore(int score) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_PLAYER_SCORE);
    packet.payload.push_back(static_cast<uint8_t>(score));
    return sendPacket(packet);
}

// Communication et Synchronisation
bool RTypeProtocolePlugin::sendChat(const std::string& msg) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_CHAT);
    uint16_t len = toBigEndian16(msg.size());
    packet.payload.resize(2 + msg.size());
    memcpy(&packet.payload[0], &len, 2);
    memcpy(&packet.payload[2], msg.data(), msg.size());
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendPing() {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_PING);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendPong() {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_PONG);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendAck(uint32_t seq) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_ACK);
    uint32_t seq_be = toBigEndian32(seq);
    packet.payload.resize(4);
    memcpy(&packet.payload[0], &seq_be, 4);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendResync() {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_RESYNC);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendFragment(uint32_t seq, const std::vector<uint8_t>& payload) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_FRAGMENT);
    uint32_t seq_be = toBigEndian32(seq);
    packet.payload.resize(4 + payload.size());
    memcpy(&packet.payload[0], &seq_be, 4);
    std::copy(payload.begin(), payload.end(), packet.payload.begin() + 4);
    return sendPacket(packet);
}

// Gestion de la partie
bool RTypeProtocolePlugin::sendGameEnd(uint32_t gameId) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_GAME_END);
    uint32_t id_be = toBigEndian32(gameId);
    packet.payload.resize(4);
    memcpy(&packet.payload[0], &id_be, 4);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendLeave() {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_LEAVE);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendReady() {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_READY);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendNotReady() {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_NOT_READY);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendCreate(RTypeGameType type) {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_CREATE);
    packet.payload.push_back(static_cast<uint8_t>(type));
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendCreateKo() {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_CREATE_KO);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendJoinKo() {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_JOIN_KO);
    return sendPacket(packet);
}

// Divers
bool RTypeProtocolePlugin::sendPause() {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_PAUSE);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendResume() {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_RESUME);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendLeaderboard() {
    RTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_LEADERBOARD);
    return sendPacket(packet);
}

bool RTypeProtocolePlugin::sendSpectate() {
    RmsTypePacket packet;
    packet.header.magic = 0x4254;
    packet.header.command = static_cast<uint8_t>(RTypeCommand::CMD_SPECTATE);
    return sendPacket(packet);
}

} // namespace protocol
} // namespace rtype
