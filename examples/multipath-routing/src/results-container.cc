#include <chrono>
#include <sstream>
#include <iomanip>
#include <boost/numeric/conversion/cast.hpp>

#include "sdn-switch.h"
#include "ppfs-switch.h"
#include "results-container.h"
#include "application/unipath-receiver.h"
#include "application/multipath-receiver.h"
#include "application/unipath-transmitter.h"
#include "application/multipath-transmitter.h"

using ns3::Time;
using namespace tinyxml2;

NS_LOG_COMPONENT_DEFINE ("ResultsContainer");

PacketDetails::PacketDetails (Time transmitted, packetSize_t dataSize)
    : transmitted (transmitted), dataSize (dataSize)
{
}

ResultsContainer::ResultsContainer (const Flow::flowContainer_t &flows)
{
  XMLNode *rootElement = m_xmlDoc.NewElement ("Log");
  m_rootNode = m_xmlDoc.InsertFirstChild (rootElement);
  InsertTimeStamp ();

  if (m_rootNode == nullptr)
    {
      throw std::runtime_error ("Could not create element node");
    }

  // Generate a new FlowResults entry for every existing flow
  for (const auto &flowPair : flows)
    {
      auto flowId = flowPair.first;
      m_flowResults.emplace (flowId, FlowResults ());
    }
}

void
ResultsContainer::LogPacketTransmission (id_t flowId, Time time, packetNumber_t pktNumber,
                                         packetSize_t dataSize)
{
  auto &flowResult = m_flowResults.at (flowId);

  // Check to make sure that there are no duplicates
  NS_ABORT_MSG_IF (flowResult.packetDetails.find (pktNumber) != flowResult.packetDetails.end (),
                   "Flow " << flowId << ": The packet " << pktNumber
                           << " has been already logged for transmission");

  flowResult.packetDetails.emplace (pktNumber, PacketDetails (time, dataSize));
  NS_LOG_INFO ("Flow " << flowId << " transmitted packet " << pktNumber << " (" << dataSize
                       << " data bytes) at " << time.GetSeconds () << "s");
}

void
ResultsContainer::LogPacketReception (id_t flowId, Time time, packetNumber_t pktNumber,
                                      packetSize_t dataSize)
{
  auto &flowResult = m_flowResults.at (flowId);

  // Check to make sure that there are no duplicates
  NS_ABORT_MSG_IF (flowResult.packetDetails.find (pktNumber) == flowResult.packetDetails.end (),
                   "Flow " << flowId << ": The packet " << pktNumber
                           << " has never been transmitted");

  auto &packetDetail = flowResult.packetDetails.at (pktNumber);

  // Ensure that the packet sizes match
  if (packetDetail.dataSize != dataSize)
    {
      NS_ABORT_MSG (
          "Flow: "
          << flowId
          << ": The size of the transmitted and received packet does not match. Packet Number "
          << pktNumber << ". Transmitted size: " << packetDetail.dataSize
          << " Received Size: " << dataSize);
    }

  packetDetail.received = time;
  NS_LOG_INFO ("Flow " << flowId << " received packet " << pktNumber << " (" << dataSize
                       << " data bytes) at " << time.GetSeconds () << "s");
}

// void
// ResultManager::AddGoodputResults (
//     const Flow::FlowContainer &flows,
//     const AppContainer::applicationContainer_t &transmitterApplications,
//     const AppContainer::applicationContainer_t &receiverApplications)
// {
//   NS_LOG_INFO ("Building the goodput results");

//   XMLElement *goodputElement = m_xmlDoc.NewElement ("Goodput");

//   for (const auto &receiverApplicationPair : receiverApplications)
//     {
//       auto flowId{receiverApplicationPair.first};

//       Ptr<ApplicationBase> receiverApp;
//       receiverApp = DynamicCast<ReceiverApp> (receiverApplicationPair.second);
//       if (receiverApp == nullptr)
//         {
//           receiverApp = DynamicCast<SinglePathReceiver> (receiverApplicationPair.second);
//         }

//       Ptr<ApplicationBase> transmitterApp;
//       transmitterApp = DynamicCast<TransmitterApp> (transmitterApplications.at (flowId));
//       if (transmitterApp == nullptr)
//         {
//           transmitterApp =
//               DynamicCast<SinglePathTransmitterApp> (transmitterApplications.at (flowId));
//           NS_LOG_INFO ("Transmitter app of type SinglePathTransmitter");
//         }
//       else
//         {
//           NS_LOG_INFO ("Transmitter app of type MultipathTransmitter");
//         }

//       const auto &flow{flows.at (flowId)};

//       XMLElement *flowElement = m_xmlDoc.NewElement ("Flow");
//       flowElement->SetAttribute ("Id", flowId);
//       flowElement->SetAttribute ("TxThroughput", (flow.dataRate.GetBitRate () / 1000000.0));
//       flowElement->SetAttribute ("TxGoodput", transmitterApp->GetTxGoodput ());
//       flowElement->SetAttribute ("MeanRxGoodPut", receiverApp->GetMeanRxGoodput ());

//       goodputElement->InsertEndChild (flowElement);
//     }

//   XMLComment *comment = m_xmlDoc.NewComment ("Goodput values are in Mbps");
//   goodputElement->InsertFirstChild (comment);

//   m_rootNode->InsertEndChild (goodputElement);
// }

// void
// ResultManager::AddDelayResults (const AppContainer::applicationContainer_t &transmitterApplications,
//                                 const AppContainer::applicationContainer_t &receiverApplications)
// {
//   NS_LOG_INFO ("Building the delay results");

//   XMLElement *delayElement = m_xmlDoc.NewElement ("Delay");

//   for (const auto &receiverApplicationPair : receiverApplications)
//     {
//       auto flowId{receiverApplicationPair.first};
//       NS_LOG_DEBUG ("Calculating delay for flow " << flowId);

//       XMLElement *flowElement = m_xmlDoc.NewElement ("Flow");
//       flowElement->SetAttribute ("Id", flowId);

//       Ptr<ApplicationBase> receiverApp;
//       receiverApp = DynamicCast<ReceiverApp> (receiverApplicationPair.second);
//       if (receiverApp == nullptr)
//         {
//           receiverApp = DynamicCast<SinglePathReceiver> (receiverApplicationPair.second);
//         }

//       Ptr<ApplicationBase> transmitterApp;
//       transmitterApp = DynamicCast<TransmitterApp> (transmitterApplications.at (flowId));
//       if (transmitterApp == nullptr)
//         {
//           transmitterApp =
//               DynamicCast<SinglePathTransmitterApp> (transmitterApplications.at (flowId));
//         }

//       const auto &receiverDelayLog{receiverApp->GetDelayLog ()};
//       const auto &transmitterDelayLog{transmitterApp->GetDelayLog ()};

//       for (const auto &delayLogEntry : receiverDelayLog)
//         {
//           auto pktNumber{delayLogEntry.first};
//           NS_LOG_DEBUG ("Working on packet " << pktNumber);

//           auto recvTime{delayLogEntry.second};
//           auto transmittedTime{transmitterDelayLog.at (pktNumber)};
//           auto delayInNs{(recvTime - transmittedTime).GetNanoSeconds ()};

//           XMLElement *packetElement = m_xmlDoc.NewElement ("Packet");
//           packetElement->SetAttribute ("Number", boost::numeric_cast<unsigned int> (pktNumber));
//           packetElement->SetAttribute ("Delay", boost::numeric_cast<double> (delayInNs));
//           flowElement->InsertEndChild (packetElement);
//         }

//       delayElement->InsertEndChild (flowElement);
//     }

//   XMLComment *comment = m_xmlDoc.NewComment ("Delay values are in nanoseconds");
//   delayElement->InsertFirstChild (comment);

//   m_rootNode->InsertEndChild (delayElement);
// }

// void
// ResultManager::AddDelayResults (const AppContainer &appHelper)
// {
//   NS_LOG_INFO ("Building the compressed delay results");

//   XMLElement *delayElement = m_xmlDoc.NewElement ("Delay");

//   for (const auto &compressedDelayLog : appHelper.GetCompressedDelayLog ())
//     {
//       const auto &flowId = compressedDelayLog.first;
//       const auto &delayLog = compressedDelayLog.second;

//       XMLElement *flowElement{m_xmlDoc.NewElement ("Flow")};
//       flowElement->SetAttribute ("Id", flowId);
//       flowElement->SetAttribute ("TotalRxPackets", static_cast<double> (delayLog.first));
//       flowElement->SetAttribute ("TotalDelayNs", delayLog.second);

//       delayElement->InsertEndChild (flowElement);
//     }

//   XMLComment *comment = m_xmlDoc.NewComment ("Delay values are in nanoseconds");
//   delayElement->InsertFirstChild (comment);

//   m_rootNode->InsertEndChild (delayElement);
// }

void
ResultsContainer::AddQueueStatistics (XMLElement *queueElement)
{
  m_rootNode->InsertEndChild (queueElement);
}

void
ResultsContainer::SaveFile (const std::string &path)
{
  if (m_xmlDoc.SaveFile (path.c_str ()) != tinyxml2::XML_SUCCESS)
    {
      throw std::runtime_error ("Could not save the result file in " + path);
    }
}

void
ResultsContainer::InsertTimeStamp ()
{
  using namespace std::chrono;
  time_point<system_clock> currentTime (system_clock::now ());
  std::time_t currentTimeFormatted = system_clock::to_time_t (currentTime);

  std::stringstream ss;
  ss << std::put_time (std::localtime (&currentTimeFormatted), "%a %d-%m-%Y %T");

  XMLElement *rootElement = m_rootNode->ToElement ();
  rootElement->SetAttribute ("Generated", ss.str ().c_str ());
}
