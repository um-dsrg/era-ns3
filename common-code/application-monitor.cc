#include <ns3/core-module.h>

#include "application-monitor.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ApplicationMonitor");


ApplicationMonitor::ApplicationMonitor (uint64_t nBytesQuota, bool logGoodputEveryPacket) :
  m_nFlows (0), m_nBytesQuota (nBytesQuota), m_logGoodputEveryPacket (logGoodputEveryPacket)
{}

ApplicationMonitor::~ApplicationMonitor()
{}

void
ApplicationMonitor::MonitorApplication (FlowId_t flowId,
                                        double requestedGoodput,
                                        ns3::Ptr<ns3::Application> application)
{
  // Call the function ReceivePacket whenever a packet is received
  application->TraceConnect ("Rx", std::to_string (flowId),
                             MakeCallback (&ApplicationMonitor::ReceivePacket, this));

  // Create the necessary flow structure and insert it in the flows map
  FlowDetails flow (requestedGoodput);
  auto ret = m_flows.insert ({flowId, flow});

  if (ret.second == false)
    NS_ABORT_MSG ("Trying to insert duplicate Flow ID: " << flowId);
  else
    NS_LOG_INFO ("Monitoring Flow " << flowId);

  // Increment the number of flows being monitored
  m_nFlows++;
}

void
ApplicationMonitor::ReceivePacket (std::string context, ns3::Ptr<const ns3::Packet> packet,
                                   const ns3::Address &address)
{
  NS_LOG_INFO ("Flow " << context << " received a packet at " << Simulator::Now());

  FlowId_t flowId (std::stoi (context));

  try
    {
      FlowDetails& flow = m_flows.at (flowId);

      if (flow.nBytesReceived == 0) // The first packet is received
        flow.firstRxPacket = Simulator::Now();

      flow.lastRxPacket = Simulator::Now();
      flow.nBytesReceived += packet->GetSize();
      flow.nPacketsReceived++;

      if (m_logGoodputEveryPacket) // Log the flow's goodput for every packet recieved
        flow.goodputPerPacket.push_back (std::make_pair (flow.nBytesReceived, flow.CalculateGoodput()));

      if (flow.nBytesReceived >= m_nBytesQuota) // The flow has met the quota
        {
          // Insert the flow id into the set. Note: The set does not allow duplicates
          // which is why this data structure is being used.
          m_flowsThatMetQuota.insert (flowId);
          flow.goodputAtQuota = flow.CalculateGoodput();
        }

      NS_LOG_INFO (flow);

      // Stop the simulation if all of the applications have received a sufficient amount
      // of bytes.
      if (m_flowsThatMetQuota.size() == m_nFlows)
        {
          NS_LOG_INFO ("Simulation STOPPED. "
                       "Quota of " << m_nBytesQuota << "bytes met by all the applications.");
          Simulator::Stop();
        }
    }
  catch (std::out_of_range)
    {
      NS_ABORT_MSG ("Flow with Id " << flowId << " not found in the ApplicationMonitor map");
    }
}
