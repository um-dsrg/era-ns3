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

using namespace ns3;

struct Link
{
  id_t id; //!< Link Id
  CustomDevice *srcNode;
  CustomDevice *dstNode;
  NodeType srcNodeType;
  NodeType dstNodeType;
  delay_t delay;
  dataRate_t capacity;
};

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
  std::map<id_t, Ptr<Node>> m_terminals; //!< Maps the node id with the ns3 node.
  /* Refactoring - END */

  std::vector<NetDeviceContainer> m_linkNetDeviceContainers;
  /*
     * When set to true, the tow net devices that make one whole link will be stored in the vector
     * m_linkNetDevicePair. This information will be used to assign IP addresses in the OSPF scenario
     * because each link must have a net "network".
     */
  bool m_storeNetDeviceLinkPairs;

  NS_LOG_TEMPLATE_DECLARE; //!< Logging
};

// #include "topology-builder.cc"

/**
   * Implementation of the TopologyBuilder class
   */

template <class SwitchType>
TopologyBuilder<SwitchType>::TopologyBuilder () : NS_LOG_TEMPLATE_DEFINE ("TopologyBuilder")
{
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::CreateUniqueNode (id_t nodeId, NodeType nodeType)
{
  if (nodeType == NodeType::Switch)
    {
      if (m_switches.find (nodeId) == m_switches.end ())
        {
          NS_LOG_INFO ("Creating the Switch " << nodeId);
          auto ret = m_switches.emplace (nodeId, SwitchType (nodeId));
          NS_ABORT_MSG_IF (ret.second == false,
                           "Trying to insert a duplicate node with id " << nodeId);
        }
    }
  else if (nodeType == NodeType::Terminal)
    {
      if (m_terminals.find (nodeId) == m_terminals.end ())
        {
          NS_LOG_INFO ("Creating the Terminal " << nodeId);
          auto ret = m_terminals.emplace (nodeId, CreateObject<Node> ());
          NS_ABORT_MSG_IF (ret.second == false,
                           "Trying to insert a duplicate node with id " << nodeId);
        }
    }
  else
    NS_ABORT_MSG ("Node Id " << nodeId << " has an invalid node type");
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::CreateNodes (tinyxml2::XMLNode *rootNode)
{
  auto netTopElement = rootNode->FirstChildElement ("NetworkTopology");
  NS_ABORT_MSG_IF (netTopElement == nullptr, "NetworkTopology Element not found");

  auto linkElement = netTopElement->FirstChildElement ("Link");

  while (linkElement != nullptr)
    {
      auto linkDetElement = linkElement->FirstChildElement ("LinkElement");

      while (linkDetElement != nullptr)
        {
          // Create source node if it does not exist
          id_t src_node_id;
          linkDetElement->QueryAttribute ("SourceNode", &src_node_id);
          auto src_node_type =
              static_cast<NodeType> (*linkDetElement->Attribute ("SourceNodeType"));
          CreateUniqueNode (src_node_id, src_node_type);

          // Create destination node if it does not exist
          id_t dst_node_id;
          linkDetElement->QueryAttribute ("DestinationNode", &dst_node_id);
          auto dst_node_type =
              static_cast<NodeType> (*linkDetElement->Attribute ("DestinationNodeType"));
          CreateUniqueNode (dst_node_id, dst_node_type);

          linkDetElement = linkDetElement->NextSiblingElement ("LinkElement");
        }
      linkElement = linkElement->NextSiblingElement ("Link");
    }
}

// FIXME This needs to be removed at the end of the refactoring stage
#include "topology-builder.cc"

#endif /* TOPOLOGY_BUILDER_H */
