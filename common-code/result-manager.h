#ifndef RESULT_MANAGER_H
#define RESULT_MANAGER_H

#include <memory>
#include <map>
#include <tinyxml2.h>

#include "ns3/flow-monitor-helper.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/ipv4.h"

#include "definitions.h"
#include "switch-device.h"
#include "../examples/ppfs/ppfs-switch.h"

class ResultManager
{
public:
  ResultManager ();

  void SetupFlowMonitor (ns3::NodeContainer& allNodes, uint32_t stopTime);
  void TraceTerminalTransmissions (ns3::NetDeviceContainer& terminalDevices,
                                   std::map <ns3::Ptr<ns3::NetDevice>, LinkId_t>& terminalToLinkId);
  void GenerateFlowMonitorXmlLog (bool enableHistograms, bool enableFlowProbes);
  void UpdateFlowIds (tinyxml2::XMLNode* logFileRootNode, ns3::NodeContainer& allNodes);
  template <typename SwitchType>
  void AddLinkStatistics (std::map<NodeId_t, SwitchType>& switchMap);
  template <typename SwitchType>
  void AddQueueStatistics (std::map<NodeId_t, SwitchType>& switchMap);
  void AddSwitchDetails (std::map<NodeId_t, PpfsSwitch>& switchMap);
  void SaveXmlResultFile (const char* resultPath);

private:
  // Function will be called every time a terminal transmits a packet
  void TerminalPacketTransmission (std::string context, ns3::Ptr<const ns3::Packet> packet);
  void InsertTimeStamp (tinyxml2::XMLNode* node);

  // Inline functions /////////////////////////////////////////////////////////
  inline uint32_t GetIpAddress (NodeId_t nodeId, ns3::NodeContainer& allNodes)
  {
    return allNodes.Get(nodeId)->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal().Get();
  }
  inline tinyxml2::XMLNode* GetRootNode () { return m_xmlResultFile->FirstChild(); }

  ns3::Ptr<ns3::FlowMonitor> m_flowMonitor;
  ns3::FlowMonitorHelper m_flowMonHelper;
  std::unique_ptr<tinyxml2::XMLDocument> m_xmlResultFile;
  /*
   * Key -> LinkId, Value -> Link Statistic
   * This map will store the link statistics of links that are originating from the
   * terminals.
   */
  std::map<LinkId_t, LinkStatistic> m_terminalLinkStatistics;
};

#include "ns3/queue.h"

using namespace tinyxml2;
using namespace ns3;

template <typename SwitchType>
void
ResultManager::AddLinkStatistics(std::map<NodeId_t, SwitchType>& switchMap)
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
      LinkId_t prevLinkId (0);
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

template <typename SwitchType>
void
ResultManager::AddQueueStatistics(std::map<NodeId_t, SwitchType>& switchMap)
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
#endif /* RESULT_MANAGER_H */
