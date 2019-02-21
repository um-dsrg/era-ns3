#ifndef ppfs_switch_h
#define ppfs_switch_h

#include "ns3/packet.h"
#include "ns3/net-device.h"
#include "ns3/random-variable-stream.h"

#include "switch-base.h"

class PpfsSwitch : public SwitchBase {

    struct ForwardingAction {
        splitRatio_t splitRatio {0};
        ns3::Ptr<ns3::NetDevice> forwardingPort {nullptr};

        ForwardingAction() = default;
        ForwardingAction(splitRatio_t splitRatio, ns3::Ptr<ns3::NetDevice> port);

        bool operator< (const ForwardingAction& other) const;
    };

    std::map<RtFlow, std::vector<ForwardingAction>> m_routingTable;
    ns3::Ptr<ns3::UniformRandomVariable> m_uniformRandomVariable; /**< Random variable */

public:
    explicit PpfsSwitch(id_t id);
    void AddEntryToRoutingTable(uint32_t srcIp, uint32_t dstIp, portNum_t srcPort,
                                portNum_t dstPort, FlowProtocol protocol, splitRatio_t splitRatio,
                                ns3::Ptr<ns3::NetDevice> forwardingPort);
    void SetPacketReception();
    void ReconcileSplitRatios();

private:
    void PacketReceived(ns3::Ptr<ns3::NetDevice> incomingPort, ns3::Ptr<const ns3::Packet> packet,
                        uint16_t protocol, const ns3::Address &src, const ns3::Address &dst,
                        ns3::NetDevice::PacketType packetType);
};

#endif /* ppfs_switch_h */
