#ifndef ROUTING_HELPER_H
#define ROUTING_HELPER_H

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
class RoutingHelper
{
public:
  RoutingHelper ();
  void BuildRoutingTable (const std::map<id_t, Flow> &flows,
                          const std::map<id_t, Ptr<NetDevice>> &transmitOnLink);

private:
  NS_LOG_TEMPLATE_DECLARE; //!< Logging
  // private:
  // void ParseIncomingFlows (std::map<std::pair<NodeId_t, FlowId_t>, double> &incomingFlow,
  //                          tinyxml2::XMLNode *rootNode);
  // inline uint32_t GetIpAddress (NodeId_t nodeId, ns3::NodeContainer &nodes);

  // std::map<NodeId_t, SwitchType> &m_switchMap;
};

template <class SwitchType>
RoutingHelper<SwitchType>::RoutingHelper () : NS_LOG_TEMPLATE_DEFINE ("RoutingHelper")
{
}

template <class SwitchType>
void
RoutingHelper<SwitchType>::BuildRoutingTable (const std::map<id_t, Flow> &flows,
                                              const std::map<id_t, Ptr<NetDevice>> &transmitOnLink)
{
  for (const auto &flowPair : flows)
    {
      const auto &flow{flowPair.second};
      NS_LOG_INFO ("Installing routing for Flow " << flow.id);

      for (const auto &path : flow.GetPaths ())
        {
          NS_LOG_INFO ("Installing routing for Path " << path.id);
          for (const auto &link : path.GetLinks ())
            {
              NS_LOG_INFO ("Working on Link " << link->id);
              if (link->srcNodeType == NodeType::Switch)
                {
                  auto forwardingPort = transmitOnLink.at (link->id);
                  SwitchType *switchNode = dynamic_cast<SwitchType *> (link->srcNode);
                  switchNode->AddEntryToRoutingTable (flow.srcAddress.Get (),
                                                      flow.dstAddress.Get (), path.srcPort,
                                                      path.dstPort, flow.protocol, forwardingPort);
                }
            }
        }
    }
}

#endif /* ROUTING_HELPER_H */
