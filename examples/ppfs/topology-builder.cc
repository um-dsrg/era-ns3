#include <assert.h>

#include "ns3/abort.h"
#include "ns3/object-factory.h"
#include "ns3/string.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/internet-module.h"

#include "topology-builder.h"

using namespace ns3;
using namespace tinyxml2;

template <class SwitchType>
TopologyBuilder<SwitchType>::TopologyBuilder (XMLNode* xmlRootNode, std::map<uint32_t,
                                              SwitchType> &switchMap,
                                              std::map<Ptr<NetDevice>, uint32_t>& terminalToLinkId,
                                              NodeContainer& allNodes, NodeContainer& terminalNodes,
                                              NodeContainer& switchNodes,
                                              NetDeviceContainer& terminalDevices) :
  m_xmlRootNode(xmlRootNode), m_switchMap(switchMap), m_terminalToLinkId(terminalToLinkId),
  m_allNodes(allNodes), m_terminalNodes(terminalNodes), m_switchNodes(switchNodes),
  m_terminalDevices(terminalDevices)
{}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::CreateNodes()
{
  XMLElement* networkTopologyElement = m_xmlRootNode->FirstChildElement("NetworkTopology");
  NS_ABORT_MSG_IF(networkTopologyElement == nullptr, "NetworkTopology Element not found");

  uint32_t numOfNodes;
  networkTopologyElement->QueryAttribute("NumberOfNodes", &numOfNodes);
  m_allNodes.Create(numOfNodes);
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::ParseNodeConfiguration()
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
          m_switchNodes.Add(m_allNodes.Get(nodeId)); // Add the node to the switches node container
        }
      else if (nodeType == 'T')
        m_terminalNodes.Add(m_allNodes.Get(nodeId)); // Add the node to the terminals node container
      else
        NS_ABORT_MSG("Unknown node type encountered");

      nodeElement = nodeElement->NextSiblingElement("Node");
    }
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::BuildNetworkTopology(std::map <uint32_t,
                                                  LinkInformation>& linkInformation)
{
  XMLElement* networkTopologyElement = m_xmlRootNode->FirstChildElement("NetworkTopology");
  NS_ABORT_MSG_IF(networkTopologyElement == nullptr, "NetworkTopology Element not found");

  uint32_t linkDelay;
  XMLElement* linkElement = networkTopologyElement->FirstChildElement("Link");
  while (linkElement != nullptr) // Looping through all the link Elements.
    {
      linkElement->QueryAttribute("Delay", &linkDelay);

      // We need to loop through all the Link Elements here.
      XMLElement* linkElementElement = linkElement->FirstChildElement("LinkElement");

      int linkIds [] = {-1, -1};

      int linkElementCounter = 0;
      while (linkElementElement != nullptr)
        {
          LinkInformation linkInfo;

          linkElementElement->QueryAttribute("Id", &linkInfo.linkId);
          linkElementElement->QueryAttribute("SourceNode", &linkInfo.srcNode);
          linkInfo.srcNodeType = *linkElementElement->Attribute("SourceNodeType");
          linkElementElement->QueryAttribute("DestinationNode", &linkInfo.dstNode);
          linkInfo.dstNodeType = *linkElementElement->Attribute("DestinationNodeType");
          linkElementElement->QueryAttribute("Capacity", &linkInfo.capacity);

          auto ret = linkInformation.insert({linkInfo.linkId, linkInfo});
          NS_ABORT_MSG_IF(ret.second == false, "Link ID " << linkInfo.linkId << " is duplicate");

          linkIds[linkElementCounter++] = linkInfo.linkId;
          linkElementElement = linkElementElement->NextSiblingElement("LinkElement");
        }

      if (linkIds[1] == -1)
        InstallP2pLink(linkInformation[linkIds[0]], linkDelay);
      else
        InstallP2pLink(linkInformation[linkIds[0]], linkInformation[linkIds[1]], linkDelay);

      linkElement = linkElement->NextSiblingElement("Link");
    }
}

template <>
void
TopologyBuilder<PpfsSwitch>::SetSwitchRandomNumberGenerator(uint32_t seed, uint32_t initRun)
{
  for (auto & switchNode : m_switchMap)
    {
      switchNode.second.SetRandomNumberGenerator(seed, initRun);
      initRun++;
    }
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::AssignIpToTerminals()
{
  InternetStackHelper internet;
  internet.Install(m_terminalNodes); //Add internet stack to the terminals ONLY

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("1.0.0.0", "255.0.0.0");

  ipv4.Assign(m_terminalDevices);
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::InstallP2pLink (LinkInformation& linkA, LinkInformation& linkB,
                                             uint32_t delay)
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
  /*
   * Inserting a reference to the net device in the switch. This information will be
   * used when building the routing table.
   */
  if (linkA.srcNodeType == 'S')
    m_switchMap[linkA.srcNode].InsertNetDevice(linkA.linkId, devA);
  else if (linkA.srcNodeType == 'T')
    {
      m_terminalToLinkId.insert({devA, linkA.linkId});
      m_terminalDevices.Add(devA);
    }

  // Setting up link B ////////////////////////////////////////////////////////
  Ptr<Node> nodeB = m_allNodes.Get(linkB.srcNode);
  m_deviceFactory.Set ("DataRate", DataRateValue(DataRate(linkB.capacity*1000000))); // In bps
  Ptr<PointToPointNetDevice> devB = m_deviceFactory.Create<PointToPointNetDevice> ();
  devB->SetAddress (Mac48Address::Allocate ());
  nodeB->AddDevice (devB);
  Ptr<Queue> queueB = m_queueFactory.Create<Queue> ();
  devB->SetQueue (queueB);
  /*
   * Inserting a reference to the net device in the switch. This information will be
   * used when building the routing table.
   */
  if (linkB.srcNodeType == 'S')
    m_switchMap[linkB.srcNode].InsertNetDevice(linkB.linkId, devB);
  else if (linkB.srcNodeType == 'T')
    {
      m_terminalToLinkId.insert({devB, linkB.linkId});
      m_terminalDevices.Add(devB);
    }

  Ptr<PointToPointChannel> channel = m_channelFactory.Create<PointToPointChannel> ();
  devA->Attach (channel);
  devB->Attach (channel);
}

template <class SwitchType>
void
TopologyBuilder<SwitchType>::InstallP2pLink (LinkInformation& link, uint32_t delay)
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
  else if (link.srcNodeType == 'T')
    {
      m_terminalToLinkId.insert({devA, link.linkId});
      m_terminalDevices.Add(devA);
    }

  // Setting up link B ////////////////////////////////////////////////////////
  Ptr<Node> nodeB = m_allNodes.Get(link.dstNode);
  Ptr<PointToPointNetDevice> devB = deviceFactory.Create<PointToPointNetDevice> ();
  devB->SetAddress (Mac48Address::Allocate ());
  nodeB->AddDevice (devB);
  Ptr<Queue> queueB = queueFactory.Create<Queue> ();
  queueB->SetMaxPackets(0); /*!< Sanity check to make sure no packets are sent from this device */
  devB->SetQueue (queueB);

  if (link.dstNodeType == 'T')
    m_terminalDevices.Add(devB);

  Ptr<PointToPointChannel> channel = channelFactory.Create<PointToPointChannel> ();
  devA->Attach (channel);
  devB->Attach (channel);
}

/**
 * Explicit instantiation for the template classes. Required to make the linker work.
 * An explicit instantiation is required for all the types that this class will be used for.
 */
template class TopologyBuilder<PpfsSwitch>;
