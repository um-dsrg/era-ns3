#include "ns3/log.h"

#include "routing-helper.h"
#include "device/switch/switch-base.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RoutingHelper");

void InstallDataPaths (const Flow &flow, const std::map<id_t, Ptr<NetDevice>> &transmitOnLink);
void InstallAckPaths (const Flow &flow, const std::map<id_t, Ptr<NetDevice>> &transmitOnLink);

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

      /* Installing data paths */
      InstallDataPaths (flow, transmitOnLink);

      /* Installing Ack paths for TCP flows only */
      InstallAckPaths (flow, transmitOnLink);
    }
}

void
InstallDataPaths (const Flow &flow, const std::map<id_t, Ptr<NetDevice>> &transmitOnLink)
{
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
}

void
InstallAckPaths (const Flow &flow, const std::map<id_t, Ptr<NetDevice>> &transmitOnLink)
{
  if (flow.protocol == FlowProtocol::Tcp) // Only install ACK paths for TCP flows
    {
      for (const auto &path : flow.GetAckPaths ())
        {
          NS_LOG_INFO ("Installing routing for Ack Path " << path.id);
          NS_LOG_INFO ("Path details: " << path);

          //for (const auto &link : path.GetLinks ())
          //  {
          //    NS_LOG_INFO ("Working on Link " << link->id);

          //    if (link->srcNodeType == NodeType::Switch)
          //      {
          //        auto forwardingPort = transmitOnLink.at (link->id);
          //        auto *switchNode = dynamic_cast<SwitchBase *> (link->srcNode);

          //        /**
          //               The addresses are reversed becuase the ACK flow has the opposite source
          //               and destination details of the data flow.
          //               */
          //        switchNode->AddEntryToRoutingTable (
          //            flow.dstNode->GetIpAddress ().Get (), flow.srcNode->GetIpAddress ().Get (),
          //            path.srcPort, path.dstPort, flow.protocol, forwardingPort,
          //            1.0 /* Ack flows are not split*/);
          //      }
          //  }
        }
    }
}
