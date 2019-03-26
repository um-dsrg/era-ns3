#include "ns3/log.h"

#include "routing-helper.h"
#include "device/switch/switch-base.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RoutingHelper");

void
BuildRoutingTable (const std::map<id_t, Flow> &flows,
                   const std::map<id_t, Ptr<NetDevice>> &transmitOnLink)
{
  NS_LOG_INFO ("Building the Routing Table");

  for (const auto &flowPair : flows)
    {
      const auto &flow{flowPair.second};
      if (flow.dataRate == 0)
        {
          NS_LOG_INFO ("Flow " << flow.id
                               << " is not being installed in the routing tables because it is "
                                  "allocated no data rate");
          continue;
        }

      NS_LOG_INFO ("Installing routing for Flow " << flow.id);

      // TODO move this to a function
      /* Installing data paths */
      auto flowDataRate = double{static_cast<double> (flow.dataRate.GetBitRate ())};
      for (const auto &path : flow.GetDataPaths ())
        {
          auto pathSplitRatio = double{path.dataRate.GetBitRate () / flowDataRate};
          NS_LOG_INFO ("Installing routing for Data Path " << path.id << ". "
                                                           << "Split Ratio: " << pathSplitRatio);

          for (const auto &link : path.GetLinks ())
            {
              NS_LOG_INFO ("Working on Link " << link->id);

              if (link->srcNodeType == NodeType::Switch)
                {
                  auto forwardingPort = transmitOnLink.at (link->id);
                  auto *switchNode = dynamic_cast<SwitchBase *> (link->srcNode);
                  switchNode->AddEntryToRoutingTable (
                      flow.srcNode->GetIpAddress ().Get (), flow.dstNode->GetIpAddress ().Get (),
                      path.srcPort, path.dstPort, flow.protocol, forwardingPort, pathSplitRatio);
                }
            }
        }

      /* Installing the ACK paths for TCP flows only */
      // TODO Update this to be handled equally well for PPFS and TCP!
      // Currently it only works for SDN and move this to a function
      if (flow.protocol == FlowProtocol::Tcp)
        {
          for (const auto &path : flow.GetAckPaths ())
            {
              NS_LOG_INFO ("Installing routing for Ack Path " << path.id);

              for (const auto &link : path.GetLinks ())
                {
                  NS_LOG_INFO ("Working on Link " << link->id);

                  if (link->srcNodeType == NodeType::Switch)
                    {
                      auto forwardingPort = transmitOnLink.at (link->id);
                      auto *switchNode = dynamic_cast<SwitchBase *> (link->srcNode);

                      /**
                         The addresses are reversed becuase the ACK flow has the opposite source
                         and destination details of the data flow.
                         */
                      switchNode->AddEntryToRoutingTable (
                          flow.dstNode->GetIpAddress ().Get (),
                          flow.srcNode->GetIpAddress ().Get (), path.srcPort, path.dstPort,
                          flow.protocol, forwardingPort, 1.0 /* Ack flows are not split*/);
                    }
                }
            }
        }
    }
}

/*template <>*/
/*void RoutingHelper<SdnSwitch>::BuildRoutingTable (const std::map<id_t, Flow> &flows,*/
/*                                                  const std::map<id_t, Ptr<NetDevice>> &transmitOnLink) {*/
/*    NS_LOG_INFO("Building the Routing Table for the SDN Switches");*/

/*    for(const auto &flowPair : flows) {*/

/*        const auto &flow{flowPair.second};*/
/*        if (flow.dataRate == 0) {*/
/*            NS_LOG_INFO ("Flow " << flow.id << " is not being installed in the routing tables because it was allocated nothing");*/
/*            continue;*/
/*        }*/

/*        NS_LOG_INFO ("Installing routing for Flow " << flow.id);*/

/* Installing data paths */
/*        for (const auto &path : flow.GetDataPaths()) {*/
/*            NS_LOG_INFO ("Installing routing for Data Path " << path.id);*/

/*            for (const auto &link : path.GetLinks ()) {*/
/*                NS_LOG_INFO ("Working on Link " << link->id);*/

/*                if (link->srcNodeType == NodeType::Switch) {*/
/*                    auto forwardingPort = transmitOnLink.at (link->id);*/
/*                    SdnSwitch *switchNode = dynamic_cast<SdnSwitch*> (link->srcNode);*/
/*                    switchNode->AddEntryToRoutingTable (flow.srcNode->GetIpAddress().Get(),*/
/*                                                        flow.dstNode->GetIpAddress().Get(),*/
/*                                                        path.srcPort, path.dstPort, flow.protocol,*/
/*                                                        forwardingPort);*/
/*                }*/
/*            }*/
/*        }*/

/*        if (flow.protocol == FlowProtocol::Tcp) { // Install ACK paths only for TCP flows*/
/*            for (const auto &path : flow.GetAckPaths()) {*/
/*                NS_LOG_INFO ("Installing routing for Ack Path " << path.id);*/

/*                for (const auto &link : path.GetLinks ()) {*/
/*                    NS_LOG_INFO ("Working on Link " << link->id);*/

/*                    if (link->srcNodeType == NodeType::Switch) {*/
/*                        auto forwardingPort = transmitOnLink.at (link->id);*/
/*                        SdnSwitch *switchNode = dynamic_cast<SdnSwitch*> (link->srcNode);*/

/***/
/*                         The addresses are reversed becuase the ACK flow has the opposite source and destination*/
/*                         details of the data flow.*/
/*                         */
/*                        switchNode->AddEntryToRoutingTable (flow.dstNode->GetIpAddress().Get(),*/
/*                                                            flow.srcNode->GetIpAddress().Get(),*/
/*                                                            path.srcPort, path.dstPort, flow.protocol,*/
/*                                                            forwardingPort);*/
/*                    }*/
/*                }*/
/*            }*/
/*        }*/
/*    }*/
/*}*/

/*template <>*/
/*void RoutingHelper<PpfsSwitch>::BuildRoutingTable (const std::map<id_t, Flow> &flows,*/
/*                                                   const std::map<id_t, Ptr<NetDevice>> &transmitOnLink) {*/
/*    NS_LOG_INFO("Building the Routing Table for the PPFS Switches");*/

/*    for(const auto &flowPair : flows) {*/

/*        const auto &flow{flowPair.second};*/
/*        if (flow.dataRate == 0) {*/
/*            NS_LOG_INFO ("Flow " << flow.id << " is not being installed in the routing tables because it was allocated nothing");*/
/*            continue;*/
/*        }*/
/*        NS_LOG_INFO ("Installing routing for Flow " << flow.id);*/

/*        auto flowDataRate = double{static_cast<double>(flow.dataRate.GetBitRate())};*/

/* Installing data paths */
/*        for (const auto &path : flow.GetDataPaths()) {*/
/*            auto pathSplitRatio = double{path.dataRate.GetBitRate() / flowDataRate};*/
/*            NS_LOG_INFO ("Installing routing for Data Path " << path.id << ". " <<*/
/*                         "Split Ratio: " << pathSplitRatio);*/

/*            for (const auto &link : path.GetLinks ()) {*/
/*                NS_LOG_INFO ("Working on Link " << link->id);*/

/*                if (link->srcNodeType == NodeType::Switch) {*/
/*                    auto forwardingPort = transmitOnLink.at (link->id);*/
/*                    PpfsSwitch* switchNode = dynamic_cast<PpfsSwitch*>(link->srcNode);*/
/*                    switchNode->AddEntryToRoutingTable(flow.srcNode->GetIpAddress().Get(),*/
/*                                                       flow.dstNode->GetIpAddress().Get(),*/
/*                                                       path.srcPort, path.dstPort, flow.protocol,*/
/*                                                       pathSplitRatio, forwardingPort);*/
/*                }*/
/*            }*/
/*        }*/

/*        if (flow.protocol == FlowProtocol::Tcp) { // Install ACK paths only for TCP flows*/
/*            const auto& ackShortestPath = flow.GetAckShortestPath();*/
/*            NS_LOG_INFO ("Installing routing for the Ack Shortest Path");*/

/*            for (const auto &link : ackShortestPath.GetLinks ()) {*/
/*                NS_LOG_INFO ("Working on Link " << link->id);*/

/*                if (link->srcNodeType == NodeType::Switch) {*/
/*                    auto forwardingPort = transmitOnLink.at (link->id);*/
/*                    PpfsSwitch* switchNode = dynamic_cast<PpfsSwitch*>(link->srcNode);*/

/***/
/*                     The addresses are reversed becuase the ACK flow has the opposite source and destination*/
/*                     details of the data flow.*/
/*                     */
/*                    switchNode->AddEntryToRoutingTable(flow.dstNode->GetIpAddress().Get(),*/
/*                                                       flow.srcNode->GetIpAddress().Get(),*/
/*                                                       ackShortestPath.srcPort,*/
/*                                                       ackShortestPath.dstPort,*/
/*                                                       flow.protocol,*/
/*                                                       1.0, Ack flows are not split */
/*                                                       forwardingPort);*/
/*                }*/
/*            }*/
/*        }*/
/*    }*/
/*}*/
