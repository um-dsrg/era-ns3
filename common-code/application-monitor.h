#ifndef APPLICATION_MONITOR_H
#define APPLICATION_MONITOR_H

#include <cstdint>
#include <map>
#include <set>

#include <ns3/nstime.h>
#include <ns3/application.h>
#include <ns3/ptr.h>
#include <ns3/packet.h>
#include <ns3/address.h>

#include "definitions.h"

class ApplicationMonitor
{
public:
  ApplicationMonitor (uint64_t nBytesQuota);
  ~ApplicationMonitor();

  void MonitorApplication (FlowId_t flowId, double requestedGoodput,
                           ns3::Ptr<ns3::Application> application);

private:

  void ReceivePacket (std::string context,
                      ns3::Ptr<const ns3::Packet> packet,
                      const ns3::Address &address);

  struct FlowDetails
  {
    FlowDetails (double requestedGoodput) : nBytesReceived (0),
      nPacketsReceived (0), requestedGoodput (requestedGoodput),
      goodputAtQuota (0.0)
    {}

    friend std::ostream& operator<< (std::ostream& output, FlowDetails& flow)
    {
      output << "  Received Bytes: " << flow.nBytesReceived << "\n"
             << "  Received Packets: " << flow.nPacketsReceived << "\n"
             << "  Requested Goodput (Mbps): " << flow.requestedGoodput << "\n"
             << "  Goodput at Quota (Mbps):  " << flow.goodputAtQuota << "\n"
             << "  Time First Rx (s): " << flow.firstRxPacket.GetSeconds() << "\n"
             << "  Time Last Rx (s):  " << flow.lastRxPacket.GetSeconds() << "\n";
      return output;
    }

    uint64_t  nBytesReceived; //!< Number of bytes received by the application
    uint64_t  nPacketsReceived; //!< Number of packets received by the application
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

  uint32_t m_nFlows; //!< The number of flows that are being monitored
  /**
   * The number of bytes each flow must receive before the simulation can
   * terminate.
   */
  uint64_t m_nBytesQuota;
  /**
   * A set of Flow Ids that have met the quota. A set is used because
   * duplicate values are not allowed.
   */
  std::set<FlowId_t> m_flowsThatMetQuota;
};

#endif /* APPLICATION_MONITOR_H */
