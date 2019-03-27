#include "ns3/log.h"

#include "flow.h"

NS_LOG_COMPONENT_DEFINE ("Flow");

/**
 * Path Implementation
 */

/**
 * The global port number counter starting from 49152 as it is the start of the range of
 * dynamic/private port numbers. This was done to avoid any conflicts with any other
 * standards.
 */
portNum_t Path::portNumCounter = 49152;

portNum_t
Path::GeneratePortNumber ()
{
  return portNumCounter++;
}

void
Path::AddLink (Link const *link)
{
  m_links.push_back (link);
}

const std::vector<Link const *> &
Path::GetLinks () const
{
  return m_links;
}

std::ostream &
operator<< (std::ostream &output, const Path &path)
{
  output << "Path ID: " << path.id << "\n";
  output << "Data Rate: " << path.dataRate << "\n";
  output << "Source Port: " << path.srcPort << "\n";
  output << "Destination Port: " << path.dstPort << "\n";
  output << "Links \n";

  for (Link const *link : path.m_links)
    {
      output << "Link ID: " << link->id << " Address: " << link << "\n";
    }
  output << "Testing link ids\n";
  for (auto test : path.m_testing)
    {
      output << "Link id: " << test << "\n";
    }

  return output;
}

/**
 * Flow Implementation
 */

void
Flow::AddDataPath (const Path &path)
{
  m_dataPaths.push_back (path);
}

const std::vector<Path> &
Flow::GetDataPaths () const
{
  return m_dataPaths;
}

void
Flow::AddAckPath (const Path &path)
{
  m_ackPaths.push_back (path);
}

const std::vector<Path> &
Flow::GetAckPaths () const
{
  return m_ackPaths;
}

bool
Flow::operator< (const Flow &other) const
{
  /*
    * Used by the map to store the keys in order.
    * In this case it is sorted by Source Ip Address, then Destination Ip Address.
    */
  if (srcNode->GetIpAddress ().Get () == other.srcNode->GetIpAddress ().Get ())
    {
      return dstNode->GetIpAddress ().Get () < other.dstNode->GetIpAddress ().Get ();
    }
  else
    {
      return srcNode->GetIpAddress ().Get () < other.srcNode->GetIpAddress ().Get ();
    }
}

std::ostream &
operator<< (std::ostream &output, const Flow &flow)
{
  ns3::Ipv4Address address;
  output << "Flow ID: " << flow.id << "\n";
  output << "Data Rate: " << flow.dataRate << "\n";
  output << "Packet Size: " << flow.packetSize << "bytes\n";
  output << "Source IP Addr: " << flow.srcNode->GetIpAddress ().Get () << "\n";
  output << "Destination IP Addr: " << flow.dstNode->GetIpAddress ().Get () << "\n";
  output << "Protocol: " << static_cast<char> (flow.protocol) << "\n";
  output << "----------------------\n";

  if (!flow.m_dataPaths.empty ())
    {
      output << "Data Paths\n---\n";
      for (const auto &dataPath : flow.m_dataPaths)
        {
          output << dataPath;
          output << "---\n";
        }
    }

  if (!flow.m_ackPaths.empty ())
    {
      output << "Ack Paths\n---\n";
      for (const auto &ackPath : flow.m_ackPaths)
        {
          output << ackPath;
          output << "---\n";
        }
    }
  return output;
}

/**
 * Function Implementation
 */

/* Key: Path ID | Value: source port, destination port */
using pathPortMap_t = std::map<id_t, std::pair<portNum_t, portNum_t>>;

pathPortMap_t AddDataPaths (Flow &flow, tinyxml2::XMLElement *flowElement,
                            Link::linkContainer_t &linkContainer, SwitchType switchType);
void AddAckPaths (Flow &flow, tinyxml2::XMLElement *flowElement, const pathPortMap_t &pathPortMap,
                  Link::linkContainer_t linkContainer, SwitchType switchType);

Flow::flowContainer_t
ParseFlows (tinyxml2::XMLNode *rootNode, Terminal::terminalContainer_t &terminalContainer,
            Link::linkContainer_t &linkContainer, SwitchType switchType)
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
      flow.srcNode = &terminalContainer.at (srcNodeId);

      id_t dstNodeId{0};
      flowElement->QueryAttribute ("DestinationNode", &dstNodeId);
      flow.dstNode = &terminalContainer.at (dstNodeId);

      flow.protocol = static_cast<FlowProtocol> (*flowElement->Attribute ("Protocol"));
      flowElement->QueryAttribute ("PacketSize", &flow.packetSize);
      double dataRate;
      flowElement->QueryAttribute ("AllocatedDataRate", &dataRate);
      flow.dataRate = ns3::DataRate (std::string{std::to_string (dataRate) + "Mbps"});

      auto pathPortMap{AddDataPaths (flow, flowElement, linkContainer, switchType)};

      std::cout << "Logging all flows (inside the loop) before adding ack paths" << std::endl;
      for (const auto &flowPair : flows)
        {
          std::cout << flowPair.second << std::endl;
        }

      if (flow.protocol == FlowProtocol::Tcp) // Parse ACK paths for TCP flows only
        {
          // The problem seems to be coming from this function
          std::cout << "Printing the link container before adding ack paths" << std::endl;
          for (const auto &linkPair : linkContainer)
            {
              std::cout << "Link ID " << linkPair.first << " Address: " << &linkPair.second
                        << " Id from object " << linkPair.second.id << std::endl;
            }
          AddAckPaths (flow, flowElement, pathPortMap, linkContainer, switchType);
          std::cout << "Printing the link container after adding ack paths" << std::endl;
          for (const auto &linkPair : linkContainer)
            {
              std::cout << "Link ID " << linkPair.first << " Address: " << &linkPair.second
                        << " Id from object " << linkPair.second.id << std::endl;
            }
        }

      auto ret = flows.emplace (flow.id, flow);
      NS_ABORT_MSG_IF (ret.second == false, "Inserting Flow " << flow.id << " failed");

      std::cout << "Logging all flows (inside the loop) after adding ack paths" << std::endl;
      for (const auto &flowPair : flows)
        {
          std::cout << flowPair.second << std::endl;
        }

      /* NS_LOG_INFO ("Flow Details:\n" << flow); */
      flowElement = flowElement->NextSiblingElement ("Flow");
    }

  /* // The problem is present already here */
  /* std::cout << "Logging the flows before returning" << std::endl; */
  /* for (const auto &flowPair : flows) */
  /*   { */
  /*     std::cout << flowPair.second << std::endl; */
  /*   } */
  return flows;
}

pathPortMap_t
AddDataPaths (Flow &flow, tinyxml2::XMLElement *flowElement, Link::linkContainer_t &linkContainer,
              SwitchType switchType)
{
  pathPortMap_t pathPortMap;

  auto pathsElement = flowElement->FirstChildElement ("Paths");
  auto pathElement = pathsElement->FirstChildElement ("Path");

  portNum_t srcPort{0};
  portNum_t dstPort{0};

  if (switchType == SwitchType::PpfsSwitch)
    { // Under PPFS, only one flow is generated; therefore, each path will have same port numbers.
      srcPort = Path::GeneratePortNumber ();
      dstPort = Path::GeneratePortNumber ();
    }

  while (pathElement != nullptr)
    {
      Path path;
      // Set port numbers
      path.srcPort = (srcPort == 0) ? Path::GeneratePortNumber () : srcPort;
      path.dstPort = (dstPort == 0) ? Path::GeneratePortNumber () : dstPort;

      pathElement->QueryAttribute ("Id", &path.id);

      double dataRate;
      pathElement->QueryAttribute ("DataRate", &dataRate);
      path.dataRate = ns3::DataRate (std::string{std::to_string (dataRate) + "Mbps"});

      auto linkElement = pathElement->FirstChildElement ("Link");
      while (linkElement != nullptr)
        {
          id_t linkId;
          linkElement->QueryAttribute ("Id", &linkId);
          path.AddLink (&linkContainer.at (linkId));
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
AddAckPaths (Flow &flow, tinyxml2::XMLElement *flowElement, const pathPortMap_t &pathPortMap,
             Link::linkContainer_t linkContainer, SwitchType switchType)
{
  if (switchType == SwitchType::PpfsSwitch)
    {
      auto ackShortestPathElement = flowElement->FirstChildElement ("AckShortestPath");

      /* All data paths have the same source and destination port numbers;
         therefore, any path will be valid */
      Path dataPath = flow.GetDataPaths ().front ();
      Path ackPath;

      /* Swapping the port numbers for the ACK flows */
      ackPath.srcPort = dataPath.dstPort;
      ackPath.dstPort = dataPath.srcPort;

      auto linkElement = ackShortestPathElement->FirstChildElement ("Link");
      while (linkElement != nullptr)
        {
          id_t linkId;
          linkElement->QueryAttribute ("Id", &linkId);
          ackPath.AddLink (&linkContainer.at (linkId));
          linkElement = linkElement->NextSiblingElement ("Link");
        }
      flow.AddAckPath (ackPath);
    }
  else if (switchType == SwitchType::SdnSwitch)
    {
      auto ackPathsElement = flowElement->FirstChildElement ("AckPaths");
      auto pathElement = ackPathsElement->FirstChildElement ("Path");

      while (pathElement != nullptr)
        {
          Path path;
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
              // NOTE The problem is here because it is not giving me the same addresses.
              // As a solution try and store a unique pointer to the link in the map instead of the link,
              // that way the underlying object does not change.
              std::cout << "The address of link " << linkId << " is " << &linkContainer[linkId]
                        << std::endl;
              path.AddLink (&linkContainer.at (linkId));
              path.m_testing.push_back (linkId);
              linkElement = linkElement->NextSiblingElement ("Link");
            }

          flow.AddAckPath (path);
          pathElement = pathElement->NextSiblingElement ("Path");
        }
    }
}
