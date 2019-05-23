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
AppContainer::InstallApplicationsOnTerminals (const Flow::flowContainer_t &flows,
                                              bool usePpfsSwitches, ResultsContainer &resContainer)
{
  for (const auto &flowPair : flows)
    {
      NS_LOG_INFO ("Installing flow " << flowPair.first);
      const auto &flow{flowPair.second};

      if (flow.dataRate.GetBitRate() > 0)
        {
          if (flow.GetDataPaths ().size () == 1 || usePpfsSwitches)
            {
              InstallApplicationOnTerminal<UnipathTransmitter, UnipathReceiver> (flow, resContainer);
            }
          else
            {
              InstallApplicationOnTerminal<MultipathTransmitter, MultipathReceiver> (flow,
                                                                                     resContainer);
            }
        }
    }
}

template <typename TransmitterApp, typename ReceiverApp>
void
AppContainer::InstallApplicationOnTerminal (const Flow &flow, ResultsContainer &resContainer)
{
  // Installing the transmitter
  auto transmitterApp = std::make_unique<TransmitterApp> (flow, resContainer);
  Ptr<Application> nsTransmitterApp = transmitterApp.get ();
  flow.srcNode->GetNode ()->AddApplication (nsTransmitterApp);
  transmitterApp->SetStartTime (Seconds (0.0));
  m_transmitterApplications.emplace (flow.id, std::move (transmitterApp));

  // Installing the receiver
  auto receiverApp = std::make_unique<ReceiverApp> (flow, resContainer);
  Ptr<Application> nsReceiverApp = receiverApp.get ();
  flow.dstNode->GetNode ()->AddApplication (nsReceiverApp);
  receiverApp->SetStartTime (Seconds (0.0));
  m_receiverApplications.emplace (flow.id, std::move (receiverApp));
}
