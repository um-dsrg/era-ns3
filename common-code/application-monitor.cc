#include <ns3/core-module.h>

#include "application-monitor.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ApplicationMonitor");


ApplicationMonitor::ApplicationMonitor() : nFlows (0)
{}

ApplicationMonitor::~ApplicationMonitor()
{}

void
ApplicationMonitor::MonitorApplication (FlowId_t flowId,
                                        ns3::Ptr<ns3::Application> application)
{
  // Call the function ReceivePacket whenever a packet is received
  application->TraceConnect ("Rx", std::to_string (flowId),
                             MakeCallback (&ApplicationMonitor::ReceivePacket, this));

  // Create the necessary flow structure and insert it in the flows dictionary
  FlowDetails flow (flowId);
  auto ret = m_flows.insert ({flowId, flow});

  if (ret.second == false)
    NS_ABORT_MSG ("Trying to insert duplicate Flow ID: " << flowId);
  else
    NS_LOG_INFO ("Monitoring Flow " << flowId);

  // Increment the number of flows being monitored
  nFlows++;
}

void
ApplicationMonitor::ReceivePacket (std::string context, ns3::Ptr<const ns3::Packet> packet,
                                   const ns3::Address &address)
{
  std::cout << "Packet received by flow " << context << std::endl;
}
