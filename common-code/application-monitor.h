#ifndef APPLICATION_MONITOR_H
#define APPLICATION_MONITOR_H

#include <cstdint>
#include <map>

#include <ns3/nstime.h>
#include <ns3/application.h>
#include <ns3/ptr.h>
#include <ns3/packet.h>
#include <ns3/address.h>

#include "definitions.h"

class ApplicationMonitor
{
public:
  ApplicationMonitor();
  ~ApplicationMonitor();

  void MonitorApplication (FlowId_t flowId, ns3::Ptr<ns3::Application> application);

private:

  void ReceivePacket (std::string context,
                      ns3::Ptr<const ns3::Packet> packet,
                      const ns3::Address &address);

  struct FlowDetails
  {
    FlowDetails (FlowId_t flowId) : flowId (flowId), nBytesReceived (0),
      requestedGoodput (0.0), goodputAtQuota (0.0)
    {}
    // TODO: Check if this is redundant.
    FlowId_t  flowId; //!< The flow Id
    uint64_t  nBytesReceived; //!< Number of bytes received by the application
    double    requestedGoodput; //!< The flow's original requested goodput
    double    goodputAtQuota; //!< The goodput when the flow has received enough bytes to meet the quota
    ns3::Time firstRxPacket; //!< The time the first packet was received
    ns3::Time lastRxPacket; //!< The time the last packet was received
  };

  /**
   * A map containing all the flow details.
   * Key: Flow Id
   * Value: FlowDetails structure
   */
  std::map<FlowId_t, FlowDetails> m_flows;

  uint32_t nFlows; //!< The number of flows that are being monitored
};

#endif /* APPLICATION_MONITOR_H */
