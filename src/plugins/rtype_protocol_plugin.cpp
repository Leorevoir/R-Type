#include "plugins/rtype_protocol_plugin.hpp"
#include "R-Engine/Plugins/NetworkPlugin.hpp"
#include "R-Engine/ECS/Event.hpp"
#include <arpa/inet.h>
#include <cstring>

namespace rtype::protocol {

static uint16_t toBigEndian16(uint16_t value) { return htons(value); }
static uint32_t toBigEndian32(uint32_t value) { return htonl(value); }

/**
 * @brief Encode the R-Type UDP header in big endian.
 *
 * @param header R-Type header to encode
 * @return std::vector<uint8_t> Encoded header
 */
static std::vector<uint8_t> encodeHeader(const RTypeHeader &header) {
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

/**
 * @brief Encode a R-Type UDP packet (header + payload).
 *
 * @param packet R-Type packet to encode
 * @return std::vector<uint8_t> Encoded packet
 */
static std::vector<uint8_t> encodePacket(const RTypePacket &packet) {
    std::vector<uint8_t> out = encodeHeader(packet.header);
    out.insert(out.end(), packet.payload.begin(), packet.payload.end());
    return out;
}

/**
 * @brief System for sending R-Type packets via the NetworkPlugin.
 *
 * @param send_events R-Type send events
 * @param network_send_events Network send events
 */
void send_rtype_packet_system(
    r::ecs::EventReader<SendRTypePacket> send_events,
    r::ecs::EventWriter<r::net::NetworkSendEvent> network_send_events) {
    for (auto &event : send_events) {
        std::vector<uint8_t> data = encodePacket(event.packet);
        r::net::Packet net_packet;
        net_packet.payload = data;
        network_send_events.send({net_packet});
    }
}

/**
 * @brief System for receiving R-Type packets from the NetworkPlugin.
 *
 * @param network_message_events Received network events
 * @param received_events Received R-Type events
 */
void receive_rtype_packet_system(
    r::ecs::EventReader<r::net::NetworkMessageEvent> network_message_events,
    r::ecs::EventWriter<ReceivedRTypePacket> received_events) {
    for (auto &event : network_message_events) {
        if (event.payload.size() < 21)
            continue;

        RTypePacket packet;
        memcpy(&packet.header.magic, &event.payload[0], 2);
        packet.header.magic = ntohs(packet.header.magic);

        if (packet.header.magic != 0x4254)
            continue;

        packet.header.version = event.payload[2];
        packet.header.flags = event.payload[3];
        memcpy(&packet.header.seq, &event.payload[4], 4);
        packet.header.seq = ntohl(packet.header.seq);
        memcpy(&packet.header.ackBase, &event.payload[8], 4);
        packet.header.ackBase = ntohl(packet.header.ackBase);
        packet.header.ackBits = event.payload[12];
        packet.header.channel = event.payload[13];
        memcpy(&packet.header.size, &event.payload[14], 2);
        packet.header.size = ntohs(packet.header.size);
        memcpy(&packet.header.id, &event.payload[16], 4);
        packet.header.id = ntohl(packet.header.id);
        packet.header.command = event.payload[20];
        packet.payload.assign(event.payload.begin() + 21,
                              event.payload.end());
        received_events.send({packet});
    }
}

/**
 * @brief Adds R-Type protocol systems and events to the application.
 *
 * @param app Application to configure
 */
void RTypeProtocolPlugin::build(r::Application &app) {
    app.add_systems(r::Schedule::UPDATE, send_rtype_packet_system);
    app.add_systems(r::Schedule::UPDATE, receive_rtype_packet_system);
    app.add_events<SendRTypePacket, ReceivedRTypePacket>();
}

} // namespace rtype::protocol