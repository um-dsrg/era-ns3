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
                   ns3::NodeContainer& allNodes);

  void CreateNodes ();
  void CreatePpfsSwitches ();
  void BuildNetworkTopology ();

private:
  void InstallP2pLink (LinkInformation& linkA, LinkInformation& linkB, uint32_t delay);
  void InstallP2pLink (LinkInformation& link, uint32_t delay);

  std::map <uint32_t, LinkInformation> m_linkInformation; // Why is this here?!
  tinyxml2::XMLNode* m_xmlRootNode;
  std::map<uint32_t, PpfsSwitch> & m_switchMap;
  ns3::NodeContainer& m_allNodes;
};

#endif /* TOPOLOGY_BUILDER_H */
