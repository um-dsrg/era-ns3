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
#include "device/custom-device.h"
#include "device/terminal/terminal.h"
#include "device/switch/switch-container.h"

using namespace ns3;
using namespace tinyxml2;

/* template <class SwitchType> */
class TopologyBuilder
{
public:
  TopologyBuilder (SwitchType switchType);

  // TODO REMOVE THE BELOW functions
  /* const Terminal::TerminalContainer &GetTerminals (); */
  /* const std::map<id_t, SwitchType> &GetSwitches (); */
  // todo Remove this function from this class and create a switch container
  /* tinyxml2::XMLElement *GetSwitchQueueLoggingElement (tinyxml2::XMLDocument &xmlDocument); */

  void CreateNodes (XMLNode *rootNode);
  std::map<id_t, Ptr<NetDevice>> BuildNetworkTopology (XMLNode *rootNode); // DONE
  void AssignIpToTerminals (); // DONE
  Flow::flowContainer_t ParseFlows (XMLNode *rootNode); // DONE
  void EnablePacketReceptionOnSwitches (); // DONE
  void ReconcileRoutingTables (); // DONE

private:
  /**< Key: Path ID | Value: source port, destination port */
  using pathPortMap_t = std::map<id_t, std::pair<portNum_t, portNum_t>>;

  void CreateUniqueNode (id_t nodeId, NodeType nodeType); // DONE
  void InstallP2pLinks (const std::vector<Link> &links,
                        std::map<id_t, Ptr<NetDevice>> &transmitOnLink); // DONE
  CustomDevice *GetNode (id_t id, NodeType nodeType); // DONE

  pathPortMap_t AddDataPaths (Flow &flow, XMLElement *flowElement); // DONE
  void AddAckPaths (Flow &flow, XMLElement *flowElement, const pathPortMap_t &pathPortMap);

  Terminal::TerminalContainer m_terminals; /**< Maps the node id with the Terminal object. */
  SwitchType m_switchType; /**< The type of switches to use */
  SwitchContainer m_switchContainer;
  std::map<id_t, Link> m_links; /**< Maps the link id with the Link object. */
};

void ShuffleLinkElements (std::vector<XMLElement *> &linkElements);

#endif /* topology_builder_h */
