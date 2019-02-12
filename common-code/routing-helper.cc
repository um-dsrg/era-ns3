#include "routing-helper.h"

NS_LOG_COMPONENT_DEFINE("RoutingHelper");

template <>
void RoutingHelper<PpfsSwitch>::BuildRoutingTable (const std::map<id_t, Flow> &flows,
                                                   const std::map<id_t, Ptr<NetDevice>> &transmitOnLink) {
    NS_LOG_INFO("Building the Routing Table for the PPFS Switch");

    for(const auto &flowPair : flows) {

        const auto &flow{flowPair.second};
        NS_LOG_INFO ("Installing routing for Flow " << flow.id);

        auto flowDataRate = double{static_cast<double>(flow.dataRate.GetBitRate())};

        /* Installing data paths */
        for (const auto &path : flow.GetDataPaths()) {
            auto pathSplitRatio = double{path.dataRate.GetBitRate() / flowDataRate};
            NS_LOG_INFO ("Installing routing for Data Path " << path.id << ". " <<
                         "Split Ratio: " << pathSplitRatio);

            for (const auto &link : path.GetLinks ()) {
                NS_LOG_INFO ("Working on Link " << link->id);

                if (link->srcNodeType == NodeType::Switch) {
                    auto forwardingPort = transmitOnLink.at (link->id);
                    PpfsSwitch* switchNode = dynamic_cast<PpfsSwitch*>(link->srcNode);
                    switchNode->AddEntryToRoutingTable(flow.srcNode->GetIpAddress().Get(),
                                                       flow.dstNode->GetIpAddress().Get(),
                                                       path.srcPort, path.dstPort, flow.protocol,
                                                       pathSplitRatio, forwardingPort);
                }
            }
        }
    }
}
