#ifndef TOPOLOGY_BUILDER_H
#define TOPOLOGY_BUILDER_H

#include <tinyxml2.h>
#include <cstdint>
#include <map>
#include <vector>

#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"

#include "definitions.h"

template <class SwitchType>
class TopologyBuilder
{
public:
  // TopologyBuilder (tinyxml2::XMLNode *xmlRootNode, std::map<NodeId_t, SwitchType> &switchMap,
  //                  std::map<ns3::Ptr<ns3::NetDevice>, LinkId_t> &terminalToLinkId,
  //                  ns3::NodeContainer &allNodes, ns3::NodeContainer &terminalNodes,
  //                  ns3::NodeContainer &switchNodes, ns3::NetDeviceContainer &terminalDevices,
  //                  bool storeNetDeviceLinkPairs);

  TopologyBuilder ();

  void CreateNodes (tinyxml2::XMLNode *rootNode);
  // Parses the node configuration element and creates PpfsSwitches instances
  void ParseNodeConfiguration ();
  // linkinformation is output.
  void BuildNetworkTopology (std::map<LinkId_t, LinkInformation> &linkInformation);
  void SetSwitchRandomNumberGenerator ();
  void AssignIpToNodes ();

private:
  void CreateUniqueNode (id_t nodeId, NodeType nodeType);
  void ShuffleLinkElements (std::vector<tinyxml2::XMLElement *> &linkElements);
  void InstallP2pLink (LinkInformation &linkA, LinkInformation &linkB, uint32_t delay);
  void InstallP2pLink (LinkInformation &link, uint32_t delay);

  // tinyxml2::XMLNode *m_xmlRootNode;
  /* Refactoring - BEGIN */
  // std::map<ns3::Ptr<ns3::NetDevice>, LinkId_t> &m_terminalToLinkId;
  // ns3::NetDeviceContainer &m_terminalDevices;
  // ns3::NetDeviceContainer m_switchDevices;
  // std::map<NodeId_t, SwitchType> &m_switchMap;
  // ns3::NodeContainer &m_allNodes;
  // ns3::NodeContainer &m_terminalNodes;
  // ns3::NodeContainer &m_switchNodes;
  std::map<id_t, SwitchType> m_switches; //!< Maps the node id with the Switch object.
  std::map<id_t, ns3::Ptr<ns3::Node>> m_terminals; //!< Maps the node id with the ns3 node.
  /* Refactoring - END */

  std::vector<ns3::NetDeviceContainer> m_linkNetDeviceContainers;
  /*
   * When set to true, the tow net devices that make one whole link will be stored in the vector
   * m_linkNetDevicePair. This information will be used to assign IP addresses in the OSPF scenario
   * because each link must have a net "network".
   */
  bool m_storeNetDeviceLinkPairs;

  ns3::NS_LOG_TEMPLATE_DECLARE; //!< Logging
};

#include "topology-builder.cc"

#endif /* TOPOLOGY_BUILDER_H */
