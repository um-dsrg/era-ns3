#include <assert.h>

#include "ns3/abort.h"
#include "ns3/object-factory.h"
#include "ns3/string.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-net-device.h"

#include "topology-builder.h"

using namespace ns3;
using namespace tinyxml2;

TopologyBuilder::TopologyBuilder (XMLNode* xmlRootNode, std::map<uint32_t, PpfsSwitch> &switchMap,
                                  NodeContainer& allNodes) :
  m_xmlRootNode(xmlRootNode), m_switchMap(switchMap), m_allNodes(allNodes)
{}

void
TopologyBuilder::CreateNodes()
{
  XMLElement* networkTopologyElement = m_xmlRootNode->FirstChildElement("NetworkTopology");
  NS_ABORT_MSG_IF(networkTopologyElement == nullptr, "NetworkTopology Element not found");

  uint32_t numOfNodes;
  networkTopologyElement->QueryAttribute("NumberOfNodes", &numOfNodes);
  m_allNodes.Create(numOfNodes);
}

void
TopologyBuilder::CreatePpfsSwitches()
{
  XMLElement* nodeConfigurationElement = m_xmlRootNode->FirstChildElement("NodeConfiguration");
  NS_ABORT_MSG_IF(nodeConfigurationElement == nullptr, "NodeConfiguration Element not found");

  XMLElement* nodeElement = nodeConfigurationElement->FirstChildElement("Node");
  while (nodeElement != nullptr)
    {
      uint32_t nodeId;
      char nodeType;

      nodeElement->QueryAttribute("Id", &nodeId);
      nodeType = *nodeElement->Attribute("Type");

      if (nodeType == 'S')
        {
          auto ret = m_switchMap.insert({nodeId, PpfsSwitch(nodeId)});
          NS_ABORT_MSG_IF(ret.second == false, "A switch with Id " << nodeId << " exists already");
        }
      nodeElement = nodeElement->NextSiblingElement("Node");
    }
}

void
TopologyBuilder::BuildNetworkTopology()
{
  XMLElement* networkTopologyElement = m_xmlRootNode->FirstChildElement("NetworkTopology");
  NS_ABORT_MSG_IF(networkTopologyElement == nullptr, "NetworkTopology Element not found");

  uint32_t linkDelay;
  XMLElement* linkElement = networkTopologyElement->FirstChildElement("Link");
  while (linkElement != nullptr) // Looping through all the link Elements.
    {
      linkElement->QueryAttribute("Delay", &linkDelay);
      std::cout << "Link delay " << linkDelay << std::endl;

      // We need to loop through all the Link Elements here.
      XMLElement* linkElementElement = linkElement->FirstChildElement("LinkElement");

      int linkIds [] = {-1, -1};

      int linkElementCounter = 0;
      while (linkElementElement != nullptr)
        {
          // TODO: Put this in a function!
          LinkInformation linkInfo;

          linkElementElement->QueryAttribute("Id", &linkInfo.linkId);
          linkElementElement->QueryAttribute("SourceNode", &linkInfo.srcNode);
          linkInfo.srcNodeType = *linkElementElement->Attribute("SourceNodeType");
          linkElementElement->QueryAttribute("DestinationNode", &linkInfo.dstNode);
          linkInfo.dstNodeType = *linkElementElement->Attribute("DestinationNodeType");
          linkElementElement->QueryAttribute("Capacity", &linkInfo.capacity);

          auto ret = m_linkInformation.insert({linkInfo.linkId, linkInfo});
          NS_ABORT_MSG_IF(ret.second == false, "Link ID " << linkInfo.linkId << " is duplicate");

          linkIds[linkElementCounter++] = linkInfo.linkId;
          linkElementElement = linkElementElement->NextSiblingElement("LinkElement");
        }

      if (linkIds[1] == -1)
        InstallP2pLink(m_linkInformation[linkIds[0]], linkDelay);
      else
        InstallP2pLink(m_linkInformation[linkIds[0]], m_linkInformation[linkIds[1]], linkDelay);

      linkElement = linkElement->NextSiblingElement("Link");
    }
}

void
TopologyBuilder::InstallP2pLink (LinkInformation& linkA, LinkInformation& linkB, uint32_t delay)
{
  ObjectFactory m_channelFactory("ns3::PointToPointChannel");
  ObjectFactory m_deviceFactory("ns3::PointToPointNetDevice");
  ObjectFactory m_queueFactory ("ns3::DropTailQueue");

  // A single delay for the link.
  m_channelFactory.Set ("Delay", TimeValue(MilliSeconds(delay)));

  // Setting up link A ////////////////////////////////////////////////////////
  Ptr<Node> nodeA = m_allNodes.Get(linkA.srcNode);
  m_deviceFactory.Set ("DataRate", DataRateValue(DataRate(linkA.capacity*1000000))); // In bps
  Ptr<PointToPointNetDevice> devA = m_deviceFactory.Create<PointToPointNetDevice> ();
  devA->SetAddress (Mac48Address::Allocate ());
  nodeA->AddDevice(devA);
  Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
  devA->SetQueue (queueA);
  // Inserting a reference to the net device in the switch. This information will be
  // used when building the routing table.
  // Only if if is a switch!!
  if (linkA.srcNodeType == 'S')
    m_switchMap[linkA.srcNode].InsertNetDevice(linkA.linkId, devA);

  // Setting up link B ////////////////////////////////////////////////////////
  Ptr<Node> nodeB = m_allNodes.Get(linkB.srcNode);
  m_deviceFactory.Set ("DataRate", DataRateValue(DataRate(linkB.capacity*1000000))); // In bps
  Ptr<PointToPointNetDevice> devB = m_deviceFactory.Create<PointToPointNetDevice> ();
  devB->SetAddress (Mac48Address::Allocate ());
  nodeB->AddDevice (devB);
  Ptr<Queue> queueB = m_queueFactory.Create<Queue> ();
  devB->SetQueue (queueB);
  // Inserting a reference to the net device in the switch. This information will be
  // used when building the routing table.
  if (linkB.srcNodeType == 'S')
    m_switchMap[linkB.srcNode].InsertNetDevice(linkB.linkId, devB);

  Ptr<PointToPointChannel> channel = m_channelFactory.Create<PointToPointChannel> ();
  devA->Attach (channel);
  devB->Attach (channel);
}

void
TopologyBuilder::InstallP2pLink (LinkInformation& link, uint32_t delay)
{
  ObjectFactory channelFactory("ns3::PointToPointChannel");
  ObjectFactory deviceFactory("ns3::PointToPointNetDevice");
  ObjectFactory queueFactory("ns3::DropTailQueue");

  channelFactory.Set ("Delay", TimeValue(MilliSeconds(delay)));
  // Setting the data rate in bps
  deviceFactory.Set ("DataRate", DataRateValue(DataRate(link.capacity*1000000)));

  // Setting up link A ////////////////////////////////////////////////////////
  Ptr<Node> nodeA = m_allNodes.Get(link.srcNode);
  Ptr<PointToPointNetDevice> devA = deviceFactory.Create<PointToPointNetDevice> ();
  devA->SetAddress (Mac48Address::Allocate ());
  nodeA->AddDevice(devA);
  Ptr<Queue> queueA = queueFactory.Create<Queue> ();
  devA->SetQueue (queueA);
  // Inserting a reference to the net device in the switch. This information will be
  // used when building the routing table.
  if (link.srcNodeType == 'S')
    m_switchMap[link.srcNode].InsertNetDevice(link.linkId, devA);

  // Setting up link B ////////////////////////////////////////////////////////
  Ptr<Node> nodeB = m_allNodes.Get(link.dstNode);
  Ptr<PointToPointNetDevice> devB = deviceFactory.Create<PointToPointNetDevice> ();
  devB->SetAddress (Mac48Address::Allocate ());
  nodeB->AddDevice (devB);
  Ptr<Queue> queueB = queueFactory.Create<Queue> ();
  queueB->SetMaxPackets(0); /*!< Sanity check to make sure no packets are sent from this device */
  devB->SetQueue (queueB);

  Ptr<PointToPointChannel> channel = channelFactory.Create<PointToPointChannel> ();
  devA->Attach (channel);
  devB->Attach (channel);
}
