#include "plugins/rtype_protocol_plugin.hpp"
#include "R-Engine/ECS/Event.hpp"
#include "R-Engine/Plugins/NetworkPlugin.hpp"
#include <vector>

namespace rtype::protocol {

/**
 * @brief System for sending R-Type packets via the NetworkPlugin.
 *
 * This system listens for SendRTypePacket events, translates the R-Type
 * specific packet into the generic r::net::Packet format, and then fires a
 * NetworkSendEvent for the NetworkPlugin to handle.
 *
 * @param send_events R-Type send events
 * @param network_send_events Network send events
 */
void send_rtype_packet_system(r::ecs::EventReader<SendRTypePacket> send_events,
    r::ecs::EventWriter<r::net::NetworkSendEvent> network_send_events)
{
    for (const auto &event : send_events) {
        r::net::Packet net_packet;

        /* Map R-Type protocol concepts to the generic network packet fields */
        net_packet.magic = event.packet.header.magic;
        net_packet.version = event.packet.header.version;
        net_packet.flags = event.packet.header.flags;
        net_packet.channel = event.packet.header.channel;
        net_packet.clientId = event.packet.header.id;
        net_packet.command = event.packet.header.command;
        net_packet.payload = event.packet.payload;

        /* The NetworkPlugin will handle filling in the sequence and acknowledgment fields. */
        network_send_events.send({net_packet});
    }
}

/**
 * @brief System for receiving R-Type packets from the NetworkPlugin.
 *
 * This system listens for the generic NetworkMessageEvent from the NetworkPlugin,
 * reconstructs an R-Type specific packet from it, and fires a
 * ReceivedRTypePacket event for other game systems to consume.
 *
 * @param network_message_events Received network events from NetworkPlugin
 * @param received_events R-Type received events to be fired
 */
void receive_rtype_packet_system(r::ecs::EventReader<r::net::NetworkMessageEvent> network_message_events,
    r::ecs::EventWriter<ReceivedRTypePacket> received_events)
{
    for (const auto &event : network_message_events) {
        RTypePacket rtype_packet;

        /* The NetworkMessageEvent provides the core information.
           Other header fields (like flags, client ID) are not passed through
           this specific event in the current NetworkPlugin implementation.
           We reconstruct what we can. */
        rtype_packet.header.command = event.message_type;
        rtype_packet.payload = event.payload;

        /* Note: For a full implementation, the NetworkMessageEvent might need
           to be expanded to include more data like the sender's client ID.
           For now, we work with what's available. */
        received_events.send({rtype_packet});
    }
}

/**
 * @brief Adds R-Type protocol systems and events to the application.
 * @param app Application to configure
 */
void RTypeProtocolPlugin::build(r::Application &app)
{
    /* These systems act as a translation layer between the game's high-level
       R-Type protocol events and the engine's low-level NetworkPlugin events. */
    app.add_systems<send_rtype_packet_system>(r::Schedule::UPDATE);
    app.add_systems<receive_rtype_packet_system>(r::Schedule::UPDATE);
    app.add_events<SendRTypePacket, ReceivedRTypePacket>();
}

}// namespace rtype::protocol
