#include "ns3/ipv4.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-net-device.h"

#include "../examples/ospf-network/ospf-switch.h"
#include "../examples/ppfs/ppfs-switch.h"

#include "routing-helper.h"

using namespace ns3;
using namespace tinyxml2;

template <class SwitchType>
RoutingHelper<SwitchType>::RoutingHelper (std::map<NodeId_t, SwitchType>& switchMap) :
  m_switchMap(switchMap)
{}

template <>
void
RoutingHelper<OspfSwitch>::
PopulateRoutingTables(NodeContainer& allNodes, XMLNode* rootNode)
{
  XMLElement* optimalSolutionElement = rootNode->FirstChildElement("OptimalSolution");
  NS_ABORT_MSG_IF(optimalSolutionElement == nullptr, "OptimalSolution element not found");

  XMLElement* flowElement = optimalSolutionElement->FirstChildElement("Flow");

  while (flowElement != nullptr)
    {
      // Parse the required flow details
      FlowId_t flowId (0);
      NodeId_t sourceNodeId (0);
      NodeId_t destinationNodeId (0);
      uint32_t portNumberXml (0);
      uint16_t portNumber (0);
      char protocol (0);

      flowElement->QueryAttribute("Id", &flowId);
      flowElement->QueryAttribute("SourceNode", &sourceNodeId);
      flowElement->QueryAttribute("DestinationNode", &destinationNodeId);
      protocol = *flowElement->Attribute("Protocol");

      if (protocol == 'U')
        {
          flowElement->QueryAttribute("PortNumber", &portNumberXml);
          portNumber = (uint16_t) portNumberXml; // Required because tinyxml does not handle uint16_t
        }
      else
        {
          flowElement->QueryAttribute("DstPortNumber", &portNumberXml);
          portNumber = (uint16_t) portNumberXml; // Required because tinyxml does not handle uint16_t
        }

      uint32_t srcIp = GetIpAddress(sourceNodeId, allNodes);
      uint32_t dstIp = GetIpAddress(destinationNodeId, allNodes);

      for (auto & switchDetails : m_switchMap)
        {
          switchDetails.second.InsertEntryInRoutingTable (srcIp, dstIp, portNumber, protocol,
                                                          flowId);
        }

      flowElement = flowElement->NextSiblingElement("Flow");
    }
}

template <>
void
RoutingHelper<PpfsSwitch>::
PopulateRoutingTables(std::map <LinkId_t, LinkInformation>& linkInformation,
                      NodeContainer& allNodes, XMLNode* rootNode)
{
  // Key -> Node Id, Flow Id, Value -> Incoming flow rate.
  std::map<std::pair<NodeId_t, FlowId_t>, double> incomingFlow;
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
      protocol = *flowElement->Attribute("Protocol");

      if (protocol == 'U')
        {
          flowElement->QueryAttribute("PortNumber", &portNumberXml);
          portNumber = (uint16_t) portNumberXml; // Required because tinyxml does not handle uint16_t
        }
      else
        {
          flowElement->QueryAttribute("DstPortNumber", &portNumberXml);
          portNumber = (uint16_t) portNumberXml; // Required because tinyxml does not handle uint16_t
        }

      uint32_t srcIp = GetIpAddress(sourceNodeId, allNodes);
      uint32_t dstIp = GetIpAddress(destinationNodeId, allNodes);

      XMLElement* linkElement = flowElement->FirstChildElement("Link");
      while (linkElement != nullptr)
        {
          LinkId_t linkId (0);
          double linkFlowRate (0.0);
          linkElement->QueryAttribute("Id", &linkId);
          linkElement->QueryAttribute("FlowRate", &linkFlowRate);

          NodeId_t linkSrcNode = linkInformation[linkId].srcNode; // Get the link's source node

          if (linkInformation[linkId].srcNodeType == 'S') // Only switches have routing tables.
            {
              double nodeIncomingFlow = incomingFlow[std::make_pair(linkSrcNode, flowId)];
              double flowRatio = linkFlowRate / nodeIncomingFlow;

              m_switchMap[linkSrcNode].InsertEntryInRoutingTable(srcIp, dstIp, portNumber, protocol,
                                                                 flowId, linkId, flowRatio);
            }
          linkElement = linkElement->NextSiblingElement("Link");
        }
      flowElement = flowElement->NextSiblingElement("Flow");
    }
}

template <class SwitchType>
void
RoutingHelper<SwitchType>::SetSwitchesPacketHandler()
{
  // Loop through the switch map.
  for (auto & switchNode : m_switchMap)
    {
      switchNode.second.SetPacketHandlingMechanism();
    }
}

template <class SwitchType>
void
RoutingHelper<SwitchType>::
ParseIncomingFlows (std::map<std::pair<NodeId_t, FlowId_t>, double>& incomingFlow,
                    XMLNode* rootNode)
{
  XMLElement* incomingFlowElement = rootNode->FirstChildElement("IncomingFlow");
  NS_ABORT_MSG_IF(incomingFlowElement == nullptr, "IncomingFlow element not found");

  XMLElement* nodeElement = incomingFlowElement->FirstChildElement("Node");
  while (nodeElement != nullptr)
    {
      NodeId_t nodeId (0);
      nodeElement->QueryAttribute("Id", &nodeId);

      XMLElement* flowElement = nodeElement->FirstChildElement("Flow");
      while (flowElement != nullptr)
        {
          FlowId_t flowId (0);
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

template <class SwitchType>
uint32_t
RoutingHelper<SwitchType>::GetIpAddress (NodeId_t nodeId, NodeContainer& nodes)
{
  return nodes.Get(nodeId)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal().Get();
}
