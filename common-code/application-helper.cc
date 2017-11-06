#include "ns3/inet-socket-address.h"
#include "ns3/on-off-helper.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/packet-sink-helper.h"

#include "application-helper.h"

using namespace ns3;
using namespace tinyxml2;

void
ApplicationHelper::InstallApplicationOnTerminals (ApplicationMonitor& applicationMonitor,
    ns3::NodeContainer& allNodes, tinyxml2::XMLNode* rootNode)
{
  XMLElement* optimalSolutionElement = rootNode->FirstChildElement ("OptimalSolution");
  NS_ABORT_MSG_IF (optimalSolutionElement == nullptr, "OptimalSolution element not found");

  XMLElement* flowElement = optimalSolutionElement->FirstChildElement ("Flow");

  while (flowElement != nullptr)
    {
      double dataRateInclHdr (0);
      char protocol (*flowElement->Attribute ("Protocol"));
      flowElement->QueryAttribute ("DataRate", &dataRateInclHdr);

      // Do not install the application if the data rate is equal to 0 or it is
      // an acknowledgement flow.
      if (dataRateInclHdr == 0 || protocol == 'A')
        {
          flowElement = flowElement->NextSiblingElement ("Flow"); // Move to the next flow
          continue;
        }

      uint32_t pktSizeInclHdr (0);
      uint32_t numOfPackets (0);
      NodeId_t dstNodeId (0);
      uint32_t srcPortNumber (0);
      uint32_t dstPortNumber (0);

      flowElement->QueryAttribute ("PacketSize", &pktSizeInclHdr);
      flowElement->QueryAttribute ("NumOfPackets", &numOfPackets);
      flowElement->QueryAttribute ("DestinationNode", &dstNodeId);

      if (protocol == 'U')
        {
          flowElement->QueryAttribute ("PortNumber", &dstPortNumber);
        }
      else
        {
          flowElement->QueryAttribute ("SrcPortNumber", &srcPortNumber);
          flowElement->QueryAttribute ("DstPortNumber", &dstPortNumber);
        }

      uint32_t pktSizeExclHdr (pktSizeInclHdr - CalculateHeaderSize (protocol));
      double dataRateExclHdr ((pktSizeExclHdr * dataRateInclHdr) / pktSizeInclHdr);

      std::string socketProtocol ("");
      if (protocol == 'T') //
        socketProtocol = "ns3::TcpSocketFactory";
      else if (protocol == 'U')
        socketProtocol = "ns3::UdpSocketFactory";
      else
        NS_ABORT_MSG ("Unknown protocol");

      InetSocketAddress destinationSocket (GetIpAddress (dstNodeId, allNodes), dstPortNumber);
      OnOffHelper onOff (socketProtocol, destinationSocket);
      onOff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      onOff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      onOff.SetAttribute ("PacketSize", UintegerValue (pktSizeExclHdr));
      onOff.SetAttribute ("DataRate", DataRateValue (dataRateExclHdr * 1000000)); // In bps
      onOff.SetAttribute ("SourcePort", UintegerValue (srcPortNumber));

      NodeId_t srcNodeId (0);
      uint32_t startTime (0);
      uint32_t endTime (0);
      flowElement->QueryAttribute ("SourceNode", &srcNodeId);
      flowElement->QueryAttribute ("StartTime", &startTime);
      flowElement->QueryAttribute ("EndTime", &endTime);

      // Configuring the transmitter
      ApplicationContainer sourceApplication = onOff.Install (allNodes.Get (srcNodeId));
      sourceApplication.Start (Seconds (startTime));

      // Configuring the receiver
      PacketSinkHelper sinkHelper (socketProtocol, destinationSocket);
      ApplicationContainer destinationApplication = sinkHelper.Install (allNodes.Get (dstNodeId));
      destinationApplication.Start (Seconds (startTime));

      // Monitor the PacketSink application
      FlowId_t flowId (0);
      flowElement->QueryAttribute ("Id", &flowId);
      applicationMonitor.MonitorApplication (flowId, dataRateExclHdr,
                                             destinationApplication.Get (0));

      flowElement = flowElement->NextSiblingElement ("Flow");
    }
}

uint32_t
ApplicationHelper::CalculateHeaderSize (char protocol)
{
  switch (protocol)
    {
    case 'T': // TCP (TCP:20 + 12 options + IP:20 + P2P:2)
      return 54;
      break;
    case 'U': // UDP (UDP:8 + IP:20 + P2P:2)
      return 30;
      break;
    default: // Error
      NS_ABORT_MSG ("Unknown protocol");
    }
}

Ipv4Address
ApplicationHelper::GetIpAddress (NodeId_t nodeId, ns3::NodeContainer& nodes)
{
  return nodes.Get (nodeId)->GetObject<Ipv4>()->GetAddress (1, 0).GetLocal();
}
