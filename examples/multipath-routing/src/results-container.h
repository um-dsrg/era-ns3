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
  double txGoodput{0.0}; /**<The rate at which the application is generating its data */
  ns3::Time firstReception{0}; /**< The time the first packet was received */
  ns3::Time lastReception{0}; /**< The time the last packet was received */

  uint64_t totalRecvBytes{0}; /**< The total number of received bytes */
  uint64_t totalRecvPackets{0}; /**< The total number of received packets */
  bool firstPacketReceived{false}; /**< Flag that is enabled when the first packet is received */

  std::map<id_t, PacketDetails> packetResults; /**< Key: Packet Id | Value: Packet Details */
};

class ResultsContainer
{
public:
  // TODO At construction phase we need to pass the flags of whether or not to keep all entries
  ResultsContainer (const Flow::flowContainer_t &flows);

  void LogFlowTxGoodputRate (id_t flowId, double goodputRate);
  void LogPacketTransmission (id_t flowId, ns3::Time time, packetNumber_t pktNumber,
                              packetSize_t dataSize);
  void LogPacketReception (id_t flowId, ns3::Time time, packetNumber_t pktNumber,
                           packetSize_t dataSize);

  void AddFlowResults ();
  void AddQueueStatistics (tinyxml2::XMLElement *queueElement);
  void SaveFile (const std::string &path);

private:
  void InsertTimeStamp ();
  tinyxml2::XMLNode *m_rootNode;

  std::map<id_t, FlowResults> m_flowResults; /**< Key: Flow Id | Value: Flow Results */
  tinyxml2::XMLDocument m_xmlDoc; /**< XML Document */
};
#endif /* results_container_h */
