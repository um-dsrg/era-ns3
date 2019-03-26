#include <boost/numeric/conversion/cast.hpp>

#include "topology-builder.h"
#include "device/switch/sdn-switch.h"
#include "device/switch/ppfs-switch.h"

NS_LOG_COMPONENT_DEFINE ("TopologyBuilder");

TopologyBuilder::TopologyBuilder (SwitchType switchType) : m_switchType (switchType)
{
}

void
TopologyBuilder::CreateNodes (XMLNode *rootNode)
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

std::map<id_t, Ptr<NetDevice>>
TopologyBuilder::BuildNetworkTopology (XMLNode *rootNode)
{
  /**
     A map that given a link id will return the NetDevice that needs to be used to
     transmit on the given link. The Node is not required because it can be retrieved
     from the link object.
     */
  std::map<id_t, Ptr<NetDevice>> transmitOnLink;

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
          auto ret = m_links.emplace (link.id, link);
          NS_ABORT_MSG_IF (ret.second == false, "Duplicate link " << link.id << " found");

          linkElementElement = linkElementElement->NextSiblingElement ("LinkElement");
        }

      InstallP2pLinks (sameDelayLinks, transmitOnLink);
      linkElement = linkElement->NextSiblingElement ("Link");
    }

  return transmitOnLink;
}

void
TopologyBuilder::AssignIpToTerminals ()
{
  InternetStackHelper stack;
  NetDeviceContainer terminalDevices; //!< Contain all the terminal's NetDevices

  for (auto &terminalPair : m_terminals)
    {
      auto terminalNode = terminalPair.second.GetNode ();

      NS_ABORT_MSG_IF (terminalNode->GetNDevices () != 1,
                       "Terminal " << terminalPair.first << " has " << terminalNode->GetNDevices ()
                                   << " NetDevice(s).");
      terminalDevices.Add (terminalNode->GetDevice (0));

      /**
         Store the Node NetDevice before installing the IP Stack as this will add an additional
         NetDevice to the node. From the documentation: The LoopbackNetDevice is automatically
         added to any node as soon as the Internet stack is initialized.
         */
      NS_LOG_INFO ("Installing the IP Stack on terminal " << terminalPair.first);
      stack.Install (terminalNode);
    }

  NS_LOG_INFO ("Assigning IP Addresses to each terminal");
  Ipv4AddressHelper ipAddrHelper;
  ipAddrHelper.SetBase ("0.0.0.0", "255.0.0.0");
  ipAddrHelper.Assign (terminalDevices);

  for (auto &terminalPair : m_terminals)
    {
      terminalPair.second.SetIpAddress ();
    }
}

Flow::flowContainer_t
TopologyBuilder::ParseFlows (XMLNode *rootNode)
{
  Flow::flowContainer_t flows;

  auto flowDetElement = rootNode->FirstChildElement ("FlowDetails");
  NS_ABORT_MSG_IF (flowDetElement == nullptr, "FlowDetails Element not found");

  auto flowElement = flowDetElement->FirstChildElement ("Flow");
  while (flowElement != nullptr)
    {
      Flow flow;

      flowElement->QueryAttribute ("Id", &flow.id);

      id_t srcNodeId{0};
      flowElement->QueryAttribute ("SourceNode", &srcNodeId);
      flow.srcNode = &m_terminals.at (srcNodeId);

      id_t dstNodeId{0};
      flowElement->QueryAttribute ("DestinationNode", &dstNodeId);
      flow.dstNode = &m_terminals.at (dstNodeId);

      flow.protocol = static_cast<FlowProtocol> (*flowElement->Attribute ("Protocol"));
      flowElement->QueryAttribute ("PacketSize", &flow.packetSize);
      double dataRate;
      flowElement->QueryAttribute ("AllocatedDataRate", &dataRate);
      flow.dataRate = ns3::DataRate (std::string{std::to_string (dataRate) + "Mbps"});

      auto pathPortMap{AddDataPaths (flow, flowElement)};

      if (flow.protocol == FlowProtocol::Tcp)
        { // Parse ACK paths for TCP flows only
          AddAckPaths (flow, flowElement, pathPortMap);
        }

      auto ret = flows.emplace (flow.id, flow);
      NS_ABORT_MSG_IF (ret.second == false, "Inserting Flow " << flow.id << " failed");

      NS_LOG_INFO ("Flow Details:\n" << flow);
      flowElement = flowElement->NextSiblingElement ("Flow");
    }

  return flows;
}

void
TopologyBuilder::EnablePacketReceptionOnSwitches ()
{
  NS_LOG_INFO ("Enabling Packet reception on all switches");
  for (auto &switchPair : m_switchContainer)
    {
      auto &switchInstance = switchPair.second;
      switchInstance->SetPacketReception ();
    }
}

void
TopologyBuilder::ReconcileRoutingTables ()
{
  NS_LOG_INFO ("Reconciling the routing tables.\n"
               "NOTE This function should only be invoked for PPFS switches");

  for (auto &switchPair : m_switchContainer)
    {
      auto &switchObject{switchPair.second};
      switchObject->ReconcileSplitRatios ();
    }
}

void
TopologyBuilder::CreateUniqueNode (id_t nodeId, NodeType nodeType)
{
  if (nodeType == NodeType::Switch)
    {
      m_switchContainer.AddSwitch (nodeId, m_switchType);
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
    {
      NS_ABORT_MSG ("Node Id " << nodeId << " has an invalid node type");
    }
}

void
TopologyBuilder::InstallP2pLinks (const std::vector<Link> &links,
                                  std::map<id_t, Ptr<NetDevice>> &transmitOnLink)
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

  /**
     If two links are present, they have been verified to be bidirectional.
     From this point onwards the first link will be used to build the Point
     To Point link, with the exception of the data rate values as they might
     be different.
     */
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
    {
      deviceFactory.Set ("DataRate", DataRateValue (DataRate (links[1].capacity * 1'000'000)));
    }
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

  auto ret = transmitOnLink.emplace (commonLink.id, srcNd);
  NS_ABORT_MSG_IF (ret.second == false,
                   "Duplicate link id " << commonLink.id << " in transmit on link map");

  if (links.size () == 2)
    {
      auto ret = transmitOnLink.emplace (links[1].id, dstNd);
      NS_ABORT_MSG_IF (ret.second == false,
                       "Duplicate link id " << links[1].id << " in transmit on link map");
    }
}

CustomDevice *
TopologyBuilder::GetNode (id_t id, NodeType nodeType)
{
  try
    {
      if (nodeType == NodeType::Switch)
        {
          return m_switchContainer.GetSwitch (id);
        }
      else if (nodeType == NodeType::Terminal)
        {
          return &m_terminals.at (id);
        }
      else
        {
          NS_ABORT_MSG ("Node " << id << " type not found");
        }
  } catch (const std::out_of_range &oor)
    {
      NS_ABORT_MSG ("The node " << id << " has not been found.");
  }
}

TopologyBuilder::pathPortMap_t
TopologyBuilder::AddDataPaths (Flow &flow, XMLElement *flowElement)
{
  TopologyBuilder::pathPortMap_t pathPortMap;

  auto pathsElement = flowElement->FirstChildElement ("Paths");
  auto pathElement = pathsElement->FirstChildElement ("Path");

  while (pathElement != nullptr)
    {
      Path path (true);
      pathElement->QueryAttribute ("Id", &path.id);

      double dataRate;
      pathElement->QueryAttribute ("DataRate", &dataRate);
      path.dataRate = ns3::DataRate (std::string{std::to_string (dataRate) + "Mbps"});

      auto linkElement = pathElement->FirstChildElement ("Link");
      while (linkElement != nullptr)
        {
          id_t linkId;
          linkElement->QueryAttribute ("Id", &linkId);
          path.AddLink (&m_links.at (linkId));
          linkElement = linkElement->NextSiblingElement ("Link");
        }

      auto ret = pathPortMap.emplace (path.id, std::make_pair (path.srcPort, path.dstPort));
      NS_ABORT_MSG_IF (ret.second == false, "Trying to insert a duplicate path " << path.id);

      flow.AddDataPath (path);
      pathElement = pathElement->NextSiblingElement ("Path");
    }

  return pathPortMap;
}

void
TopologyBuilder::AddAckPaths (Flow &flow, XMLElement *flowElement,
                              const TopologyBuilder::pathPortMap_t &pathPortMap)
{
  auto ackPathsElement = flowElement->FirstChildElement ("AckPaths");
  auto pathElement = ackPathsElement->FirstChildElement ("Path");

  while (pathElement != nullptr)
    {
      Path path (false);
      pathElement->QueryAttribute ("Id", &path.id);

      const auto &dataPathPortPair{pathPortMap.at (path.id)};
      /* Swapping the port numbers for the ACK flows */
      path.srcPort = dataPathPortPair.second;
      path.dstPort = dataPathPortPair.first;

      auto linkElement = pathElement->FirstChildElement ("Link");
      while (linkElement != nullptr)
        {
          id_t linkId;
          linkElement->QueryAttribute ("Id", &linkId);
          path.AddLink (&m_links.at (linkId));
          linkElement = linkElement->NextSiblingElement ("Link");
        }

      flow.AddAckPath (path);
      pathElement = pathElement->NextSiblingElement ("Path");
    }
}

///////////////////////////////////////////////////////////////////////////////////
/*template <>*/
/*typename TopologyBuilder<PpfsSwitch>::PathPortMap*/
/*TopologyBuilder<PpfsSwitch>::AddDataPaths(Flow &flow, XMLElement *flowElement) {*/

/*    TopologyBuilder<PpfsSwitch>::PathPortMap pathPortMap;*/

/*    auto pathsElement = flowElement->FirstChildElement ("Paths");*/
/*    auto pathElement = pathsElement->FirstChildElement ("Path");*/

/*    Path dummyPath(true);*/

/*    while (pathElement != nullptr) {*/
/*        Path path(false);*/
/*        path.srcPort = dummyPath.srcPort;*/
/*        path.dstPort = dummyPath.dstPort;*/

/*        pathElement->QueryAttribute ("Id", &path.id);*/

/*        double dataRate;*/
/*        pathElement->QueryAttribute("DataRate", &dataRate);*/
/*        path.dataRate = ns3::DataRate(std::string{std::to_string(dataRate) + "Mbps"});*/

/*        auto linkElement = pathElement->FirstChildElement ("Link");*/
/*        while (linkElement != nullptr) {*/
/*            id_t linkId;*/
/*            linkElement->QueryAttribute ("Id", &linkId);*/
/*            path.AddLink (&m_links.at (linkId));*/
/*            linkElement = linkElement->NextSiblingElement ("Link");*/
/*        }*/

/*        auto ret = pathPortMap.emplace(path.id, std::make_pair(path.srcPort, path.dstPort));*/
/*        NS_ABORT_MSG_IF(ret.second == false, "Trying to insert a duplicate path " << path.id);*/

/*        flow.AddDataPath(path);*/
/*        pathElement = pathElement->NextSiblingElement ("Path");*/
/*    }*/

/*    return pathPortMap;*/
/*}*/

/*template <>*/
/*void TopologyBuilder<PpfsSwitch>::AddAckPaths(Flow& flow, XMLElement* flowElement,*/
/*                                              const TopologyBuilder<PpfsSwitch>::PathPortMap& pathPortMap) {*/

/*    auto ackShortestPathElement = flowElement->FirstChildElement("AckShortestPath");*/

/*   */
/*     All data paths have the same source and destination port numbers; therefore,*/
/*     any path will do for this purpose.*/
/**/
/*    Path dataPath = flow.GetDataPaths().front();*/
/*    Path ackPath(false);*/

/* Swapping the port numbers for the ACK flows */
/*    ackPath.srcPort = dataPath.dstPort;*/
/*    ackPath.dstPort = dataPath.srcPort;*/

/*    auto linkElement = ackShortestPathElement->FirstChildElement("Link");*/
/*    while (linkElement != nullptr) {*/
/*        id_t linkId;*/
/*        linkElement->QueryAttribute ("Id", &linkId);*/
/*        ackPath.AddLink (&m_links.at (linkId));*/
/*        linkElement = linkElement->NextSiblingElement ("Link");*/
/*    }*/

/*    flow.AddAckShortestPath(ackPath);*/
/*}*/

/*template <>*/
/*void TopologyBuilder<SdnSwitch>::ReconcileRoutingTables() {*/
/*}*/

/*template <>*/
/*void TopologyBuilder<PpfsSwitch>::ReconcileRoutingTables() {*/
/*    NS_LOG_INFO("Reconciling the routing tables.\n"*/
/*                "NOTE This function should only be invoked for PPFS switches");*/

/*    for (auto& switchPair : m_switches) {*/
/*        auto& switchObject {switchPair.second};*/
/*        switchObject.ReconcileSplitRatios();*/
/*    }*/
/*}*/

/*template <>*/
/*tinyxml2::XMLElement* TopologyBuilder<SdnSwitch>::GetSwitchQueueLoggingElement(XMLDocument& xmlDocument) {*/
/*    XMLElement* queuesElement = xmlDocument.NewElement("Queue");*/
/*    for (const auto& switchPair : m_switches) {*/
/*        auto switchId {switchPair.first};*/
/*        NS_LOG_INFO("Saving the queue elements of switch " << switchId);*/

/*        XMLElement* switchElement = xmlDocument.NewElement("Switch");*/
/*        switchElement->SetAttribute("Id", switchId);*/

/*        auto netDevCounter = uint32_t{0};*/
/*        const auto& switchInstance {switchPair.second};*/
/*        for (const auto& devLogPair : switchInstance.m_netDeviceQueueLog) {*/
/*            XMLElement* netDevElement = xmlDocument.NewElement("NetDevice");*/
/*            netDevElement->SetAttribute("Id", netDevCounter);*/

/*            for (const auto& queueLogPair: devLogPair.second) {*/
/*                XMLElement* sizeElement = xmlDocument.NewElement("Packet");*/
/*                sizeElement->SetAttribute("NumPktsInQueue", queueLogPair.first);*/
/*                sizeElement->SetAttribute("Time", boost::numeric_cast<double>(queueLogPair.second.GetNanoSeconds()));*/
/*                netDevElement->InsertEndChild(sizeElement);*/
/*            }*/

/*            netDevCounter++;*/
/*            switchElement->InsertEndChild(netDevElement);*/
/*        }*/
/*        queuesElement->InsertEndChild(switchElement);*/
/*    }*/
/*    return queuesElement;*/
/*}*/

/*template <>*/
/*tinyxml2::XMLElement* TopologyBuilder<PpfsSwitch>::GetSwitchQueueLoggingElement(XMLDocument& xmlDocument) {*/
/*    return nullptr;*/
/*}*/

/**
 Shuffles the XML Link elements in place.

 @param linkElements[in,out] Vector of link elements.
 */
void
ShuffleLinkElements (std::vector<XMLElement *> &linkElements)
{
  std::random_shuffle (linkElements.begin (), linkElements.end ());
}