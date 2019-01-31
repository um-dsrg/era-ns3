#include <assert.h>
#include <vector>

#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/object-factory.h"
#include "ns3/string.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/internet-module.h"

#include "random-generator-manager.h"
#include "../examples/ppfs/ppfs-switch.h"
#include "../examples/ospf-network/ospf-switch.h"
#include "topology-builder.h"

using namespace ns3;
using namespace tinyxml2;

// template <class SwitchType>
// TopologyBuilder<SwitchType>::TopologyBuilder (XMLNode *xmlRootNode,
//                                               std::map<uint32_t, SwitchType> &switchMap,
//                                               std::map<Ptr<NetDevice>, uint32_t> &terminalToLinkId,
//                                               NodeContainer &allNodes, NodeContainer &terminalNodes,
//                                               NodeContainer &switchNodes,
//                                               NetDeviceContainer &terminalDevices,
//                                               bool storeNetDeviceLinkPairs)
//     : m_xmlRootNode (xmlRootNode),
//       m_switchMap (switchMap),
//       m_terminalToLinkId (terminalToLinkId),
//       m_allNodes (allNodes),
//       m_terminalNodes (terminalNodes),
//       m_switchNodes (switchNodes),
//       m_terminalDevices (terminalDevices),
//       m_storeNetDeviceLinkPairs (storeNetDeviceLinkPairs)
// {
// }
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

// template <class SwitchType>
// void
// TopologyBuilder<SwitchType>::CreateNodes ()
// {
//   XMLElement *networkTopologyElement = m_xmlRootNode->FirstChildElement ("NetworkTopology");
//   NS_ABORT_MSG_IF (networkTopologyElement == nullptr, "NetworkTopology Element not found");

//   uint32_t numOfNodes;
//   networkTopologyElement->QueryAttribute ("NumberOfNodes", &numOfNodes);
//   m_allNodes.Create (numOfNodes);
// }

// template <class SwitchType>
// void
// TopologyBuilder<SwitchType>::ParseNodeConfiguration ()
// {
//   XMLElement *nodeConfigurationElement = m_xmlRootNode->FirstChildElement ("NodeConfiguration");
//   NS_ABORT_MSG_IF (nodeConfigurationElement == nullptr, "NodeConfiguration Element not found");

//   XMLElement *nodeElement = nodeConfigurationElement->FirstChildElement ("Node");
//   while (nodeElement != nullptr)
//     {
//       NodeId_t nodeId;
//       char nodeType;

//       nodeElement->QueryAttribute ("Id", &nodeId);
//       nodeType = *nodeElement->Attribute ("Type");

//       if (nodeType == 'S')
//         {
//           Ptr<Node> switchNode = m_allNodes.Get (nodeId);
//           auto ret = m_switchMap.insert ({nodeId, SwitchType (nodeId, switchNode)});
//           NS_ABORT_MSG_IF (ret.second == false, "A switch with Id " << nodeId << " exists already");
//           m_switchNodes.Add (switchNode); // Add the node to the switches node container
//         }
//       else if (nodeType == 'T')
//         m_terminalNodes.Add (
//             m_allNodes.Get (nodeId)); // Add the node to the terminals node container
//       else
//         NS_ABORT_MSG ("Unknown node type encountered");

//       nodeElement = nodeElement->NextSiblingElement ("Node");
//     }
// }

template <class SwitchType>
void
TopologyBuilder<SwitchType>::BuildNetworkTopology (
    std::map<LinkId_t, LinkInformation> &linkInformation)
{
  // XMLElement *networkTopologyElement = m_xmlRootNode->FirstChildElement ("NetworkTopology");
  // NS_ABORT_MSG_IF (networkTopologyElement == nullptr, "NetworkTopology Element not found");

  // /*
  //  * Looping through all the link Elements and storing them in a vector such that they can be
  //  * shuffled
  //  */
  // std::vector<XMLElement *> linkElements;
  // XMLElement *linkElement = networkTopologyElement->FirstChildElement ("Link");
  // while (linkElement != nullptr)
  //   {
  //     linkElements.push_back (linkElement);
  //     linkElement = linkElement->NextSiblingElement ("Link");
  //   }

  // ShuffleLinkElements (linkElements);

  // uint32_t linkDelay;
  // for (auto linkElement : linkElements)
  //   {
  //     linkElement->QueryAttribute ("Delay", &linkDelay);

  //     // We need to loop through all the Link Elements here.
  //     XMLElement *linkElementElement = linkElement->FirstChildElement ("LinkElement");

  //     int linkIds[] = {-1, -1};

  //     int linkElementCounter = 0;
  //     while (linkElementElement != nullptr)
  //       {
  //         LinkInformation linkInfo;

  //         linkElementElement->QueryAttribute ("Id", &linkInfo.linkId);
  //         linkElementElement->QueryAttribute ("SourceNode", &linkInfo.srcNode);
  //         linkInfo.srcNodeType = *linkElementElement->Attribute ("SourceNodeType");
  //         linkElementElement->QueryAttribute ("DestinationNode", &linkInfo.dstNode);
  //         linkInfo.dstNodeType = *linkElementElement->Attribute ("DestinationNodeType");
  //         linkElementElement->QueryAttribute ("Capacity", &linkInfo.capacity);

  //         auto ret = linkInformation.insert ({linkInfo.linkId, linkInfo});
  //         NS_ABORT_MSG_IF (ret.second == false, "Link ID " << linkInfo.linkId << " is duplicate");

  //         linkIds[linkElementCounter++] = linkInfo.linkId;
  //         linkElementElement = linkElementElement->NextSiblingElement ("LinkElement");
  //       }

  //     if (linkIds[1] == -1)
  //       InstallP2pLink (linkInformation[linkIds[0]], linkDelay);
  //     else
  //       InstallP2pLink (linkInformation[linkIds[0]], linkInformation[linkIds[1]], linkDelay);

  //     linkElement = linkElement->NextSiblingElement ("Link");
  //   }
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::SetSwitchRandomNumberGenerator ()
{
  // for (auto &switchNode : m_switchMap)
  //   {
  //     switchNode.second.SetRandomNumberGenerator ();
  //   }
}

template <>
void
TopologyBuilder<PpfsSwitch>::AssignIpToNodes ()
{
  // NS_ABORT_MSG_IF (
  //     (m_linkNetDeviceContainers.size () > 0 || m_storeNetDeviceLinkPairs),
  //     "When running a PPFS simulation, you *SHOULD* not store the net device link pairs"
  //     ". That feature is only available to OSPF simulations.");

  // InternetStackHelper internet;
  // Ipv4AddressHelper ipv4;
  // ipv4.SetBase ("0.0.0.0", "255.0.0.0");

  // internet.Install (m_terminalNodes); //Add internet stack to the terminals ONLY
  // ipv4.Assign (m_terminalDevices);
}

template <>
void
TopologyBuilder<OspfSwitch>::AssignIpToNodes ()
{
  // InternetStackHelper internet;
  // Ipv4AddressHelper ipv4;
  // ipv4.SetBase ("0.0.0.0", "255.255.255.252");

  // //Add the internet stack to switches and terminals
  // internet.Install (m_terminalNodes);
  // internet.Install (m_switchNodes);

  // for (auto &netDeviceContainer : m_linkNetDeviceContainers)
  //   {
  //     ipv4.Assign (netDeviceContainer);
  //     ipv4.NewNetwork ();
  //   }

  // // Freeing the vector's memory as this will no longer be required.
  // m_linkNetDeviceContainers.clear ();
  // std::vector<ns3::NetDeviceContainer> ().swap (m_linkNetDeviceContainers);
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::ShuffleLinkElements (std::vector<XMLElement *> &linkElements)
{
  typedef
      typename std::iterator_traits<std::vector<XMLElement *>::iterator>::difference_type diff_t;

  Ptr<UniformRandomVariable> randomGenerator =
      RandomGeneratorManager::CreateUniformRandomVariable (0, 0);

  diff_t numOfLinkElements = linkElements.end () - linkElements.begin ();

  for (diff_t i = numOfLinkElements - 1; i > 0; --i)
    {
      // I think this is inclusive. Verify
      uint32_t randValue = randomGenerator->GetInteger (0, i);
      std::swap (linkElements[i], linkElements[randValue]);
    }
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::InstallP2pLink (LinkInformation &linkA, LinkInformation &linkB,
                                             uint32_t delay)
{
  ObjectFactory m_channelFactory ("ns3::PointToPointChannel");
  ObjectFactory m_deviceFactory ("ns3::PointToPointNetDevice");
  ObjectFactory m_queueFactory ("ns3::DropTailQueue<Packet>");

  // // A single delay for the link.
  // m_channelFactory.Set ("Delay", TimeValue (MilliSeconds (delay)));

  // // Setting up link A ////////////////////////////////////////////////////////
  // Ptr<Node> nodeA = m_allNodes.Get (linkA.srcNode);
  // m_deviceFactory.Set ("DataRate", DataRateValue (DataRate (linkA.capacity * 1000000))); // In bps
  // Ptr<PointToPointNetDevice> devA = m_deviceFactory.Create<PointToPointNetDevice> ();
  // devA->SetAddress (Mac48Address::Allocate ());
  // nodeA->AddDevice (devA);
  // Ptr<Queue<Packet>> queueA = m_queueFactory.Create<Queue<Packet>> ();
  // devA->SetQueue (queueA);
  // /*
  //  * Inserting a reference to the net device in the switch. This information will be
  //  * used when building the routing table.
  //  */
  // if (linkA.srcNodeType == 'S')
  //   {
  //     m_switchMap[linkA.srcNode].InsertNetDevice (linkA.linkId, devA);
  //     m_switchDevices.Add (devA);
  //   }
  // else if (linkA.srcNodeType == 'T')
  //   {
  //     m_terminalToLinkId.insert ({devA, linkA.linkId});
  //     m_terminalDevices.Add (devA);
  //   }

  // // Setting up link B ////////////////////////////////////////////////////////
  // Ptr<Node> nodeB = m_allNodes.Get (linkB.srcNode);
  // m_deviceFactory.Set ("DataRate", DataRateValue (DataRate (linkB.capacity * 1000000))); // In bps
  // Ptr<PointToPointNetDevice> devB = m_deviceFactory.Create<PointToPointNetDevice> ();
  // devB->SetAddress (Mac48Address::Allocate ());
  // nodeB->AddDevice (devB);
  // Ptr<Queue<Packet>> queueB = m_queueFactory.Create<Queue<Packet>> ();
  // devB->SetQueue (queueB);
  // /*
  //  * Inserting a reference to the net device in the switch. This information will be
  //  * used when building the routing table.
  //  */
  // if (linkB.srcNodeType == 'S')
  //   {
  //     m_switchMap[linkB.srcNode].InsertNetDevice (linkB.linkId, devB);
  //     m_switchDevices.Add (devB);
  //   }
  // else if (linkB.srcNodeType == 'T')
  //   {
  //     m_terminalToLinkId.insert ({devB, linkB.linkId});
  //     m_terminalDevices.Add (devB);
  //   }

  // Ptr<PointToPointChannel> channel = m_channelFactory.Create<PointToPointChannel> ();
  // devA->Attach (channel);
  // devB->Attach (channel);

  // if (m_storeNetDeviceLinkPairs) // Storing the link net device pairs
  //   {
  //     NetDeviceContainer netDeviceContainer;
  //     netDeviceContainer.Add (devA);
  //     netDeviceContainer.Add (devB);
  //     m_linkNetDeviceContainers.push_back (netDeviceContainer);
  //   }
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::InstallP2pLink (LinkInformation &link, uint32_t delay)
{
  ObjectFactory channelFactory ("ns3::PointToPointChannel");
  ObjectFactory deviceFactory ("ns3::PointToPointNetDevice");
  ObjectFactory queueFactory ("ns3::DropTailQueue");

  // channelFactory.Set ("Delay", TimeValue (MilliSeconds (delay)));
  // // Setting the data rate in bps
  // deviceFactory.Set ("DataRate", DataRateValue (DataRate (link.capacity * 1000000)));

  // // Setting up link A ////////////////////////////////////////////////////////
  // Ptr<Node> nodeA = m_allNodes.Get (link.srcNode);
  // Ptr<PointToPointNetDevice> devA = deviceFactory.Create<PointToPointNetDevice> ();
  // devA->SetAddress (Mac48Address::Allocate ());
  // nodeA->AddDevice (devA);
  // Ptr<Queue<Packet>> queueA = queueFactory.Create<Queue<Packet>> ();
  // devA->SetQueue (queueA);
  // // Inserting a reference to the net device in the switch. This information will be
  // // used when building the routing table.
  // if (link.srcNodeType == 'S')
  //   {
  //     m_switchMap[link.srcNode].InsertNetDevice (link.linkId, devA);
  //     m_switchDevices.Add (devA);
  //   }
  // else if (link.srcNodeType == 'T')
  //   {
  //     m_terminalToLinkId.insert ({devA, link.linkId});
  //     m_terminalDevices.Add (devA);
  //   }

  // // Setting up link B ////////////////////////////////////////////////////////
  // Ptr<Node> nodeB = m_allNodes.Get (link.dstNode);
  // Ptr<PointToPointNetDevice> devB = deviceFactory.Create<PointToPointNetDevice> ();
  // devB->SetAddress (Mac48Address::Allocate ());
  // nodeB->AddDevice (devB);
  // Ptr<Queue<Packet>> queueB = queueFactory.Create<Queue<Packet>> ();
  // queueB->SetMaxSize (
  //     QueueSize (QueueSizeUnit::PACKETS,
  //                0)); /**< Sanity check to make sure no packets are sent from this device */
  // devB->SetQueue (queueB);

  // if (link.dstNodeType == 'S')
  //   m_switchDevices.Add (devB);
  // else if (link.dstNodeType == 'T')
  //   m_terminalDevices.Add (devB);

  // Ptr<PointToPointChannel> channel = channelFactory.Create<PointToPointChannel> ();
  // devA->Attach (channel);
  // devB->Attach (channel);

  // if (m_storeNetDeviceLinkPairs) // Storing the link net device pairs
  //   {
  //     NetDeviceContainer netDeviceContainer;
  //     netDeviceContainer.Add (devA);
  //     netDeviceContainer.Add (devB);
  //     m_linkNetDeviceContainers.push_back (netDeviceContainer);
  //   }
}
