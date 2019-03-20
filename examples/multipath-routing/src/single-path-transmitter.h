#ifndef single_path_transmitter_h
#define single_path_transmitter_h

#include <list>

#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"
#include "ns3/application.h"
#include "ns3/ipv4-address.h"

#include "flow.h"
#include "definitions.h"
#include "application-base.h"

class SinglePathTransmitterApp : public TransmitterBase
{
public:
  explicit SinglePathTransmitterApp (const Flow &flow);
  ~SinglePathTransmitterApp () override;

private:
  void StartApplication () override;
  void StopApplication () override;
  void TransmitPacket ();

  void RtoChanged (ns3::Time oldVal, ns3::Time newVal);

  portNum_t srcPort;
  ns3::Ptr<ns3::Socket> txSocket; /**< The socket to transmit data on the given path. */
  ns3::Address dstAddress; /**< The path's destination address. */
  std::list<packetNumber_t> m_txBuffer; /**< Socket level transmit buffer */
};

#endif /* single_path_transmitter_h */
