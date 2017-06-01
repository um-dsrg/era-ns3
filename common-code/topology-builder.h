#ifndef TOPOLOGY_BUILDER_H
#define TOPOLOGY_BUILDER_H

#include <tinyxml2.h>
#include <cstdint>
#include <map>

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/queue.h"

#include "../common-code/definitions.h"

struct LinkInformation
{
  uint32_t linkId;
  uint32_t srcNode;
  char srcNodeType;
  uint32_t dstNode;
  char dstNodeType;
  double capacity;
};

template <class SwitchType>
class TopologyBuilder
{
public:
  TopologyBuilder (tinyxml2::XMLNode* xmlRootNode,
                   std::map<NodeId_t, SwitchType>& switchMap,
                   std::map<ns3::Ptr<ns3::NetDevice>, LinkId_t>& terminalToLinkId,
                   ns3::NodeContainer& allNodes,
                   ns3::NodeContainer& terminalNodes,
                   ns3::NodeContainer& switchNodes,
                   ns3::NetDeviceContainer& terminalDevices);

  void CreateNodes ();
  // Parses the node configuration element and creates PpfsSwitches instances
  void ParseNodeConfiguration ();
  // linkinformation is output.
  void BuildNetworkTopology (std::map <LinkId_t, LinkInformation>& linkInformation);
  void SetSwitchRandomNumberGenerator (uint32_t seed, uint32_t initRun);
  void AssignIpToNodes (bool assignToTerminals, bool assignToSwitches);

private:
  void InstallP2pLink (LinkInformation& linkA, LinkInformation& linkB, uint32_t delay);
  void InstallP2pLink (LinkInformation& link, uint32_t delay);

  tinyxml2::XMLNode* m_xmlRootNode;
  std::map<NodeId_t, SwitchType> & m_switchMap;
  std::map<ns3::Ptr<ns3::NetDevice>, LinkId_t>& m_terminalToLinkId;
  ns3::NodeContainer& m_allNodes;
  ns3::NodeContainer& m_terminalNodes;
  ns3::NodeContainer& m_switchNodes;
  ns3::NetDeviceContainer& m_terminalDevices;
  ns3::NetDeviceContainer m_switchDevices;
};

#include "topology-builder.cc"

#endif /* TOPOLOGY_BUILDER_H */
