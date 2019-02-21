#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/core-module.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/inet-socket-address.h"

#include "receiver-app.h"
#include "transmitter-app.h"
#include "application-helper.h"
#include "single-path-receiver.h"
#include "single-path-transmitter.h"

using namespace ns3;
using namespace tinyxml2;

NS_LOG_COMPONENT_DEFINE ("ApplicationHelper");


void ApplicationHelper::InstallApplicationsOnTerminals(const Flow::FlowContainer &flows,
                                                       const Terminal::TerminalContainer &terminals,
                                                       bool usePpfsSwitches)
{
    for (const auto& flowPair : flows) {
        NS_LOG_INFO("Installing flow " << flowPair.first);
        const auto& flow {flowPair.second};

        if (flow.GetDataPaths().size() == 1 || usePpfsSwitches) {
            // Install the single path transmitter
            Ptr<SinglePathTransmitterApp> singlePathTransmitter = CreateObject<SinglePathTransmitterApp>(flow);
            flow.srcNode->GetNode()->AddApplication(singlePathTransmitter);
            singlePathTransmitter->SetStartTime(Seconds(0.0));
            m_transmitterApplications.emplace(flow.id, singlePathTransmitter);

            // Install the single path receiver
            Ptr<SinglePathReceiver> singlePathReceiverApp = CreateObject<SinglePathReceiver>(flow);
            flow.dstNode->GetNode()->AddApplication(singlePathReceiverApp);
            singlePathReceiverApp->SetStartTime(Seconds(0.0));
            m_receiverApplications.emplace(flow.id, singlePathReceiverApp);
        } else {
            // Install the multipath transmitter
            Ptr<TransmitterApp> transmitterApp = CreateObject<TransmitterApp>(flow);
            flow.srcNode->GetNode()->AddApplication(transmitterApp);
            transmitterApp->SetStartTime(Seconds(0.0));
            m_transmitterApplications.emplace(flow.id, transmitterApp);

            // Install the multipath receiver
            Ptr<ReceiverApp> receiverApp = CreateObject<ReceiverApp>(flow);
            flow.dstNode->GetNode()->AddApplication(receiverApp);
            receiverApp->SetStartTime(Seconds(0.0));
            m_receiverApplications.emplace(flow.id, receiverApp);
        }
    }
}

const ApplicationHelper::applicationContainer_t& ApplicationHelper::GetTransmitterApps() const {
    return m_transmitterApplications;
}

const ApplicationHelper::applicationContainer_t& ApplicationHelper::GetReceiverApps() const {
    return m_receiverApplications;
}
