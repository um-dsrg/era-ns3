#ifndef ppfs_switch_h
#define ppfs_switch_h

#include <map>

#include "ns3/queue.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/net-device.h"
#include "ns3/random-variable-stream.h"

#include "../common-code/switch-base.h"

class PpfsSwitch : public SwitchBase {
public:
    PpfsSwitch (id_t id);

    void AddEntryToRoutingTable (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort, portNum_t dstPort,
                                 FlowProtocol protocol, ns3::Ptr<ns3::NetDevice> forwardingPort);
    void SetPacketReception ();

private:
    void PacketReceived(ns3::Ptr<ns3::NetDevice> incomingPort, ns3::Ptr<const ns3::Packet> packet,
                        uint16_t protocol, const ns3::Address &src, const ns3::Address &dst,
                        ns3::NetDevice::PacketType packetType);

    struct RtFlow {
        RtFlow () = default;

        RtFlow (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort, portNum_t dstPort,
                FlowProtocol protocol)
        : srcIp{srcIp}, dstIp{dstIp}, srcPort{srcPort}, dstPort{dstPort}, protocol{protocol}
        {
        }

        bool operator< (const RtFlow &other) const;

        uint32_t srcIp{0};
        uint32_t dstIp{0};
        portNum_t srcPort{0};
        portNum_t dstPort{0};
        FlowProtocol protocol {FlowProtocol::Undefined};
    };

    friend std::ostream& operator<<(std::ostream& os, const RtFlow& flow);
    RtFlow ExtractFlowFromPacket (ns3::Ptr<const ns3::Packet> packet, uint16_t protocol);

    std::map<RtFlow, ns3::Ptr<ns3::NetDevice>> m_routingTable;
};

#endif /* ppfs_switch_h */
