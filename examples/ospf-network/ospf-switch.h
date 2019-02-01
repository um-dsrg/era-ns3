#ifndef OSPF_SWITCH_H
#define OSPF_SWITCH_H

#include "ns3/node.h"
#include "ns3/packet.h"

#include "../common-code/switch-base.h"

class OspfSwitch : public SwitchBase
{
public:
  OspfSwitch () = default;
  OspfSwitch (id_t id);

  void SetPacketHandlingMechanism ();
  void InsertEntryInRoutingTable (uint32_t srcIpAddr, uint32_t dstIpAddr, uint16_t portNumber,
                                  char protocol, FlowId_t flowId);

private:
  void LogPacketTransmission (std::string context, ns3::Ptr<const ns3::Packet> packet);
  std::map<Flow, FlowId_t> m_routingTable;
};

#endif /* OSPF_SWITCH_H */
