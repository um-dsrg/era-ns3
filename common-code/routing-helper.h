#ifndef routing_helper_h
#define routing_helper_h

#include <map>
#include <tinyxml2.h>

#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/node-container.h"

#include "definitions.h"
#include "topology-builder.h"
#include "flow.h"

template <class SwitchType>
class RoutingHelper {
public:
    RoutingHelper ();
    void BuildRoutingTable (const std::map<id_t, Flow> &flows,
                            const std::map<id_t, Ptr<NetDevice>> &transmitOnLink);

private:
    NS_LOG_TEMPLATE_DECLARE; /**< Logging component. */
};

template <class SwitchType>
RoutingHelper<SwitchType>::RoutingHelper () : NS_LOG_TEMPLATE_DEFINE ("RoutingHelper") {
}

template <class SwitchType>
void RoutingHelper<SwitchType>::BuildRoutingTable (const std::map<id_t, Flow> &flows,
                                                   const std::map<id_t, Ptr<NetDevice>> &transmitOnLink) {
    for(const auto &flowPair : flows) {

        const auto &flow{flowPair.second};
        NS_LOG_INFO ("Installing routing for Flow " << flow.id);

        /* Installing data paths */
        for (const auto &path : flow.GetDataPaths()) {
            NS_LOG_INFO ("Installing routing for Data Path " << path.id);

            for (const auto &link : path.GetLinks ()) {
                NS_LOG_INFO ("Working on Link " << link->id);

                if (link->srcNodeType == NodeType::Switch) {
                    auto forwardingPort = transmitOnLink.at (link->id);
                    SwitchType *switchNode = dynamic_cast<SwitchType *> (link->srcNode);
                    switchNode->AddEntryToRoutingTable (flow.srcNode->GetIpAddress().Get(),
                                                        flow.dstNode->GetIpAddress().Get(),
                                                        path.srcPort, path.dstPort, flow.protocol,
                                                        forwardingPort);
                }
            }
        }

        /* Installing ack paths */
        for (const auto &path : flow.GetAckPaths()) {
            NS_LOG_INFO ("Installing routing for Ack Path " << path.id);

            for (const auto &link : path.GetLinks ()) {
                NS_LOG_INFO ("Working on Link " << link->id);

                if (link->srcNodeType == NodeType::Switch) {
                    auto forwardingPort = transmitOnLink.at (link->id);
                    SwitchType *switchNode = dynamic_cast<SwitchType *> (link->srcNode);

                    /* The addresses are reversed becuase the ACK flow has the opposite source and destination
                       details of the data flow. */
                    switchNode->AddEntryToRoutingTable (flow.dstNode->GetIpAddress().Get(),
                                                        flow.srcNode->GetIpAddress().Get(),
                                                        path.srcPort, path.dstPort, flow.protocol,
                                                        forwardingPort);
                }
            }
        }
    }
}

#endif /* routing_helper_h */
