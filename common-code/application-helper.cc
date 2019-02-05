#include "ns3/core-module.h"
#include "ns3/inet-socket-address.h"
#include "ns3/on-off-helper.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/packet-sink-helper.h"

#include "application-helper.h"
#include "transmitter-app.h"
#include "receiver-app.h"

using namespace ns3;
using namespace tinyxml2;

NS_LOG_COMPONENT_DEFINE ("ApplicationHelper");


void ApplicationHelper::InstallApplicationsOnTerminals(const Flow::FlowContainer &flows,
                                                       const Terminal::TerminalContainer &terminals)
{
  for (const auto& flowPair : flows) {
    NS_LOG_INFO("Installing flow " << flowPair.first);
    const auto& flow {flowPair.second};

    Ptr<TransmitterApp> transmitterApp = CreateObject<TransmitterApp>(flow);
    flow.srcNode->GetNode()->AddApplication(transmitterApp);
    transmitterApp->SetStartTime(Seconds(1.0));
    transmitterApp->SetStopTime(Seconds(10.0));

    Ptr<ReceiverApp> receiverApp = CreateObject<ReceiverApp>(flow);
    flow.dstNode->GetNode()->AddApplication(receiverApp);
    receiverApp->SetStartTime(Seconds(0.0));
    receiverApp->SetStopTime(Seconds(10.0));
  }

//  // Test application from source node 0 to 2
//  NS_LOG_UNCOND("This is a test");
//  // Source
//  InetSocketAddress sinkSocket (terminals.at(2).GetIpAddress(), 10000);
//  OnOffHelper onOff("ns3::UdpSocketFactory", sinkSocket);
//  onOff.SetAttribute("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
//  onOff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
//  onOff.SetAttribute("PacketSize", UintegerValue(1470));
//  onOff.SetAttribute("DataRate", DataRateValue(5000000)); // Data rate is set to 5Mbps
//  onOff.SetAttribute("MaxBytes", UintegerValue(1470)); // 1 packet
//
//  ApplicationContainer srcApp = onOff.Install(terminals.at(0).GetNode());
//  srcApp.Start(Seconds(1));
//  srcApp.Stop(Seconds(10));
//
//  // Sink
//  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkSocket);
//  ApplicationContainer sinkApp = sinkHelper.Install(terminals.at(2).GetNode());
//  sinkApp.Start(Seconds(0));
//  sinkApp.Stop(Seconds(10));
}

//ApplicationHelper::ApplicationHelper (bool ignoreOptimalDataRates)
//    : m_ignoreOptimalDataRates (ignoreOptimalDataRates)
//{
//}

//void
//ApplicationHelper::InstallApplicationOnTerminals (ApplicationMonitor &applicationMonitor,
//                                                  ns3::NodeContainer &allNodes,
//                                                  uint32_t nPacketsPerFlow,
//                                                  tinyxml2::XMLNode *rootNode)
//{
//  // If the optimal data rates are to be ignored then we need to parse the
//  // FlowDataRateModifications element from the XML Log output by the optimal
//  // solution.
//  if (m_ignoreOptimalDataRates)
//    ParseDataRateModifications (rootNode);
//
//  XMLElement *optimalSolutionElement = rootNode->FirstChildElement ("OptimalSolution");
//  NS_ABORT_MSG_IF (optimalSolutionElement == nullptr, "OptimalSolution element not found");
//
//  XMLElement *flowElement = optimalSolutionElement->FirstChildElement ("Flow");
//
//  while (flowElement != nullptr)
//    {
//      FlowId_t flowId (0);
//      double dataRateInclHdr (0);
//      char protocol (*flowElement->Attribute ("Protocol"));
//
//      flowElement->QueryAttribute ("Id", &flowId);
//      flowElement->QueryAttribute ("DataRate", &dataRateInclHdr);
//
//      dataRateInclHdr = GetFlowDataRate (flowId, dataRateInclHdr);
//
//      // Do not install the application if the data rate is equal to 0 or it is
//      // an acknowledgement flow.
//      if (dataRateInclHdr == 0 || protocol == 'A')
//        {
//          flowElement = flowElement->NextSiblingElement ("Flow"); // Move to the next flow
//          continue;
//        }
//
//      uint32_t pktSizeInclHdr (0);
//      NodeId_t dstNodeId (0);
//      uint32_t srcPortNumber (0);
//      uint32_t dstPortNumber (0);
//
//      flowElement->QueryAttribute ("PacketSize", &pktSizeInclHdr);
//      flowElement->QueryAttribute ("DestinationNode", &dstNodeId);
//
//      if (protocol == 'U')
//        flowElement->QueryAttribute ("PortNumber", &dstPortNumber);
//      else
//        {
//          flowElement->QueryAttribute ("SrcPortNumber", &srcPortNumber);
//          flowElement->QueryAttribute ("DstPortNumber", &dstPortNumber);
//        }
//
//      uint32_t pktSizeExclHdr (pktSizeInclHdr - CalculateHeaderSize (protocol));
//      double dataRateExclHdr ((pktSizeExclHdr * dataRateInclHdr) / pktSizeInclHdr);
//
//      std::string socketProtocol ("");
//      if (protocol == 'T') //
//        socketProtocol = "ns3::TcpSocketFactory";
//      else if (protocol == 'U')
//        socketProtocol = "ns3::UdpSocketFactory";
//      else
//        NS_ABORT_MSG ("Unknown protocol");
//
//      InetSocketAddress destinationSocket (GetIpAddress (dstNodeId, allNodes), dstPortNumber);
//      OnOffHelper onOff (socketProtocol, destinationSocket);
//      onOff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
//      onOff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
//      onOff.SetAttribute ("PacketSize", UintegerValue (pktSizeExclHdr));
//      onOff.SetAttribute ("DataRate", DataRateValue (dataRateExclHdr * 1000000)); // In bps
//      onOff.SetAttribute ("SourcePort", UintegerValue (srcPortNumber));
//      if (nPacketsPerFlow > 0)
//        {
//          uint32_t maxBytes (nPacketsPerFlow * pktSizeExclHdr);
//          onOff.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
//        }
//      NodeId_t srcNodeId (0);
//      uint32_t startTime (0);
//
//      flowElement->QueryAttribute ("SourceNode", &srcNodeId);
//      flowElement->QueryAttribute ("StartTime", &startTime);
//
//      // Configuring the transmitter
//      ApplicationContainer sourceApplication = onOff.Install (allNodes.Get (srcNodeId));
//      sourceApplication.Start (Seconds (startTime));
//
//      // Configuring the receiver
//      PacketSinkHelper sinkHelper (socketProtocol, destinationSocket);
//      ApplicationContainer destinationApplication = sinkHelper.Install (allNodes.Get (dstNodeId));
//      destinationApplication.Start (Seconds (startTime));
//
//      // Log Flow Details
//      NS_LOG_INFO ("Flow Entry"
//                   << "\n"
//                   << "  Id: " << flowId << "\n"
//                   << "  Source: " << srcNodeId << "\n"
//                   << "  Destination: " << dstNodeId << "\n"
//                   << "  Src Port: " << srcPortNumber << "\n"
//                   << "  Dst Port: " << dstPortNumber << "\n"
//                   << "  Protocol: " << protocol << "\n"
//                   << "  Packet Size Incl Hdr (bytes): " << pktSizeInclHdr << "\n"
//                   << "  Packet Size Excl Hdr (bytes): " << pktSizeExclHdr << "\n"
//                   << "  Data Rate Incl Hdr (Mbps): " << dataRateInclHdr << "\n"
//                   << "  Data Rate Excl Hdr (Mbps): " << dataRateExclHdr);
//
//      NS_LOG_INFO ("");
//      // Monitor the PacketSink application
//      applicationMonitor.MonitorApplication (flowId, dataRateExclHdr,
//                                             destinationApplication.Get (0));
//
//      flowElement = flowElement->NextSiblingElement ("Flow");
//    }
//}

//void
//ApplicationHelper::ParseDataRateModifications (XMLNode *rootNode)
//{
//  XMLElement *dataRateModsElement = rootNode->FirstChildElement ("FlowDataRateModifications");
//
//  // This element does not exist in the XML. This means that all flows can transmit the
//  // requested data without any modifications.
//  if (dataRateModsElement == nullptr)
//    return;
//
//  XMLElement *flowElement = dataRateModsElement->FirstChildElement ("Flow");
//
//  while (flowElement != nullptr)
//    {
//      FlowId_t flowId (0);
//      FlowDataRate flowDataRate;
//
//      flowElement->QueryAttribute ("Id", &flowId);
//      flowElement->QueryAttribute ("RequestedDataRate", &flowDataRate.requestedDataRate);
//      flowElement->QueryAttribute ("ReceivedDataRate", &flowDataRate.receivedDataRate);
//
//      auto ret = m_updatedFlows.insert ({flowId, flowDataRate});
//
//      if (ret.second == false)
//        NS_ABORT_MSG ("Duplicate Flow ID(" << flowId
//                                           << ") in the FlowDataRateModifications section found.");
//
//      flowElement = flowElement->NextSiblingElement ("Flow");
//    }
//}
//
//double
//ApplicationHelper::GetFlowDataRate (FlowId_t flowId, double optimalFlowDr)
//{
//  try
//    {
//      if (m_ignoreOptimalDataRates)
//        return m_updatedFlows.at (flowId).requestedDataRate;
//      else
//        return optimalFlowDr;
//  } catch (std::out_of_range)
//    {
//      // The Flow in question does not have any data rate modifications.
//      // Return the optimal flow data rate.
//      return optimalFlowDr;
//  }
//}
//
//uint32_t
//ApplicationHelper::CalculateHeaderSize (char protocol)
//{
//  switch (protocol)
//    {
//    case 'T': // TCP (TCP:20 + 12 options + IP:20 + P2P:2)
//      return 54;
//      break;
//    case 'U': // UDP (UDP:8 + IP:20 + P2P:2)
//      return 30;
//      break;
//    default: // Error
//      NS_ABORT_MSG ("Unknown protocol");
//    }
//}
//
//Ipv4Address
//ApplicationHelper::GetIpAddress (NodeId_t nodeId, ns3::NodeContainer &nodes)
//{
//  return nodes.Get (nodeId)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
//}
