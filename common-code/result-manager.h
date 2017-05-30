#ifndef RESULT_MANAGER_H
#define RESULT_MANAGER_H

#include <memory>
#include <map>
#include <tinyxml2.h>

#include "ns3/flow-monitor-helper.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"

#include "examples/ppfs/ppfs-switch.h"

class ResultManager
{
public:
  ResultManager ();

  struct LinkStatistic
  {
    ns3::Time timeFirstTx;
    ns3::Time timeLastTx;
    uint32_t packetsTransmitted;
    uint32_t bytesTransmitted;

    LinkStatistic () : timeFirstTx (0), timeLastTx (0), packetsTransmitted (0), bytesTransmitted (0)
    {}
  };

  void SetupFlowMonitor (ns3::NodeContainer& allNodes, uint32_t stopTime);
  void TraceTerminalTransmissions (ns3::NetDeviceContainer& terminalDevices,
                                   std::map <ns3::Ptr<ns3::NetDevice>, LinkId_t>& terminalToLinkId);
  void GenerateFlowMonitorXmlLog (bool enableHistograms, bool enableFlowProbes);
  void UpdateFlowIds (tinyxml2::XMLNode* logFileRootNode, ns3::NodeContainer& allNodes);
  template <typename SwitchType>
  void AddLinkStatistics (std::map<NodeId_t, SwitchType>& switchMap);
  template <typename SwitchType>
  void AddQueueStatistics (std::map<NodeId_t, SwitchType>& switchMap);
  void SaveXmlResultFile (const char* resultPath);

private:
  // Function will be called every time a terminal transmits a packet
  void TerminalPacketTransmission (std::string context, ns3::Ptr<const ns3::Packet> packet);
  void InsertTimeStamp (tinyxml2::XMLNode* node);
  inline uint32_t GetIpAddress (NodeId_t nodeId, ns3::NodeContainer& allNodes);
  inline tinyxml2::XMLNode* GetRootNode ();

  ns3::Ptr<ns3::FlowMonitor> m_flowMonitor;
  ns3::FlowMonitorHelper m_flowMonHelper;
  std::unique_ptr<tinyxml2::XMLDocument> m_xmlResultFile;
  /*
   * Key -> LinkId, Value -> Link Statistic
   * This map will store the link statistics of links that are originating from the
   * terminals.
   */
  std::map<LinkId_t, LinkStatistic> m_terminalLinkStatistics;
};

#endif /* RESULT_MANAGER_H */
