#ifndef results_container_h
#define results_container_h

#include <map>
#include <string>
#include <tinyxml2.h>

#include "ns3/nstime.h"
#include "ns3/socket.h"

#include "flow.h"
#include "definitions.h"
#include "device/switch/switch-container.h"

struct PacketDetails
{
  PacketDetails (ns3::Time transmitted, packetSize_t transmittedDataSize);

  ns3::Time received;
  ns3::Time transmitted;
  // The number of data bytes stored in the packet
  packetSize_t transmittedDataSize = 0;
  packetSize_t receivedDataSize = 0;
};

struct FlowResults
{
  double txGoodput{0.0}; /**<The rate at which the application is generating its data */
  ns3::Time firstReception{0}; /**< The time the first packet was received */
  ns3::Time lastReception{0}; /**< The time the last packet was received */
  ns3::Time totalDelay{0}; /**< The total delay time */

  uint64_t totalRecvBytes{0}; /**< The total number of received bytes */
  uint64_t totalRecvPackets{0}; /**< The total number of received packets */
  bool firstPacketReceived{false}; /**< Flag that is enabled when the first packet is received */

  bufferSize_t maxMstcpRecvBufferSize{0}; /**< The largest receiver buffer size in bytes */

  std::map<id_t, PacketDetails> packetResults; /**< Key: Packet Id | Value: Packet Details */
};

struct SwitchResults
{
  uint64_t numDroppedPackets{0};
  uint64_t maxBufferUsage{0}; /**< The largest used buffer size. */
};

class ResultsContainer
{
public:
  ResultsContainer (bool logPacketResults);

  void SetupFlowResults (const Flow::flowContainer_t &flows);
  void SetupSwitchResults (const SwitchContainer &switchContainer);

  // Log Flow results
  void LogFlowTxGoodputRate (id_t flowId, double goodputRate);
  void LogPacketTransmission (id_t flowId, const ns3::Time &time, packetNumber_t pktNumber,
                              packetSize_t dataSize, ns3::Ptr<ns3::Socket> socket);
  void LogPacketReception (id_t flowId, const ns3::Time &time, packetNumber_t pktNumber,
                           packetSize_t dataSize);
  void LogMstcpReceiverBufferSize (id_t flowId, bufferSize_t bufferSize);

  // Log Switch results
  void LogPacketDrop (id_t switchId, const ns3::Time &time);
  void LogBufferSize (id_t switchIt, uint64_t bufferSize);

  // Save results to XML file
  void AddFlowResults ();
  void AddSwitchResults ();
  void AddSimulationParameters (const std::string &inputFile, const std::string &outputFile,
                                const std::string &flowMonitorOutput, const std::string &stopTime,
                                bool enablePcap, bool useSack, bool usePpfsSwitches,
                                bool useSdnSwitches, bool perPacketDelayLog,
                                const std::string &switchPortBufferSize, uint64_t switchBufferSize,
                                bool logPacketResults);
  void SaveFile (const std::string &path);

private:
  std::string GetSocketDetails (ns3::Ptr<ns3::Socket> socket);
  void InsertTimeStamp ();

  bool m_logPacketResults{false};
  std::map<id_t, FlowResults> m_flowResults; /**< Key: Flow Id | Value: Flow Results */
  std::map<id_t, SwitchResults> m_switchResults; /**< Key: Switch Id | Value: Switch Results */

  tinyxml2::XMLNode *m_rootNode; /**< XML Root Node */
  tinyxml2::XMLDocument m_xmlDoc; /**< XML Document */
};
#endif /* results_container_h */
