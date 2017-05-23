#include <iomanip>
#include <chrono>
#include <sstream>
#include <string>

#include "ns3/log.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/point-to-point-net-device.h"

#include "ppfs-switch.h"
#include "result-manager.h"

using namespace ns3;
using namespace tinyxml2;

NS_LOG_COMPONENT_DEFINE ("ResultManager");

ResultManager::ResultManager() : m_xmlResultFile(new XMLDocument)
{}

void
ResultManager::SetupFlowMonitor(NodeContainer& allNodes, uint32_t stopTime)
{
  m_flowMonitor = m_flowMonHelper.Install(allNodes); // Enable flow monitor on all the nodes
  m_flowMonitor->SetAttribute("MaxPerHopDelay", TimeValue(Seconds(stopTime)));
}

void
ResultManager::TraceTerminalTransmissions(ns3::NetDeviceContainer &terminalDevices,
                                          std::map <Ptr<NetDevice>, uint32_t>& terminalToLinkId)
{
  for (auto terminalDevice = terminalDevices.Begin(); terminalDevice != terminalDevices.End();
       ++terminalDevice)
    {
      auto ret = terminalToLinkId.find((*terminalDevice));
      if (ret == terminalToLinkId.end())
        {
          NS_LOG_INFO("WARNING: The Node " << (*terminalDevice)->GetNode()->GetId() << " has no "
                      "outgoing links");
          continue;
        }

      // Setting the link Id as the context for the callback function
      (*terminalDevice)->TraceConnect("PhyTxBegin", std::to_string(ret->second),
                                      MakeCallback(&ResultManager::TerminalPacketTransmission,
                                                   this));
    }
}

void
ResultManager::GenerateFlowMonitorXmlLog()
{
  XMLError eResult  = m_xmlResultFile->Parse(m_flowMonitor->SerializeToXmlString(2, true, false)
                                             .c_str());
  NS_ABORT_MSG_IF(eResult != XML_SUCCESS, "TinyXml could not parse FlowMonitor results");


  // Get the Root element and rename it from FlowMonitor -> Results
  XMLNode* rootNode = GetRootNode();
  rootNode->SetValue("Results");
  InsertTimeStamp(rootNode);
}

void
ResultManager::UpdateFlowIds(XMLNode* logFileRootNode, NodeContainer& allNodes)
{
  // Parsing our flows ////////////////////////////////////////////////////////
  XMLElement* optimalSolutionElement = logFileRootNode->FirstChildElement("OptimalSolution");
  NS_ABORT_MSG_IF(optimalSolutionElement == nullptr, "OptimalSolution element not found");

  struct FlowDetails
  {
    uint32_t srcIpAddr;
    uint32_t dstIpAddr;
    uint32_t portNumber; // Destination Port Number
    char protocol; // T = TCP, U = UDP

    bool operator<(const FlowDetails &other) const
    {
      if (srcIpAddr == other.srcIpAddr)
        {
          if (dstIpAddr == other.dstIpAddr)
            return portNumber < other.portNumber;
          else
            return dstIpAddr < other.dstIpAddr;
        }
      else
        return srcIpAddr < other.srcIpAddr;
    }
  };

  std::map<FlowDetails, uint32_t> flowToIdMap; /*!< Key -> Flow Details, Value -> Flow Id */
  XMLElement* flowElement = optimalSolutionElement->FirstChildElement("Flow");
  while (flowElement != nullptr)
    {
      uint32_t srcNodeId (0);
      uint32_t dstNodeId (0);
      uint32_t flowId (0);
      flowElement->QueryAttribute("SourceNode", &srcNodeId);
      flowElement->QueryAttribute("DestinationNode", &dstNodeId);
      flowElement->QueryAttribute("Id", &flowId);

      FlowDetails flow;
      flow.srcIpAddr = GetIpAddress(srcNodeId, allNodes);
      flow.dstIpAddr = GetIpAddress(dstNodeId, allNodes);
      flowElement->QueryAttribute("PortNumber", &flow.portNumber);
      flow.protocol = *flowElement->Attribute("Protocol");

      flowToIdMap.insert({flow, flowId});

      flowElement = flowElement->NextSiblingElement("Flow");
    }

  FlowMonitor::FlowStatsContainer flowStatistics = m_flowMonitor->GetFlowStats();
  Ptr<Ipv4FlowClassifier> flowClassifier =
    DynamicCast<Ipv4FlowClassifier> (m_flowMonHelper.GetClassifier());
  NS_ASSERT(flowClassifier != 0); // Checking that the type cast works.

  // Parsing the FlowMonitor's flows //////////////////////////////////////////
  // Key -> FlowMonitor Flow Id, Value -> Our FlowId
  std::map<uint32_t, uint32_t> flowMonIdToFlowId;
  for (const auto& flow : flowStatistics)
    {
      uint32_t flowMonFlowId (flow.first);
      Ipv4FlowClassifier::FiveTuple flowMonFlow = flowClassifier->FindFlow(flowMonFlowId);

      FlowDetails flowDetails;
      flowDetails.srcIpAddr = flowMonFlow.sourceAddress.Get();
      flowDetails.dstIpAddr = flowMonFlow.destinationAddress.Get();
      flowDetails.portNumber = flowMonFlow.destinationPort; // Destination Port Number

      // Protocol numbers taken from ipv4-flow-classifier.cc
      if (flowMonFlow.protocol == 6) flowDetails.protocol = 'T';
      else if (flowMonFlow.protocol == 17) flowDetails.protocol = 'U';

      auto ourFlowIt = flowToIdMap.find(flowDetails);
      NS_ABORT_MSG_IF(ourFlowIt == flowToIdMap.end(),
                      "FlowMonitor's flow could not be mapped to our flow set");
      flowMonIdToFlowId.insert({flowMonFlowId, ourFlowIt->second});
    }

  // Need to update the XML File
  // Updating the FlowStats element ///////////////////////////////////////////
  XMLElement* flowStatsElement = GetRootNode()->FirstChildElement("FlowStats");
  NS_ABORT_MSG_IF(flowStatsElement == nullptr, "FlowStats element not found");

  flowElement = flowStatsElement->FirstChildElement("Flow");

  while (flowElement != nullptr)
    {
      uint32_t flowMonFlowId (0);
      flowElement->QueryAttribute("flowId", &flowMonFlowId);

      auto ret = flowMonIdToFlowId.find(flowMonFlowId);
      NS_ABORT_MSG_IF(ret == flowMonIdToFlowId.end(),
                      "Flow Id not found when parsing flow monitor results");
      flowElement->SetAttribute("flowId", ret->second/* Our Flow Id */);

      flowElement = flowStatsElement->NextSiblingElement("Flow");
    }
  // Updating the Ipv4FlowClassifier element //////////////////////////////////
  XMLElement* flowClassifierElement = GetRootNode()->FirstChildElement("Ipv4FlowClassifier");
  NS_ABORT_MSG_IF(flowClassifierElement == nullptr, "Ipv4FlowClassifier element not found");

  flowElement = flowClassifierElement->FirstChildElement("Flow");

  while (flowElement != nullptr)
    {
      uint32_t flowMonFlowId (0);
      flowElement->QueryAttribute("flowId", &flowMonFlowId);

      auto ret = flowMonIdToFlowId.find(flowMonFlowId);
      NS_ABORT_MSG_IF(ret == flowMonIdToFlowId.end(),
                      "Flow Id not found when parsing flow monitor results");
      flowElement->SetAttribute("flowId", ret->second/* Our Flow Id */);

      flowElement = flowClassifierElement->NextSiblingElement("Flow");
    }
}

void
ResultManager::AddLinkStatistics(std::map<uint32_t, PpfsSwitch>& switchMap)
{
  // switchMap: Key -> Node ID. Value -> Switch object
  XMLElement* linkStatisticsElement = m_xmlResultFile->NewElement("LinkStatistics");

  // Terminal Link statistics /////////////////////////////////////////////////
  XMLElement* terminalConnectionsElement = m_xmlResultFile->NewElement("TerminalConnections");

  for (const auto& terminalLinkStatistic : m_terminalLinkStatistics )
    {
      XMLElement* linkElement = m_xmlResultFile->NewElement("Link");
      linkElement->SetAttribute("Id", terminalLinkStatistic.first);
      linkElement->SetAttribute("TimeFirstTx",
                                terminalLinkStatistic.second.timeFirstTx.GetSeconds());
      linkElement->SetAttribute("TimeLastTx",
                                terminalLinkStatistic.second.timeLastTx.GetSeconds());
      linkElement->SetAttribute("PacketsTransmitted",
                                terminalLinkStatistic.second.packetsTransmitted);
      linkElement->SetAttribute("BytesTransmitted",
                                terminalLinkStatistic.second.bytesTransmitted);
      terminalConnectionsElement->InsertEndChild(linkElement);
    }
  linkStatisticsElement->InsertEndChild(terminalConnectionsElement);

  // Switch Link Statistics ///////////////////////////////////////////////////
  XMLElement* switchConnectionsElement = m_xmlResultFile->NewElement("SwitchConnections");
  for (const auto& switchNode : switchMap) // Loop through all the switches
    {
      XMLElement* switchElement = m_xmlResultFile->NewElement("Switch");
      switchElement->SetAttribute("Id", switchNode.first);

      // Loop through all the links the switch is connected to
      uint32_t prevLinkId (0);
      bool firstLinkInserted (false);
      XMLElement* linkElement (0);
      for (const auto& linkStatistic : switchNode.second.GetLinkStatistics())
        {
          // Creating the link element
          if (firstLinkInserted == false || prevLinkId != linkStatistic.first.linkId)
            {
              // Store the link element before we generate a new one with a different link id
              if (linkElement) switchElement->InsertEndChild(linkElement);
              linkElement = m_xmlResultFile->NewElement("Link");
              linkElement->SetAttribute("Id", linkStatistic.first.linkId);
              firstLinkInserted = true;
            }

          XMLElement* flowElement = m_xmlResultFile->NewElement("Flow");
          flowElement->SetAttribute("Id", linkStatistic.first.flowId);
          flowElement->SetAttribute("TimeFirstTx",
                                    linkStatistic.second.timeFirstTx.GetSeconds());
          flowElement->SetAttribute("TimeLastTx",
                                    linkStatistic.second.timeLastTx.GetSeconds());
          flowElement->SetAttribute("PacketsTransmitted",
                                    linkStatistic.second.packetsTransmitted);
          flowElement->SetAttribute("BytesTransmitted",
                                    linkStatistic.second.bytesTransmitted);

          prevLinkId = linkStatistic.first.linkId;
          linkElement->InsertEndChild(flowElement);
        }
      // The last one is not inserted via the loop, so we need to insert it after the loop finishes
      if (linkElement) switchElement->InsertEndChild(linkElement);
      switchConnectionsElement->InsertEndChild(switchElement);
    }
  linkStatisticsElement->InsertEndChild(switchConnectionsElement);

  XMLComment* comment = m_xmlResultFile->NewComment("Time values are in Seconds");
  linkStatisticsElement->InsertFirstChild(comment);
  GetRootNode()->InsertFirstChild(linkStatisticsElement);
}

void
ResultManager::AddQueueStatistics(std::map<uint32_t, PpfsSwitch>& switchMap)
{
  // switchMap: Key -> Node ID. Value -> Switch object
  XMLElement* queueStatisticsElement = m_xmlResultFile->NewElement("QueueStatistics");

  for (const auto& switchNode : switchMap) // Loop through all the switches
    {
      XMLElement* switchElement = m_xmlResultFile->NewElement("Switch");
      switchElement->SetAttribute("Id", switchNode.first);

      // Loop through the queue results
      for (const auto& queueResult : switchNode.second.GetQueueResults())
        {
          XMLElement* linkElement = m_xmlResultFile->NewElement("Link");
          uint32_t linkId = queueResult.first;
          linkElement->SetAttribute("Id", linkId); // Link Id
          linkElement->SetAttribute("PeakNumOfPackets", queueResult.second.peakNumOfPackets);
          linkElement->SetAttribute("PeakNumOfBytes", queueResult.second.peakNumOfBytes);

          // Given the link id I need to access the queue and retreive it's details.
          Ptr<Queue> queue = switchNode.second.GetQueueFromLinkId(linkId);
          linkElement->SetAttribute("QueueMaxNumPackets", queue->GetMaxPackets());
          linkElement->SetAttribute("QueueMaxBytes", queue->GetMaxBytes());
          linkElement->SetAttribute("TotalDroppedPackets", queue->GetTotalDroppedPackets());
          linkElement->SetAttribute("TotalDroppedBytes", queue->GetTotalDroppedBytes());

          switchElement->InsertEndChild(linkElement);
        }

      queueStatisticsElement->InsertEndChild(switchElement);
    }

  XMLComment* comment = m_xmlResultFile->NewComment("The maximum number of packets/bytes stored"
                                                    " in the transmit queue during the simulation");
  queueStatisticsElement->InsertFirstChild(comment);
  GetRootNode()->InsertFirstChild(queueStatisticsElement);
}

void
ResultManager::SaveXmlResultFile(const char* resultPath)
{
  m_xmlResultFile->InsertFirstChild(m_xmlResultFile->NewDeclaration());
  XMLError saveResult = m_xmlResultFile->SaveFile(resultPath);
  NS_ABORT_MSG_IF(saveResult != XML_SUCCESS, "Could not save XML results file");
}

void
ResultManager::TerminalPacketTransmission(std::string context, ns3::Ptr<const ns3::Packet> packet)
{
  uint32_t linkId (std::stoul(context));
  Simulator::Now();

  auto ret = m_terminalLinkStatistics.find(linkId);

  if (ret == m_terminalLinkStatistics.end()) // LinkId does not exist, create it
    {
      LinkStatistic linkStatistic;
      linkStatistic.timeFirstTx = Simulator::Now();
      linkStatistic.timeLastTx = Simulator::Now();
      linkStatistic.packetsTransmitted++;
      linkStatistic.bytesTransmitted += packet->GetSize();

      m_terminalLinkStatistics.insert({linkId, linkStatistic});
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
ResultManager::InsertTimeStamp(XMLNode* node)
{
  std::chrono::time_point<std::chrono::system_clock> currentTime (std::chrono::system_clock::now());
  std::time_t currentTimeFormatted = std::chrono::system_clock::to_time_t(currentTime);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&currentTimeFormatted), "%a %d-%m-%Y %T");

  XMLElement* rootElement = node->ToElement();
  rootElement->SetAttribute("Generated", ss.str().c_str());
}

uint32_t
ResultManager::GetIpAddress (uint32_t nodeId, ns3::NodeContainer& allNodes)
{
  return allNodes.Get(nodeId)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal().Get();
}

XMLNode*
ResultManager::GetRootNode ()
{
  return m_xmlResultFile->FirstChild();
}
