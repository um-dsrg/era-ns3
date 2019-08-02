#ifndef sdn_switch_h
#define sdn_switch_h

#include <map>

#include "ns3/queue.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/net-device.h"
#include "ns3/random-variable-stream.h"

#include "switch-base.h"

class SdnSwitch : public SwitchBase
{
public:
  SdnSwitch (id_t id, uint64_t switchBufferSize, const std::string& txBufferRetrievalMethod,
             ResultsContainer &resContainer);

  void AddEntryToRoutingTable (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort, portNum_t dstPort,
                               FlowProtocol protocol, ns3::Ptr<ns3::NetDevice> forwardingPort,
                               splitRatio_t splitRatio) override;
  void SetPacketReception () override;

private:
  void PacketReceived (ns3::Ptr<ns3::NetDevice> incomingPort, ns3::Ptr<const ns3::Packet> packet,
                       uint16_t protocol, const ns3::Address &src, const ns3::Address &dst,
                       ns3::NetDevice::PacketType ndPacketType) override;

  std::map<RtFlow, ns3::Ptr<ns3::NetDevice>> m_routingTable;
};

#endif /* sdn_switch_h */
