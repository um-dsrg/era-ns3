#ifndef ROUTING_HELPER_H
#define ROUTING_HELPER_H

#include <map>
#include <tinyxml2.h>

#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/node-container.h"

#include "ppfs-switch.h"
#include "topology-builder.h"

class RoutingHelper
{
public:
  void PopulateRoutingTables (std::map<uint32_t, PpfsSwitch>& switchMap,
                              std::map <uint32_t, LinkInformation>& linkInformation,
                              ns3::NodeContainer& allNodes,
                              tinyxml2::XMLNode* rootNode);
  void SetReceiveFunctionForSwitches (ns3::NodeContainer switchNodes);
  void ReceiveFromDevice (ns3::Ptr<ns3::NetDevice> incomingPort, ns3::Ptr<const ns3::Packet> packet,
                          uint16_t protocol, ns3::Address const &src, ns3::Address const &dst,
                          ns3::NetDevice::PacketType packetType);
private:
  void ParseIncomingFlows (std::map<std::pair<uint32_t, uint32_t>, double>& incomingFlow,
                           tinyxml2::XMLNode* rootNode);
  inline uint32_t GetIpAddress (uint32_t nodeId, ns3::NodeContainer& nodes);
};

#endif /* ROUTING_HELPER_H */
