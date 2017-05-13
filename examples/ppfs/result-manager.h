#ifndef RESULT_MANAGER_H
#define RESULT_MANAGER_H

#include <memory>
#include <tinyxml2.h>

#include "ns3/flow-monitor-helper.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"

#include "ppfs-switch.h"

class ResultManager
{
public:
  ResultManager ();

  void SetupFlowMonitor (ns3::NodeContainer& allNodes, uint32_t stopTime);
  void TraceTerminalTransmissions (ns3::NetDeviceContainer& terminalDevices,
                                   std::map <ns3::Ptr<ns3::NetDevice>, uint32_t>& terminalToLinkId);
  void GenerateFlowMonitorXmlLog ();
  void UpdateFlowIds (tinyxml2::XMLNode* logFileRootNode, ns3::NodeContainer& allNodes);
  void AddQueueStatistics (std::map<uint32_t, PpfsSwitch>& switchMap);
  void SaveXmlResultFile (const char* resultPath);

private:
  // Function will be called every time a node transmits a packet
  void TerminalPacketTransmission (std::string context, ns3::Ptr<const ns3::Packet> packet);
  void InsertTimeStamp (tinyxml2::XMLNode* node);
  inline uint32_t GetIpAddress (uint32_t nodeId, ns3::NodeContainer& allNodes);
  inline tinyxml2::XMLNode* GetRootNode ();

  ns3::Ptr<ns3::FlowMonitor> m_flowMonitor;
  ns3::FlowMonitorHelper m_flowMonHelper;
  std::unique_ptr<tinyxml2::XMLDocument> m_xmlResultFile;
};

#endif /* RESULT_MANAGER_H */
