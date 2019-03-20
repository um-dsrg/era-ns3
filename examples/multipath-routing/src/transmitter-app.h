#ifndef transmitter_app_h
#define transmitter_app_h

#include <map>
#include <list>
#include <vector>

#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"
#include "ns3/application.h"
#include "ns3/ipv4-address.h"
#include "ns3/random-variable-stream.h"

#include "flow.h"
#include "definitions.h"
#include "application-base.h"

class TransmitterApp : public TransmitterBase
{
public:
  explicit TransmitterApp (const Flow &flow);
  virtual ~TransmitterApp ();

private:
  void StartApplication () override;
  void StopApplication () override;
  void TransmitPacket ();

  void RtoChanged (std::string context, ns3::Time oldVal, ns3::Time newVal);

  packetSize_t CalculateHeaderSize (FlowProtocol protocol) override;
  inline double GetRandomNumber ();

  struct PathInformation
  {
    portNum_t srcPort;
    ns3::Ptr<ns3::Socket> txSocket; /**< The socket to transmit data on the given path. */
    ns3::Address dstAddress; /**< The path's destination address. */
  };

  std::map<id_t, PathInformation> m_pathInfoContainer; /**< Path Id | Path Information */
  std::vector<std::pair<double, id_t>> m_pathSplitRatio; /**< Split Ratio | Path Id */
  ns3::Ptr<ns3::UniformRandomVariable> m_uniformRandomVariable; /**< Random variable */
  std::map<ns3::Ptr<ns3::Socket>, std::list<packetNumber_t>>
      m_socketTxBuffer; /**< Socket level transmit buffer */
};

#endif /* transmitter_app_h */
