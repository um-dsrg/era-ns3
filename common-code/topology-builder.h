#ifndef TOPOLOGY_BUILDER_H
#define TOPOLOGY_BUILDER_H

#include <map>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <tinyxml2.h>

#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/object-factory.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-net-device.h"

#include "definitions.h"
#include "custom-device.h"
#include "terminal.h"

using namespace ns3;
using namespace tinyxml2;

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

  void CreateNodes (XMLNode *rootNode);
  void BuildNetworkTopology (XMLNode *rootNode);
  void AssignIpToTerminals ();

  /* The below public functions still need to be optimised */
  // Parses the node configuration element and creates PpfsSwitches instances
  void ParseNodeConfiguration ();
  // linkinformation is output.

  void SetSwitchRandomNumberGenerator ();
  void AssignIpToNodes ();

private:
  void CreateUniqueNode (id_t nodeId, NodeType nodeType);
  void InstallP2pLinks (const std::vector<Link> &links);
  CustomDevice *GetNode (id_t id, NodeType nodeType);
  // void InstallP2pLink (LinkInformation &linkA, LinkInformation &linkB, uint32_t delay);
  // void InstallP2pLink (LinkInformation &link, uint32_t delay);

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
  std::map<id_t, Terminal> m_terminals; //!< Maps the node id with the Terminal object.
  /**
   * A map that given a link id will return the Node Id and the NetDevice that needs to be
   * used to transmit on the given link.
   */
  // std::map<id_t, std::pair<id_t, ns3::Ptr<ns3::PointToPointNetDevice>>> m_transmit_on_link;
  std::map<id_t, std::pair<id_t, Ptr<NetDevice>>> m_transmitOnLink;
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

void ShuffleLinkElements (std::vector<XMLElement *> &linkElements);

/**
   * Implementation of the TopologyBuilder class
   */

template <class SwitchType>
TopologyBuilder<SwitchType>::TopologyBuilder () : NS_LOG_TEMPLATE_DEFINE ("TopologyBuilder")
{
}

template <class SwitchType>
CustomDevice *
TopologyBuilder<SwitchType>::GetNode (id_t id, NodeType nodeType)
{
  try
    {
      if (nodeType == NodeType::Switch)
        return &m_switches.at (id);
      else if (nodeType == NodeType::Terminal)
        return &m_terminals.at (id);
      else
        NS_ABORT_MSG ("Node " << id << " type not found");
  } catch (const std::out_of_range &oor)
    {
      NS_ABORT_MSG ("The node " << id << " has not been found.");
  }
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
          auto ret = m_terminals.emplace (nodeId, Terminal (nodeId));
          NS_ABORT_MSG_IF (ret.second == false,
                           "Trying to insert a duplicate node with id " << nodeId);
        }
    }
  else
    NS_ABORT_MSG ("Node Id " << nodeId << " has an invalid node type");
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::CreateNodes (XMLNode *rootNode)
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

template <class SwitchType>
void
TopologyBuilder<SwitchType>::BuildNetworkTopology (XMLNode *rootNode)
{
  XMLElement *networkTopologyElement = rootNode->FirstChildElement ("NetworkTopology");
  NS_ABORT_MSG_IF (networkTopologyElement == nullptr, "NetworkTopology Element not found");

  // Store the pointers to the link elements such that the order in which
  // links are built can be shuffled.
  std::vector<XMLElement *> linkElements;
  XMLElement *linkElement = networkTopologyElement->FirstChildElement ("Link");
  while (linkElement != nullptr)
    {
      linkElements.push_back (linkElement);
      linkElement = linkElement->NextSiblingElement ("Link");
    }

  ShuffleLinkElements (linkElements);

  delay_t linkDelay;
  for (auto linkElement : linkElements)
    {
      linkElement->QueryAttribute ("Delay", &linkDelay);

      // We need to loop through all the Link Elements here.
      XMLElement *linkElementElement = linkElement->FirstChildElement ("LinkElement");

      std::vector<Link> sameDelayLinks;
      while (linkElementElement != nullptr)
        {
          Link link;
          link.delay = linkDelay;
          linkElementElement->QueryAttribute ("Id", &link.id);
          linkElementElement->QueryAttribute ("Capacity", &link.capacity);

          // Source Node
          uint32_t srcNodeId;
          linkElementElement->QueryAttribute ("SourceNode", &srcNodeId);
          link.srcNodeType =
              static_cast<NodeType> (*linkElementElement->Attribute ("SourceNodeType"));
          link.srcNode = GetNode (srcNodeId, link.srcNodeType);

          // Destination Node
          uint32_t dstNodeId;
          linkElementElement->QueryAttribute ("DestinationNode", &dstNodeId);
          link.dstNodeType =
              static_cast<NodeType> (*linkElementElement->Attribute ("DestinationNodeType"));
          link.dstNode = GetNode (dstNodeId, link.dstNodeType);

          // Save link
          sameDelayLinks.push_back (link);
          linkElementElement = linkElementElement->NextSiblingElement ("LinkElement");
        }

      InstallP2pLinks (sameDelayLinks);
      linkElement = linkElement->NextSiblingElement ("Link");
    }
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::InstallP2pLinks (const std::vector<Link> &links)
{
  NS_ABORT_MSG_IF (links.size () > 2 || links.size () < 1,
                   "The number of same delay links should be between 1 and 2");

  NS_LOG_INFO ("Creating link " << links[0].id);
  if (links.size () == 2)
    {
      NS_LOG_INFO ("Creating link " << links[1].id);
      /* Check that the two links are in fact bidirectional */
      NS_ASSERT_MSG (links[0].delay == links[1].delay, "The link delay values do not match");
      NS_ASSERT_MSG (links[0].srcNode->GetId () == links[1].dstNode->GetId (),
                     "Link A source node is not equal to Link B destination node");
      NS_ASSERT_MSG (links[0].dstNode->GetId () == links[1].srcNode->GetId (),
                     "Link A destination node is not equal to Link B source node");
    }

  // If two links are present, they have been verified to be bidirectional.
  // From this point onwards the first link will be used to build the Point
  // To Point link, with the exception of the data rate values as they might
  // be different.
  const auto &commonLink{links[0]};

  // Device Configuration
  ObjectFactory deviceFactory ("ns3::PointToPointNetDevice");

  // Create NetDevice on source node
  deviceFactory.Set ("DataRate", DataRateValue (DataRate (commonLink.capacity * 1'000'000)));
  Ptr<PointToPointNetDevice> srcNd = deviceFactory.Create<PointToPointNetDevice> ();
  srcNd->SetAddress (Mac48Address::Allocate ());
  commonLink.srcNode->GetNode ()->AddDevice (srcNd);

  // Create NetDevice on destination node
  if (links.size () == 2)
    deviceFactory.Set ("DataRate", DataRateValue (DataRate (links[1].capacity * 1'000'000)));
  Ptr<PointToPointNetDevice> dstNd = deviceFactory.Create<PointToPointNetDevice> ();
  dstNd->SetAddress (Mac48Address::Allocate ());
  commonLink.dstNode->GetNode ()->AddDevice (dstNd);

  // Create Queue on source NetDevice
  ObjectFactory queueFactory ("ns3::DropTailQueue<Packet>");
  Ptr<Queue<Packet>> srcQueue = queueFactory.Create<Queue<Packet>> ();
  srcNd->SetQueue (srcQueue);

  // Create Queue on destination NetDevice
  Ptr<Queue<Packet>> dstQueue = queueFactory.Create<Queue<Packet>> ();
  dstNd->SetQueue (dstQueue);

  // Connect Net Devices together via the Point To Point channel
  ObjectFactory channelFactory ("ns3::PointToPointChannel");
  channelFactory.Set ("Delay", TimeValue (MilliSeconds (commonLink.delay)));
  Ptr<PointToPointChannel> channel = channelFactory.Create<PointToPointChannel> ();
  srcNd->Attach (channel);
  dstNd->Attach (channel);

  auto ret = m_transmitOnLink.emplace (commonLink.id,
                                       std::make_pair (commonLink.srcNode->GetId (), srcNd));
  NS_ABORT_MSG_IF (ret.second == false,
                   "Duplicate link id " << commonLink.id << " in transmit on link map");

  if (links.size () == 2)
    {
      auto ret = m_transmitOnLink.emplace (links[1].id,
                                           std::make_pair (links[1].srcNode->GetId (), dstNd));
      NS_ABORT_MSG_IF (ret.second == false,
                       "Duplicate link id " << links[1].id << " in transmit on link map");
    }
}

/**
 * \brief      Shuffles the XML Link Elements in place.
 *
 * \param      linkElements  The link elements
 */
void
ShuffleLinkElements (std::vector<XMLElement *> &linkElements)
{
  // TODO Test that this is working
  std::random_shuffle (linkElements.begin (), linkElements.end ());
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::AssignIpToTerminals ()
{
  InternetStackHelper stack;
  NetDeviceContainer terminalDevices; //!< Contain all the terminal's NetDevices

  for (auto &terminalPair : m_terminals)
    {
      auto &terminalNode = terminalPair.second.GetNode ();

      NS_ABORT_MSG_IF (terminalNode->GetNDevices () != 1,
                       "Terminal " << terminalPair.first << " has " << terminalNode->GetNDevices ()
                                   << " NetDevice(s).");
      terminalDevices.Add (terminalNode->GetDevice (0));

      // Store the Node NetDevice before installing the IP Stack as this will add an additional
      // NetDevice to the node. From the documentation: The LoopbackNetDevice is automatically
      // added to any node as soon as the Internet stack is initialized.
      NS_LOG_INFO ("Installing the IP Stack on terminal " << terminalPair.first);
      stack.Install (terminalNode);
    }

  NS_LOG_INFO ("Assigning IP Addresses to each terminal");
  Ipv4AddressHelper ipAddrHelper;
  ipAddrHelper.SetBase ("0.0.0.0", "255.0.0.0");
  ipAddrHelper.Assign (terminalDevices);

  for (auto &terminalPair : m_terminals)
    {
      terminalPair.second.RetrieveIpAddress ();
    }
}

// FIXME This needs to be removed at the end of the refactoring stage
#include "topology-builder.cc"

#endif /* TOPOLOGY_BUILDER_H */
