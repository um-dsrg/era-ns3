#include <memory>

#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/core-module.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/inet-socket-address.h"

#include "app-container.h"
#include "unipath-receiver.h"
#include "multipath-receiver.h"
#include "unipath-transmitter.h"
#include "multipath-transmitter.h"

using namespace ns3;
using namespace tinyxml2;

NS_LOG_COMPONENT_DEFINE ("AppContainer");

void
AppContainer::InstallApplicationsOnTerminals (const Flow::FlowContainer &flows,
                                              const Terminal::TerminalContainer &terminals,
                                              bool usePpfsSwitches, ResultsContainer &resContainer)
{
  for (const auto &flowPair : flows)
    {
      NS_LOG_INFO ("Installing flow " << flowPair.first);
      const auto &flow{flowPair.second};

      if (flow.GetDataPaths ().size () == 1 || usePpfsSwitches)
        {
          InstallApplicationOnTerminal<UnipathTransmitter, UnipathReceiver> (flow, resContainer);
        }
      else
        {
          InstallApplicationOnTerminal<MultipathTransmitter, MultipathReceiver> (flow,
                                                                                 resContainer);
        }

      // Create the flow delay log entry.
      // m_flowDelayLog.emplace (flowPair.first, std::make_pair (0, 0.0));
    }
}

template <typename TransmitterApp, typename ReceiverApp>
void
AppContainer::InstallApplicationOnTerminal (const Flow &flow, ResultsContainer &resContainer)
{
  // Installing the transmitter
  std::unique_ptr<TransmitterApp> transmitterApp =
      std::make_unique<TransmitterApp> (flow, resContainer);
  Ptr<Application> nsTransmitterApp = transmitterApp.get ();
  flow.srcNode->GetNode ()->AddApplication (nsTransmitterApp);
  transmitterApp->SetStartTime (Seconds (0.0));
  m_transmitterApplications.emplace (flow.id, std::move (transmitterApp));

  // Installing the receiver
  std::unique_ptr<ReceiverApp> receiverApp = std::make_unique<ReceiverApp> (flow, resContainer);
  Ptr<Application> nsReceiverApp = receiverApp.get ();
  flow.dstNode->GetNode ()->AddApplication (nsReceiverApp);
  receiverApp->SetStartTime (Seconds (0.0));
  m_receiverApplications.emplace (flow.id, std::move (receiverApp));
}

// const AppContainer::applicationContainer_t &
// AppContainer::GetTransmitterApps () const
// {
//   return m_transmitterApplications;
// }

// const AppContainer::applicationContainer_t &
// AppContainer::GetReceiverApps () const
// {
//   return m_receiverApplications;
// }

// const std::map<id_t, std::pair<uint64_t, double>> &
// AppContainer::GetCompressedDelayLog () const
// {
//   return m_flowDelayLog;
// }

// void
// AppContainer::CompressDelayLog ()
// {
//   NS_LOG_INFO ("Compressing the delay log at " << Simulator::Now ());

//   for (auto &receiverApplicationPair : m_receiverApplications)
//     {
//       auto flowId{receiverApplicationPair.first};
//       NS_LOG_INFO ("Compressing the delay log for Flow: " << flowId);

//       Ptr<ApplicationBase> receiverApp =
//           DynamicCast<ApplicationBase> (receiverApplicationPair.second);
//       NS_ABORT_MSG_IF (receiverApp == nullptr,
//                        "The receiver application could not be converted to base");

//       Ptr<ApplicationBase> transmitterApp =
//           DynamicCast<ApplicationBase> (m_transmitterApplications.at (flowId));
//       NS_ABORT_MSG_IF (transmitterApp == nullptr,
//                        "The transmitter application could not be converted to base");

//       auto &receiverDelayLog{receiverApp->m_delayLog};
//       auto &transmitterDelayLog{transmitterApp->m_delayLog};

//       std::list<packetNumber_t> packetsToRemove;

//       std::pair<uint64_t /* number of packets */, double /* total delay */> &flowDelayLog{
//           m_flowDelayLog.at (flowId)};

//       for (const auto &delayLogEntry : receiverDelayLog)
//         {
//           auto pktNumber{delayLogEntry.first};
//           NS_LOG_INFO ("Working on packet number: " << pktNumber);

//           auto recvTime{delayLogEntry.second};
//           auto transmittedTime{transmitterDelayLog.at (pktNumber)};
//           auto delayInNs{(recvTime - transmittedTime).GetNanoSeconds ()};

//           flowDelayLog.first++;
//           flowDelayLog.second += delayInNs;

//           NS_LOG_INFO ("Total Number of packets received: " << flowDelayLog.first);
//           NS_LOG_INFO ("Total Delay: " << flowDelayLog.second << "ns");
//           packetsToRemove.emplace_back (pktNumber);
//         }

//       for (const auto &packetNumToRemove : packetsToRemove)
//         {
//           NS_LOG_INFO ("Removing packet " << packetNumToRemove << " from application delay logs");
//           receiverDelayLog.erase (packetNumToRemove);
//           transmitterDelayLog.erase (packetNumToRemove);
//         }
//     }
//   Simulator::Schedule (Time ("500ms"), &AppContainer::CompressDelayLog, this);
// }
