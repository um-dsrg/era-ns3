#ifndef routing_helper_h
#define routing_helper_h

#include <map>
#include <tinyxml2.h>

#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"

#include "flow.h"
#include "sdn-switch.h"
#include "definitions.h"
#include "ppfs-switch.h"
#include "topology-builder.h"

/**
 * \brief      Routing helper base class.
 *
 * This base class is created to allow the creation of the RoutingHelper class at runtime based on the simulation switch type.
 */
class RoutingHelperBase {
public:
    virtual void BuildRoutingTable (const std::map<id_t, Flow> &flows, const std::map<id_t, Ptr<NetDevice>> &transmitOnLink) = 0;
    virtual ~RoutingHelperBase() {}
};

/**
 RoutingHelper Class Declaration
 */

template <class SwitchType>
class RoutingHelper : public RoutingHelperBase {
    NS_LOG_TEMPLATE_DECLARE; /**< Logging component. */

public:
    RoutingHelper ();
    void BuildRoutingTable (const std::map<id_t, Flow> &flows,
                            const std::map<id_t, Ptr<NetDevice>> &transmitOnLink);
};

/**
 RoutingHelper Class Implementation
 */

template <class SwitchType>
RoutingHelper<SwitchType>::RoutingHelper () : NS_LOG_TEMPLATE_DEFINE ("RoutingHelper") {
}
#endif /* routing_helper_h */
