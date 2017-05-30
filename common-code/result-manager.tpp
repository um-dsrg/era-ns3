#include "ns3/queue.h"

#include "result-manager.h"

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
