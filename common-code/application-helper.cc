#include "ns3/inet-socket-address.h"
#include "ns3/on-off-helper.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/packet-sink-helper.h"

#include "application-helper.h"

using namespace ns3;
using namespace tinyxml2;

uint32_t
ApplicationHelper::InstallApplicationOnTerminals(ns3::NodeContainer& allNodes,
                                                 tinyxml2::XMLNode* rootNode)
{
  XMLElement* optimalSolutionElement = rootNode->FirstChildElement("OptimalSolution");
  NS_ABORT_MSG_IF(optimalSolutionElement == nullptr, "OptimalSolution element not found");

  uint32_t maxEndTime (0);

  XMLElement* flowElement = optimalSolutionElement->FirstChildElement("Flow");

  while (flowElement != nullptr)
    {
      uint32_t pktSizeInclHdr (0);
      uint32_t numOfPackets (0);
      uint32_t dstNodeId (0);
      uint32_t portNumber (0);
      double dataRateInclHdr (0);

      char protocol (*flowElement->Attribute("Protocol"));
      flowElement->QueryAttribute("PacketSize", &pktSizeInclHdr);
      flowElement->QueryAttribute("DataRate", &dataRateInclHdr);
      flowElement->QueryAttribute("NumOfPackets", &numOfPackets);
      flowElement->QueryAttribute("DestinationNode", &dstNodeId);
      flowElement->QueryAttribute("PortNumber", &portNumber);

      uint32_t pktSizeExclHdr (pktSizeInclHdr - CalculateHeaderSize(protocol));
      uint32_t maxBytes (numOfPackets * pktSizeExclHdr);
      double dataRateExclHdr ((pktSizeExclHdr * dataRateInclHdr)/pktSizeInclHdr);

      std::string socketProtocol ("");
      if (protocol == 'T') //
        socketProtocol = "ns3::TcpSocketFactory";
      else if (protocol == 'U')
        socketProtocol = "ns3::UdpSocketFactory";
      else
        NS_ABORT_MSG("Unknown protocol");

      InetSocketAddress destinationSocket (GetIpAddress(dstNodeId, allNodes), portNumber);
      OnOffHelper onOff (socketProtocol, destinationSocket);
      onOff.SetAttribute ("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
      onOff.SetAttribute ("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
      onOff.SetAttribute ("PacketSize", UintegerValue(pktSizeExclHdr));
      onOff.SetAttribute("DataRate", DataRateValue(dataRateExclHdr * 1000000)); // In bps
      onOff.SetAttribute("MaxBytes", UintegerValue(maxBytes));

      uint32_t srcNodeId (0);
      uint32_t startTime (0);
      uint32_t endTime (0);
      flowElement->QueryAttribute("SourceNode", &srcNodeId);
      flowElement->QueryAttribute("StartTime", &startTime);
      flowElement->QueryAttribute("EndTime", &endTime);

      // Configuring the transmitter
      ApplicationContainer sourceApplication = onOff.Install(allNodes.Get(srcNodeId));
      sourceApplication.Start(Seconds(startTime));
      sourceApplication.Stop(Seconds(endTime));

      // Configuring the receiver
      PacketSinkHelper sinkHelper (socketProtocol, destinationSocket);
      ApplicationContainer destinationApplication = sinkHelper.Install(allNodes.Get(dstNodeId));
      destinationApplication.Start(Seconds(startTime));
      destinationApplication.Stop(Seconds(endTime));

      if (endTime > maxEndTime) maxEndTime = endTime;

      flowElement = flowElement->NextSiblingElement("Flow");
    }
  return maxEndTime; // Returning the longest flow time
}

uint32_t
ApplicationHelper::CalculateHeaderSize (char protocol)
{
  switch (protocol)
    {
    case 'T': // TCP (TCP:20 + IP:20 + P2P:2)
      return 42;
      break;
    case 'U': // UDP (UDP:8 + IP:20 + P2P:2)
      return 30;
      break;
    default: // Error
      NS_ABORT_MSG("Unknown protocol");
    }
}

Ipv4Address
ApplicationHelper::GetIpAddress (uint32_t nodeId, ns3::NodeContainer& nodes)
{
  return nodes.Get(nodeId)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
}
