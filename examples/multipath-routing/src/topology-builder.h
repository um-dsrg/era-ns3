#ifndef topology_builder_h
#define topology_builder_h

#include <map>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <tinyxml2.h>

#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/object-factory.h"
#include "ns3/node-container.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/net-device-container.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-net-device.h"

#include "flow.h"
#include "definitions.h"
#include "results-container.h"
#include "device/custom-device.h"
#include "device/terminal/terminal.h"
#include "device/switch/switch-container.h"

using namespace ns3;
using namespace tinyxml2;

class TopologyBuilder
{
public:
  TopologyBuilder (SwitchType switchType, SwitchContainer &switchContainer,
                   Terminal::terminalContainer_t &terminalContainer, ResultsContainer &resContainer,
                   const std::string &txQueueRetrievalMethod);

  void AssignIpToTerminals ();
  void CreateNodes (XMLNode *rootNode);
  std::map<id_t, Ptr<NetDevice>> BuildNetworkTopology (XMLNode *rootNode,
                                                       Link::linkContainer_t &linkContainer);

private:
  using pathPortMap_t = std::map<id_t, std::pair<portNum_t, portNum_t>>;

  void CreateUniqueNode (id_t nodeId, NodeType nodeType);
  void InstallP2pLinks (const std::vector<const Link *> &links,
                        std::map<id_t, Ptr<NetDevice>> &transmitOnLink);
  CustomDevice *GetNode (id_t id, NodeType nodeType);

  SwitchType m_switchType; // The type of switches to use
  SwitchContainer &m_switchContainer;
  Terminal::terminalContainer_t &m_terminalContainer;
  ResultsContainer &m_resContainer;
  const std::string m_txQueueRetrievalMethod;
};

void ShuffleLinkElements (std::vector<XMLElement *> &linkElements);

#endif /* topology_builder_h */
