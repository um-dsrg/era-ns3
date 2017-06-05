#ifndef OSPF_SWITCH_H
#define OSPF_SWITCH_H

#include "ns3/node.h"
#include "ns3/packet.h"

#include "../common-code/switch-device.h"

class OspfSwitch : public SwitchDevice
{
public:
  OspfSwitch ();
  OspfSwitch (NodeId_t id, ns3::Ptr<ns3::Node> switchNode);

  void SetPacketHandlingMechanism ();
  void InsertEntryInRoutingTable (uint32_t srcIpAddr, uint32_t dstIpAddr, uint16_t portNumber,
                                  char protocol, uint32_t flowId);
private:
  void LogPacketTransmission (std::string context, ns3::Ptr<const ns3::Packet> packet);
  std::map <Flow, FlowId_t> m_routingTable;
};

#endif /* OSPF_SWITCH_H */
