#ifndef transmitter_app_h
#define transmitter_app_h

#include <map>
#include <vector>

#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"
#include "ns3/application.h"
#include "ns3/ipv4-address.h"
#include "ns3/random-variable-stream.h"

#include "definitions.h"
#include "flow.h"

class TransmitterApp : public ns3::Application {
public:
  TransmitterApp (const Flow& flow);
  virtual ~TransmitterApp();

private:
  virtual void StartApplication ();
  virtual void StopApplication ();
  void TransmitPacket();

  inline double GetRandomNumber ();

  struct PathInformation
  {
    portNum_t srcPort;
    ns3::Ptr<ns3::Socket> txSocket; //!< The socket to transmit data on the given path.
    ns3::Address dstAddress; //!< The path's destination address.
  };

  bool m_appRunning {false}; //!< Flag that determines the application's running stage

  ns3::DataRate m_dataRate; //!< Application's data rate
  ns3::Time m_transmissionInterval; //!< The time between packet transmissions
  packetSize_t m_packetSize {0}; //!< The packet size in bytes
  packetNumber_t m_packetNumber {0}; //!< The number of the packet currently transmitted

  std::map<id_t, PathInformation> m_pathInfoContainer; //!< Path Id | Path Information
  std::vector<std::pair<double, id_t>> m_pathSplitRatio; //!< Split Ratio | Path Id
  ns3::EventId m_sendEvent; //!< The Event Id of the pending TransmitPacket event
  ns3::Ptr<ns3::UniformRandomVariable> m_uniformRandomVariable; //!< Random variable
};

#endif /* transmitter_app_h */
