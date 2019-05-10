#ifndef flow_monitor_h
#define flow_monitor_h

#include "ns3/flow-monitor-helper.h"

#include "flow.h"

void SaveFlowMonitorResultFile (ns3::FlowMonitorHelper &flowMonHelper,
                                const Flow::flowContainer_t &flows,
                                const std::string &flowMonitorOutputLocation);

#endif /* flow_monitor_h */
