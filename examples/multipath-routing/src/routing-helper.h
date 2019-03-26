#ifndef routing_helper_h
#define routing_helper_h

#include "ns3/net-device.h"

#include "flow.h"

void BuildRoutingTable (const std::map<id_t, Flow> &flows,
                        const std::map<id_t, ns3::Ptr<ns3::NetDevice>> &transmitOnLink);
#endif /* routing_helper_h */
