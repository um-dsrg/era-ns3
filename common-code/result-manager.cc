#include <iomanip>
#include <chrono>
#include <sstream>
#include <string>

#include "ns3/log.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/point-to-point-net-device.h"

#include "result-manager.h"

using namespace ns3;
using namespace tinyxml2;

NS_LOG_COMPONENT_DEFINE ("ResultManager");

ResultManager::ResultManager() : m_xmlResultFile (new XMLDocument)
{}

void
ResultManager::SetupFlowMonitor (NodeContainer& allNodes)
{
  // Enable flow monitor on all the nodes
  m_flowMonitor = m_flowMonHelper.Install (allNodes);
  m_flowMonitor->SetAttribute ("MaxPerHopDelay", TimeValue (NanoSeconds (9223372036854775807.0)));
}

void
ResultManager::TraceTerminalTransmissions (ns3::NetDeviceContainer &terminalDevices,
    std::map <Ptr<NetDevice>, LinkId_t>& terminalToLinkId)
{
  for (auto terminalDevice = terminalDevices.Begin(); terminalDevice != terminalDevices.End();
       ++terminalDevice)
    {
      auto ret = terminalToLinkId.find ((*terminalDevice));
      if (ret == terminalToLinkId.end())
        {
          NS_LOG_INFO ("WARNING: The Node " << (*terminalDevice)->GetNode()->GetId() << " has no "
                       "outgoing links");
          continue;
        }

      // Setting the link Id as the context for the callback function
      (*terminalDevice)->TraceConnect ("PhyTxBegin", std::to_string (ret->second),
                                       MakeCallback (&ResultManager::TerminalPacketTransmission,
                                           this));
    }
}

void
ResultManager::GenerateFlowMonitorXmlLog (bool enableHistograms, bool enableFlowProbes)
{
  XMLError eResult  = m_xmlResultFile->Parse (m_flowMonitor->SerializeToXmlString (2,
                      enableHistograms,
                      enableFlowProbes)
                      .c_str());
  NS_ABORT_MSG_IF (eResult != XML_SUCCESS, "TinyXml could not parse FlowMonitor results");


  // Get the Root element and rename it from FlowMonitor -> Results
  XMLNode* rootNode = GetRootNode();
  rootNode->SetValue ("Results");
  InsertTimeStamp (rootNode);
}

void
ResultManager::UpdateFlowIds (XMLNode* logFileRootNode, NodeContainer& allNodes)
{
  // Parsing our flows ////////////////////////////////////////////////////////
  XMLElement* optimalSolutionElement = logFileRootNode->FirstChildElement ("OptimalSolution");
  NS_ABORT_MSG_IF (optimalSolutionElement == nullptr, "OptimalSolution element not found");

  std::map<Flow, FlowId_t> flowToIdMap; /*!< Key -> Flow Details, Value -> Flow Id */
  XMLElement* flowElement = optimalSolutionElement->FirstChildElement ("Flow");
  while (flowElement != nullptr)
    {
      NodeId_t srcNodeId (0);
      NodeId_t dstNodeId (0);
      FlowId_t flowId (0);
      flowElement->QueryAttribute ("SourceNode", &srcNodeId);
      flowElement->QueryAttribute ("DestinationNode", &dstNodeId);
      flowElement->QueryAttribute ("Id", &flowId);

      Flow flow;
      flow.srcIpAddr = GetIpAddress (srcNodeId, allNodes);
      flow.dstIpAddr = GetIpAddress (dstNodeId, allNodes);
      flow.SetProtocol (*flowElement->Attribute ("Protocol"));

      // This conversion is needed because tinyxml2 does not handle uint16_t parameters
      uint32_t portNumber;
      if (flow.GetProtocol() == Flow::Protocol::Tcp)
        {
          flowElement->QueryAttribute ("DstPortNumber", &portNumber);
          flow.dstPortNumber = (uint16_t) portNumber;
        }
      else
        {
          flowElement->QueryAttribute ("PortNumber", &portNumber);
          flow.dstPortNumber = (uint16_t) portNumber;
        }
      flowToIdMap.insert ({flow, flowId});

      flowElement = flowElement->NextSiblingElement ("Flow");
    }

  FlowMonitor::FlowStatsContainer flowStatistics = m_flowMonitor->GetFlowStats();
  Ptr<Ipv4FlowClassifier> flowClassifier =
    DynamicCast<Ipv4FlowClassifier> (m_flowMonHelper.GetClassifier());
  NS_ASSERT (flowClassifier != 0); // Checking that the type cast works.

  // Parsing the FlowMonitor's flows //////////////////////////////////////////
  // Key -> FlowMonitor Flow Id, Value -> Our FlowId
  std::map<FlowId_t, FlowId_t> flowMonIdToFlowId;
  for (const auto& flow : flowStatistics)
    {
      uint32_t flowMonFlowId (flow.first);
      Ipv4FlowClassifier::FiveTuple flowMonFlow = flowClassifier->FindFlow (flowMonFlowId);

      Flow flowDetails;
      flowDetails.srcIpAddr = flowMonFlow.sourceAddress.Get();
      flowDetails.dstIpAddr = flowMonFlow.destinationAddress.Get();
      flowDetails.dstPortNumber = flowMonFlow.destinationPort; // Destination Port Number

      // Protocol numbers taken from ipv4-flow-classifier.cc
      if (flowMonFlow.protocol == 6) flowDetails.SetProtocol (Flow::Protocol::Tcp);
      else if (flowMonFlow.protocol == 17) flowDetails.SetProtocol (Flow::Protocol::Udp);

      auto ourFlowIt = flowToIdMap.find (flowDetails);
      NS_ABORT_MSG_IF (ourFlowIt == flowToIdMap.end(),
                       "FlowMonitor's flow could not be mapped to our flow set");
      flowMonIdToFlowId.insert ({flowMonFlowId, ourFlowIt->second});
    }

  // Need to update the XML File
  // Updating the FlowStats element ///////////////////////////////////////////
  XMLElement* flowStatsElement = GetRootNode()->FirstChildElement ("FlowStats");
  NS_ABORT_MSG_IF (flowStatsElement == nullptr, "FlowStats element not found");

  flowElement = flowStatsElement->FirstChildElement ("Flow");

  while (flowElement != nullptr)
    {
      FlowId_t flowMonFlowId (0);
      flowElement->QueryAttribute ("flowId", &flowMonFlowId);

      auto ret = flowMonIdToFlowId.find (flowMonFlowId);
      NS_ABORT_MSG_IF (ret == flowMonIdToFlowId.end(),
                       "Flow Id not found when parsing flow monitor results");
      flowElement->SetAttribute ("flowId", ret->second/* Our Flow Id */);

      flowElement = flowElement->NextSiblingElement ("Flow");
    }
  // Updating the Ipv4FlowClassifier element //////////////////////////////////
  XMLElement* flowClassifierElement = GetRootNode()->FirstChildElement ("Ipv4FlowClassifier");
  NS_ABORT_MSG_IF (flowClassifierElement == nullptr, "Ipv4FlowClassifier element not found");

  flowElement = flowClassifierElement->FirstChildElement ("Flow");

  while (flowElement != nullptr)
    {
      FlowId_t flowMonFlowId (0);
      flowElement->QueryAttribute ("flowId", &flowMonFlowId);

      auto ret = flowMonIdToFlowId.find (flowMonFlowId);
      NS_ABORT_MSG_IF (ret == flowMonIdToFlowId.end(),
                       "Flow Id not found when parsing flow monitor results");
      flowElement->SetAttribute ("flowId", ret->second/* Our Flow Id */);

      flowElement = flowElement->NextSiblingElement ("Flow");
    }
}

void
ResultManager::AddApplicationMonitorResults (const ApplicationMonitor &applicationMonitor)
{
  XMLElement* applicationMonitorElement = m_xmlResultFile->NewElement ("ApplicationMonitor");

  for (auto const& flow : applicationMonitor.GetFlowMap())
    {
      XMLElement* flowElement = m_xmlResultFile->NewElement ("Flow");

      flowElement->SetAttribute ("Id", flow.first);
      flowElement->SetAttribute ("BytesReceived",
                                 static_cast<unsigned int> (flow.second.nBytesReceived));
      flowElement->SetAttribute ("PacketsReceived",
                                 static_cast<unsigned int> (flow.second.nPacketsReceived));
      flowElement->SetAttribute ("RequestedGoodput", flow.second.requestedGoodput);
      flowElement->SetAttribute ("GoodputAtQuota", flow.second.goodputAtQuota);
      flowElement->SetAttribute ("TimeFirstRxPacket", flow.second.firstRxPacket.GetSeconds());
      flowElement->SetAttribute ("TimeLastRxPacket", flow.second.lastRxPacket.GetSeconds());

      applicationMonitorElement->InsertEndChild (flowElement);
    }

  XMLComment* comment = m_xmlResultFile->NewComment ("Time values are in Seconds");
  applicationMonitorElement->InsertFirstChild (comment);

  GetRootNode()->InsertFirstChild (applicationMonitorElement);
}

void
ResultManager::AddSwitchDetails (std::map<NodeId_t, PpfsSwitch>& switchMap)
{
  // switchMap: Key -> Node ID. Value -> Switch object
  XMLElement* switchDetailsElement = m_xmlResultFile->NewElement ("SwitchDetails");

  for (const auto& switchNode : switchMap) // Loop through all the switches
    {
      XMLElement* switchElement = m_xmlResultFile->NewElement ("Switch");
      switchElement->SetAttribute ("Id", switchNode.first);
      switchElement->SetAttribute ("Seed", switchNode.second.GetSeed());
      switchElement->SetAttribute ("Run", switchNode.second.GetRun());

      switchDetailsElement->InsertEndChild (switchElement);
    }

  GetRootNode()->InsertFirstChild (switchDetailsElement);
}

void
ResultManager::SaveXmlResultFile (const char* resultPath)
{
  m_xmlResultFile->InsertFirstChild (m_xmlResultFile->NewDeclaration());
  XMLError saveResult = m_xmlResultFile->SaveFile (resultPath);
  NS_ABORT_MSG_IF (saveResult != XML_SUCCESS, "Could not save XML results file");
}

void
ResultManager::TerminalPacketTransmission (std::string context, ns3::Ptr<const ns3::Packet> packet)
{
  LinkId_t linkId (std::stoul (context));
  Simulator::Now();

  auto ret = m_terminalLinkStatistics.find (linkId);

  if (ret == m_terminalLinkStatistics.end()) // LinkId does not exist, create it
    {
      LinkStatistic linkStatistic;
      linkStatistic.timeFirstTx = Simulator::Now();
      linkStatistic.timeLastTx = Simulator::Now();
      linkStatistic.packetsTransmitted++;
      linkStatistic.bytesTransmitted += packet->GetSize();

      m_terminalLinkStatistics.insert ({linkId, linkStatistic});
    }
  else // LinkId exists, update it
    {
      LinkStatistic& linkStatistic = ret->second;
      linkStatistic.timeLastTx = Simulator::Now();
      linkStatistic.packetsTransmitted++;
      linkStatistic.bytesTransmitted += packet->GetSize();
    }
}

void
ResultManager::InsertTimeStamp (XMLNode* node)
{
  std::chrono::time_point<std::chrono::system_clock> currentTime (std::chrono::system_clock::now());
  std::time_t currentTimeFormatted = std::chrono::system_clock::to_time_t (currentTime);

  std::stringstream ss;
  ss << std::put_time (std::localtime (&currentTimeFormatted), "%a %d-%m-%Y %T");

  XMLElement* rootElement = node->ToElement();
  rootElement->SetAttribute ("Generated", ss.str().c_str());
}
