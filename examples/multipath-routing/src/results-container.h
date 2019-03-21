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
  PacketDetails (ns3::Time transmitted, packetSize_t packetSize);

  ns3::Time received;
  ns3::Time transmitted;
  packetSize_t packetSize;
};

struct FlowResults
{
  std::map<id_t, PacketDetails> packetDetails; /**< Key: Packet Id | Value: Packet Details */
};

class ResultsContainer
{
public:
  // TODO At construction phase we need to pass the flags of whether or not to keep all entries
  ResultsContainer (const Flow::flowContainer_t &flows);

  void LogPacketTransmission (id_t flowId, ns3::Time time, packetNumber_t pktNumber,
                              packetSize_t pktSize);
  void LogPacketReception (id_t flowId, ns3::Time time, packetNumber_t pktNumber,
                           packetSize_t pktSize);

  // void AddGoodputResults (const Flow::FlowContainer &flows,
  //                         const AppContainer::applicationContainer_t &transmitterApplications,
  //                         const AppContainer::applicationContainer_t &receiverApplications);
  // void AddDelayResults (const AppContainer::applicationContainer_t &transmitterApplications,
  //                       const AppContainer::applicationContainer_t &receiverApplications);
  // void AddDelayResults (const AppContainer &appHelper);
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
