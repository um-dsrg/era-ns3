#ifndef ROUTING_HELPER_H
#define ROUTING_HELPER_H

#include <map>
#include <tinyxml2.h>

#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/node-container.h"

#include "../common-code/definitions.h"
#include "topology-builder.h"

template <class SwitchType>
class RoutingHelper
{
public:
  RoutingHelper (std::map<NodeId_t, SwitchType>& switchMap);

  void PopulateRoutingTables (std::map <LinkId_t, LinkInformation>& linkInformation,
                              ns3::NodeContainer& allNodes,
                              tinyxml2::XMLNode* rootNode);

  void SetSwitchesPacketHandler ();

private:
  void ParseIncomingFlows (std::map<std::pair<NodeId_t, FlowId_t>, double>& incomingFlow,
                           tinyxml2::XMLNode* rootNode);
  inline uint32_t GetIpAddress (uint32_t nodeId, ns3::NodeContainer& nodes);

  std::map<NodeId_t, SwitchType>& m_switchMap;
};

#include "routing-helper.tpp"
#endif /* ROUTING_HELPER_H */
