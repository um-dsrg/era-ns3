#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/abort.h"
#include "ns3/double.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/point-to-point-net-device.h"

#include "sdn-switch.h"
#include "../../common-code/random-generator-manager.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SdnSwitch");

SdnSwitch::SdnSwitch (id_t id) : SwitchBase (id) {
}

void SdnSwitch::AddEntryToRoutingTable (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort,
                                         portNum_t dstPort, FlowProtocol protocol,
                                         Ptr<NetDevice> forwardingPort) {
    RtFlow rtFlow {srcIp, dstIp, srcPort, dstPort, protocol};
    auto ret = m_routingTable.emplace (rtFlow, forwardingPort);
    NS_ABORT_MSG_IF (ret.second == false, "Unable to add routing table entry in Switch " << m_id);

    NS_LOG_INFO("Add entry in PPFS Switch " << m_id);
    NS_LOG_INFO(rtFlow);
}

void SdnSwitch::SetPacketReception() {
    uint32_t numOfDevices = m_node->GetNDevices ();
    NS_ASSERT (numOfDevices > 0);
    for (uint32_t currentDevice = 0; currentDevice < numOfDevices; ++currentDevice) {
        m_node->RegisterProtocolHandler (MakeCallback (&SdnSwitch::PacketReceived, this),
                                         /*all protocols*/ 0, m_node->GetDevice (currentDevice),
                                         /*disable promiscuous mode*/ false);
    }
}

void SdnSwitch::PacketReceived(Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                const Address &src, const Address &dst, NetDevice::PacketType packetType) {
    NS_LOG_INFO("Switch " << m_id << " received a packet at " << Simulator::Now().GetSeconds() << "s");
    auto parsedFlow = ExtractFlowFromPacket(packet, protocol);

    try {
        auto forwardingNetDevice = m_routingTable.at(parsedFlow);
        auto sendSuccess = forwardingNetDevice->Send(packet->Copy(), dst, protocol);
        if (!sendSuccess) {
            NS_LOG_INFO("Warning: Packet transmission failed");
        }
    } catch (const std::out_of_range& oor) {
        NS_ABORT_MSG("Routing table Miss on Switch " << m_id << ".\nFlow Details\n" << parsedFlow);
    }
}

std::ostream& operator<<(std::ostream& os, const SdnSwitch::RtFlow& flow)
{
    os << "Protocol " << static_cast<char> (flow.protocol) << "\n";
    os << "Source IP " << flow.srcIp << "\n";
    os << "Source Port " << flow.srcPort << "\n";
    os << "Destination IP " << flow.dstIp << "\n";
    os << "Destination Port " << flow.dstPort << "\n";
    return os;
}

bool SdnSwitch::RtFlow::operator< (const RtFlow &other) const {
    if (srcIp == other.srcIp) {
        if (dstIp == other.dstIp) {
            if (srcPort == other.srcPort) {
                return dstPort < other.dstPort;
            } else {
                return srcPort < other.srcPort;
            }
        } else {
            return dstIp < other.dstIp;
        }
    } else {
        return srcIp < other.srcIp;
    }
}

SdnSwitch::RtFlow SdnSwitch::ExtractFlowFromPacket(Ptr<const Packet> packet, uint16_t protocol) {
    Ptr<Packet> receivedPacket = packet->Copy (); // Copy the packet for parsing purposes
    RtFlow flow;

    if (protocol == Ipv4L3Protocol::PROT_NUMBER) {
        Ipv4Header ipHeader;
        uint8_t ipProtocol (0);

        if (receivedPacket->PeekHeader (ipHeader)) { // Extract source and destination IP
            ipProtocol = ipHeader.GetProtocol ();
            flow.srcIp = ipHeader.GetSource().Get();
            flow.dstIp = ipHeader.GetDestination().Get();
            receivedPacket->RemoveHeader (ipHeader); // Removing the IP header
        }

        if (ipProtocol == UdpL4Protocol::PROT_NUMBER) { // UDP Packet
            UdpHeader udpHeader;
            if (receivedPacket->PeekHeader (udpHeader)) {
                flow.srcPort = udpHeader.GetSourcePort();
                flow.dstPort = udpHeader.GetDestinationPort();
                flow.protocol = FlowProtocol::Udp;
            }
        } else if (ipProtocol == TcpL4Protocol::PROT_NUMBER) { // TCP Packet
            TcpHeader tcpHeader;
            if (receivedPacket->PeekHeader (tcpHeader)) {
                flow.srcPort = tcpHeader.GetSourcePort();
                flow.dstPort = tcpHeader.GetDestinationPort();
                flow.protocol = FlowProtocol::Tcp;
            }
        } else if (ipProtocol == Icmpv4L4Protocol::PROT_NUMBER) { // ICMP Packet
            Icmpv4Header icmpHeader;
            if (receivedPacket->PeekHeader (icmpHeader)) {
                flow.protocol = FlowProtocol::Icmp;
                switch (icmpHeader.GetType ()) {
                    case Icmpv4Header::Type_e::ICMPV4_ECHO:
                        NS_LOG_INFO ("ICMP Echo message received at Switch " << m_id);
                        break;
                    case Icmpv4Header::Type_e::ICMPV4_ECHO_REPLY:
                        NS_LOG_INFO ("ICMP Echo reply message received at Switch " << m_id);
                        break;
                    case Icmpv4Header::Type_e::ICMPV4_DEST_UNREACH:
                        NS_LOG_INFO ("ICMP Destination Unreachable message received at Switch " << m_id);
                        break;
                    case Icmpv4Header::Type_e::ICMPV4_TIME_EXCEEDED:
                        NS_LOG_INFO ("ICMP Time exceeded message received at Switch " << m_id);
                        break;
                    default:
                        NS_ABORT_MSG ("ICMP unidentified message received at Switch " << m_id);
                        break;
                }
            }
        } else {
            NS_ABORT_MSG ("Unknown packet type received. Packet Type " << std::to_string (ipProtocol));
        }
    }  else {
        NS_ABORT_MSG ("Non-IP Packet received. Protocol value " << protocol);
    }

    return flow;
}
