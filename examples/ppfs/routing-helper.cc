#include "routing-helper.h"

#include "ns3/ipv4.h"

using namespace ns3;
using namespace tinyxml2;

void
RoutingHelper::PopulateRoutingTables (std::map<uint32_t, PpfsSwitch>& switchMap,
                                      std::map <uint32_t, LinkInformation>& linkInformation,
                                      NodeContainer& allNodes, XMLNode* rootNode)
{
  // Key -> Node Id, Flow Id, Value -> Incoming flow rate.
  std::map<std::pair<uint32_t, uint32_t>, double> incomingFlow;
  ParseIncomingFlows(incomingFlow, rootNode);
  /*!< Key -> Node ID. Value -> Switch object switchMap*/
  /*!< Key -> Link ID, Value -> Link Info linkInformation*/
  XMLElement* optimalSolutionElement = rootNode->FirstChildElement("OptimalSolution");
  NS_ABORT_MSG_IF(optimalSolutionElement == nullptr, "OptimalSolution element not found");

  XMLElement* flowElement = optimalSolutionElement->FirstChildElement("Flow");

  while (flowElement != nullptr)
    {
      // Parse the required flow details
      uint32_t flowId (0);
      uint32_t sourceNodeId (0);
      uint32_t destinationNodeId (0);
      uint32_t portNumberXml (0);
      uint16_t portNumber (0);
      char protocol (0);

      flowElement->QueryAttribute("Id", &flowId);
      flowElement->QueryAttribute("SourceNode", &sourceNodeId);
      flowElement->QueryAttribute("DestinationNode", &destinationNodeId);
      flowElement->QueryAttribute("PortNumber", &portNumberXml);
      portNumber = (uint16_t) portNumberXml; // Required because tinyxml does not handle uint16_t
      protocol = *flowElement->Attribute("Protocol");

      uint32_t srcIp = GetIpAddress(sourceNodeId, allNodes);
      uint32_t dstIp = GetIpAddress(destinationNodeId, allNodes);

      XMLElement* linkElement = flowElement->FirstChildElement("Link");
      while (linkElement != nullptr)
        {
          uint32_t linkId (0);
          double linkFlowRate (0.0);
          linkElement->QueryAttribute("Id", &linkId);
          linkElement->QueryAttribute("FlowRate", &linkFlowRate);

          uint32_t linkSrcNode = linkInformation[linkId].srcNode; // Get the link's source node

          if (linkInformation[linkId].srcNodeType == 'S') // Only switches have routing tables.
            {
              double nodeIncomingFlow = incomingFlow[std::make_pair(linkSrcNode, flowId)];
              double flowRatio = linkFlowRate / nodeIncomingFlow;

              switchMap[linkSrcNode].InsertEntryInRoutingTable(srcIp, dstIp, portNumber, protocol,
                                                               linkId, flowRatio);
            }
          linkElement = linkElement->NextSiblingElement("Link");
        }
      flowElement = flowElement->NextSiblingElement("Flow");
    }
}

void
RoutingHelper::SetReceiveFunctionForSwitches(NodeContainer switchNodes)
{
  for (auto switchNode = switchNodes.Begin(); switchNode != switchNodes.End(); ++switchNode)
    {
      uint32_t numOfDevices = (*switchNode)->GetNDevices();
      NS_ASSERT(numOfDevices > 0);
      for (uint32_t currentDevice = 0; currentDevice < numOfDevices; ++currentDevice)
        {
          (*switchNode)->RegisterProtocolHandler(MakeCallback(&RoutingHelper::ReceiveFromDevice,
                                                              this),
                                                 0, (*switchNode)->GetDevice(currentDevice),
                                                 true);
        }
    }
}

void
RoutingHelper::ReceiveFromDevice(Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                                 uint16_t protocol, const Address &src, const Address &dst,
                                 NetDevice::PacketType packetType)
{
  std::cout << "A packet was received on node " << incomingPort->GetNode()->GetId() << std::endl;
  // TODO: We need to insert the functionality here.
}

void
RoutingHelper::ParseIncomingFlows (std::map<std::pair<uint32_t, uint32_t>, double>& incomingFlow,
                                   XMLNode* rootNode)
{
  XMLElement* incomingFlowElement = rootNode->FirstChildElement("IncomingFlow");
  NS_ABORT_MSG_IF(incomingFlowElement == nullptr, "IncomingFlow element not found");

  XMLElement* nodeElement = incomingFlowElement->FirstChildElement("Node");
  while (nodeElement != nullptr)
    {
      uint32_t nodeId (0);
      nodeElement->QueryAttribute("Id", &nodeId);

      XMLElement* flowElement = nodeElement->FirstChildElement("Flow");
      while (flowElement != nullptr)
        {
          uint32_t flowId (0);
          double flowValue (0);
          flowElement->QueryAttribute("Id", &flowId);
          flowElement->QueryAttribute("IncomingFlow", &flowValue);

          auto ret = incomingFlow.insert({std::make_pair(nodeId, flowId), flowValue});
          NS_ABORT_MSG_IF(ret.second == false, "Duplicate entry when parsing IncomingFlow");

          flowElement = flowElement->NextSiblingElement("Flow");
        }
      nodeElement = nodeElement->NextSiblingElement("Node");
    }
}

uint32_t
RoutingHelper::GetIpAddress (uint32_t nodeId, NodeContainer& nodes)
{
  return nodes.Get(nodeId)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal().Get();
}
