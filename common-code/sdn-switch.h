#ifndef sdn_switch_h
#define sdn_switch_h

#include <map>

#include "ns3/queue.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/net-device.h"
#include "ns3/random-variable-stream.h"

#include "switch-base.h"

class SdnSwitch : public SwitchBase {
public:
    SdnSwitch(id_t id);

    void AddEntryToRoutingTable(uint32_t srcIp, uint32_t dstIp, portNum_t srcPort, portNum_t dstPort,
                                FlowProtocol protocol, ns3::Ptr<ns3::NetDevice> forwardingPort);
    void SetPacketReception();

    /**
     * Store the number of packets in the queue for each net device available on the switch.
     */
    std::map<ns3::Ptr<ns3::NetDevice>, std::list<uint32_t>> m_netDeviceQueueLog;

private:
    void PacketReceived(ns3::Ptr<ns3::NetDevice> incomingPort, ns3::Ptr<const ns3::Packet> packet,
                        uint16_t protocol, const ns3::Address &src, const ns3::Address &dst,
                        ns3::NetDevice::PacketType packetType);

    std::map<RtFlow, ns3::Ptr<ns3::NetDevice>> m_routingTable;
};

#endif /* sdn_switch_h */
