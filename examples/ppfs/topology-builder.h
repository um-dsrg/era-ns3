#ifndef TOPOLOGY_BUILDER_H
#define TOPOLOGY_BUILDER_H

#include <tinyxml2.h>
#include <cstdint>
#include <map>

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/queue.h"

#include "ppfs-switch.h"

struct LinkInformation
{
  uint32_t linkId;
  uint32_t srcNode;
  char srcNodeType;
  uint32_t dstNode;
  char dstNodeType;
  double capacity;
};

class TopologyBuilder
{
public:
  TopologyBuilder (tinyxml2::XMLNode* xmlRootNode,
                   std::map<uint32_t, PpfsSwitch> &switchMap,
                   ns3::NodeContainer& allNodes,
                   ns3::NodeContainer& terminalNodes,
                   ns3::NodeContainer& switchNodes);

  void CreateNodes ();
  // Parses the node configuration element and creates PpfsSwitches instances
  void ParseNodeConfiguration ();
  // linkinformation is output.
  void BuildNetworkTopology (std::map <uint32_t, LinkInformation>& linkInformation);
  void AssignIpToTerminals ();

private:
  void InstallP2pLink (LinkInformation& linkA, LinkInformation& linkB, uint32_t delay);
  void InstallP2pLink (LinkInformation& link, uint32_t delay);

  tinyxml2::XMLNode* m_xmlRootNode;
  std::map<uint32_t, PpfsSwitch> & m_switchMap;
  ns3::NodeContainer& m_allNodes;
  ns3::NodeContainer& m_terminalNodes;
  ns3::NodeContainer& m_switchNodes;
  ns3::NetDeviceContainer m_terminalDevices;
};

#endif /* TOPOLOGY_BUILDER_H */
