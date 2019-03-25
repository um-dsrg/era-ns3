#ifndef results_container_h
#define results_container_h

#include <map>
#include <string>
#include <tinyxml2.h>

#include "ns3/nstime.h"

#include "flow.h"
#include "definitions.h"

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
  ns3::Time firstReception{0};
  ns3::Time lastReception{0};

  uint64_t totalRecvBytes;
  bool firstPacketReceived{false};

  std::map<id_t, PacketDetails> packetResults; /**< Key: Packet Id | Value: Packet Details */
};

class ResultsContainer
{
public:
  // TODO At construction phase we need to pass the flags of whether or not to keep all entries
  ResultsContainer (const Flow::flowContainer_t &flows);

  void LogPacketTransmission (id_t flowId, ns3::Time time, packetNumber_t pktNumber,
                              packetSize_t dataSize);
  void LogPacketReception (id_t flowId, ns3::Time time, packetNumber_t pktNumber,
                           packetSize_t dataSize);

  void AddFlowResults ();
  void AddQueueStatistics (tinyxml2::XMLElement *queueElement);
  void SaveFile (const std::string &path);

  // todo Put back to private
  tinyxml2::XMLDocument m_xmlDoc;

private:
  void InsertTimeStamp ();
  tinyxml2::XMLNode *m_rootNode;

  std::map<id_t, FlowResults> m_flowResults; /**< Key: Flow Id | Value: Flow Results */
};
#endif /* results_container_h */
