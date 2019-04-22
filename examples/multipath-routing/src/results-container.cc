#include <chrono>
#include <sstream>
#include <iomanip>
#include <boost/numeric/conversion/cast.hpp>

#include "device/switch/sdn-switch.h"
#include "device/switch/ppfs-switch.h"
#include "results-container.h"
#include "application/unipath-receiver.h"
#include "application/multipath-receiver.h"
#include "application/unipath-transmitter.h"
#include "application/multipath-transmitter.h"

using ns3::Time;
using namespace tinyxml2;

NS_LOG_COMPONENT_DEFINE ("ResultsContainer");

PacketDetails::PacketDetails (Time transmitted, packetSize_t transmittedDataSize)
    : transmitted (transmitted), transmittedDataSize (transmittedDataSize)
{
}

ResultsContainer::ResultsContainer (bool logPacketResults) : m_logPacketResults (logPacketResults)
{
  XMLNode *rootElement = m_xmlDoc.NewElement ("Log");
  m_rootNode = m_xmlDoc.InsertFirstChild (rootElement);
  InsertTimeStamp ();

  if (m_rootNode == nullptr)
    {
      throw std::runtime_error ("Could not create element node");
    }
}

void
ResultsContainer::SetupFlowResults (const Flow::flowContainer_t &flows)
{
  // Generate a new FlowResults entry for every existing flow
  for (const auto &flowPair : flows)
    {
      auto flowId = flowPair.first;
      m_flowResults.emplace (flowId, FlowResults ());
    }
}

void
ResultsContainer::SetupSwitchResults (const SwitchContainer &switchContainer)
{
  // Generate a new SwitchResults entry for every existing switch
  for (const auto &switchPair : switchContainer)
    {
      auto switchId = switchPair.first;
      m_switchResults.emplace (switchId, SwitchResults ());
    }
}

void
ResultsContainer::LogFlowTxGoodputRate (id_t flowId, double goodputRate)
{
  auto &flowResult = m_flowResults.at (flowId);
  flowResult.txGoodput = goodputRate;
}

void
ResultsContainer::LogPacketTransmission (id_t flowId, const Time &time, packetNumber_t pktNumber,
                                         packetSize_t dataSize)
{
  auto &flowResult = m_flowResults.at (flowId);

  // Check to make sure that there are no duplicates
  NS_ABORT_MSG_IF (flowResult.packetResults.find (pktNumber) != flowResult.packetResults.end (),
                   "Flow " << flowId << ": The packet " << pktNumber
                           << " has been already logged for transmission");

  flowResult.packetResults.emplace (pktNumber, PacketDetails (time, dataSize));
  NS_LOG_INFO (time.GetSeconds () << "s: Flow " << flowId << " transmitted packet " << pktNumber
                                  << " (" << dataSize << " data bytes)");
}

void
ResultsContainer::LogPacketReception (id_t flowId, const Time &time, packetNumber_t pktNumber,
                                      packetSize_t dataSize)
{
  auto &flowResult = m_flowResults.at (flowId);

  // Check to make sure that there are no duplicates
  NS_ABORT_MSG_IF (flowResult.packetResults.find (pktNumber) == flowResult.packetResults.end (),
                   "Flow " << flowId << ": The packet " << pktNumber
                           << " has never been transmitted");

  // Log the time the flow received the first and last packet
  if (flowResult.firstPacketReceived == false)
    {
      flowResult.firstPacketReceived = true;
      flowResult.firstReception = time;
    }

  flowResult.lastReception = time;

  // Log the total number of received bytes and total number of packets received
  flowResult.totalRecvBytes += dataSize;
  flowResult.totalRecvPackets++;

  auto &packetDetail = flowResult.packetResults.at (pktNumber);

  packetDetail.received = time;
  packetDetail.receivedDataSize = dataSize;

  // Output a warning if the packet sizes do not match
  NS_ABORT_MSG_IF (
      packetDetail.transmittedDataSize != packetDetail.receivedDataSize,
      "Flow " << flowId
              << ": The size of the transmitted and received packet does not match. Packet Number "
              << pktNumber << ". Transmitted size: " << packetDetail.transmittedDataSize
              << " Received Size: " << packetDetail.receivedDataSize);

  NS_LOG_INFO (time.GetSeconds () << "s: Flow " << flowId << " received packet " << pktNumber
                                  << " (" << dataSize << " data bytes)");

  // We calculate the total delay here
  flowResult.totalDelay += packetDetail.received - packetDetail.transmitted;

  if (m_logPacketResults == false)
    {
      // If non for every packet delete
      auto numElementsRemoved = flowResult.packetResults.erase (pktNumber);
      NS_ABORT_MSG_IF (numElementsRemoved != 1, "The details of flow " << flowId << " packet "
                                                                       << pktNumber
                                                                       << " have not been removed");
    }
}

void
ResultsContainer::LogPacketDrop (id_t switchId, const ns3::Time &time)
{
  auto &switchResult = m_switchResults.at (switchId);
  switchResult.numDroppedPackets++;

  NS_LOG_INFO (time.GetSeconds ()
               << "s: Switch " << switchId
               << "dropped a packet due to buffer overflow.\n  Total number of dropped packets: "
               << switchResult.numDroppedPackets);
}

void
ResultsContainer::AddFlowResults ()
{
  /*
   * NOTE: We may need to delete the pending items here if the GEANT simulation result files
   * become too big.
   */

  NS_LOG_INFO ("Saving the flow results");

  using boost::numeric_cast;

  using boost::numeric::bad_numeric_cast;
  using boost::numeric::negative_overflow;
  using boost::numeric::positive_overflow;

  auto flowId = id_t{0};

  try
    {
      XMLElement *resultsElement = m_xmlDoc.NewElement ("Results");

      for (const auto &flowResultsPair : m_flowResults)
        {
          flowId = flowResultsPair.first;
          auto &flowResult = flowResultsPair.second;

          NS_LOG_INFO ("Saving the results of Flow: " << flowId);

          XMLElement *flowElement{m_xmlDoc.NewElement ("Flow")};
          flowElement->SetAttribute ("Id", flowId);
          flowElement->SetAttribute ("TxGoodput", flowResult.txGoodput);
          flowElement->SetAttribute (
              "TimeFirstRx", numeric_cast<double> (flowResult.firstReception.GetNanoSeconds ()));
          flowElement->SetAttribute (
              "TimeLastRx", numeric_cast<double> (flowResult.lastReception.GetNanoSeconds ()));
          flowElement->SetAttribute ("NumRecvPackets",
                                     numeric_cast<double> (flowResult.totalRecvPackets));
          flowElement->SetAttribute ("TotalRecvBytes",
                                     numeric_cast<double> (flowResult.totalRecvBytes));
          flowElement->SetAttribute (
              "TotalDelay", numeric_cast<double> (flowResult.totalDelay.GetNanoSeconds ()));

          for (const auto &packetResult : flowResult.packetResults)
            {
              auto &packetNumber{packetResult.first};
              auto &packetDetails{packetResult.second};

              XMLElement *packetElement{m_xmlDoc.NewElement ("Packet")};
              packetElement->SetAttribute ("Id", packetNumber);
              packetElement->SetAttribute (
                  "Transmitted",
                  numeric_cast<double> (packetDetails.transmitted.GetNanoSeconds ()));
              packetElement->SetAttribute (
                  "Received", numeric_cast<double> (packetDetails.received.GetNanoSeconds ()));
              packetElement->SetAttribute ("TxDataSize", packetDetails.transmittedDataSize);
              packetElement->SetAttribute ("RxDataSize", packetDetails.receivedDataSize);

              flowElement->InsertEndChild (packetElement);
            }
          resultsElement->InsertEndChild (flowElement);
        }

      XMLComment *comment = m_xmlDoc.NewComment ("Timings are given in NanoSeconds");
      resultsElement->InsertFirstChild (comment);

      m_rootNode->InsertEndChild (resultsElement);
  } catch (const positive_overflow &e)
    {
      NS_ABORT_MSG ("Saving the flow results of flow "
                    << flowId << "caused a positive overflow.\nError message: " << e.what ());
  } catch (const negative_overflow &e)
    {
      NS_ABORT_MSG ("Saving the flow results of flow "
                    << flowId << "caused a negative overflow.\nError message: " << e.what ());
  } catch (const bad_numeric_cast &e)
    {
      NS_ABORT_MSG ("Saving the flow results of flow "
                    << flowId << "caused a bad numeric cast.\nError message: " << e.what ());
  }
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
