#include <ns3/core-module.h>

#include "application-monitor.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ApplicationMonitor");


ApplicationMonitor::ApplicationMonitor() : nFlows (0)
{}

ApplicationMonitor::~ApplicationMonitor()
{}

void
ApplicationMonitor::MonitorApplication (FlowId_t flowId, ns3::Ptr<ns3::Application> application)
{
  // Call the function ReceivePacket whenever a packet is received
  application->TraceConnect ("Rx", std::to_string (flowId),
                             MakeCallback (&ApplicationMonitor::ReceivePacket, this));

  // Create the necessary flow structure in the dictionary
  FlowDetails flow (flowId);
  auto ret = m_flows.insert ({flowId, flow});

  if (ret.second == false)
    NS_LOG_ERROR ("Duplicate Flow ID: " << flowId);
  else
    NS_LOG_INFO ("Monitoring Flow " << flowId);

  // Increment the number of flows being monitored
  nFlows++;
}