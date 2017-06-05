#ifndef APPLICATION_HELPER_H
#define APPLICATION_HELPER_H

#include <tinyxml2.h>
#include "ns3/node-container.h"
#include "ns3/ipv4.h"

#include "definitions.h"

class ApplicationHelper
{
public:
  uint32_t InstallApplicationOnTerminals (ns3::NodeContainer& allNodes,
                                          tinyxml2::XMLNode* rootNode);
private:
  inline uint32_t CalculateHeaderSize (char protocol);
  inline ns3::Ipv4Address GetIpAddress (NodeId_t nodeId, ns3::NodeContainer& nodes);
};

#endif /* APPLICATION_HELPER_H */
