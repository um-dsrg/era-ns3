#ifndef unipath_transmitter_h
#define unipath_transmitter_h

#include <list>

#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"
#include "ns3/application.h"
#include "ns3/ipv4-address.h"

#include "../flow.h"
#include "../definitions.h"
#include "application-base.h"
#include "../results-container.h"

class UnipathTransmitter : public TransmitterBase
{
public:
  explicit UnipathTransmitter (const Flow &flow, ResultsContainer &resContainer);
  ~UnipathTransmitter () override;

private:
  void StartApplication () override;
  void StopApplication () override;
  void TransmitPacket () override;

  void RtoChanged (ns3::Time oldVal, ns3::Time newVal);

  /**
   * The path id that all data is logged to be transmitted. A uni-path
   * transmitter will only transmit flows over a single path, it is the switches
   * that will take care of splitting the packets over multiple paths. However,
   * for logging purposes a valid path id needs to be given when logging
   * transmission because of the packet transmission counter. Therefore, all
   * packets will be logged as they are transmitted on the first path id of the
   * given flow.
   */
  id_t m_transmitPathId = 0;

  portNum_t srcPort;
  ns3::Ptr<ns3::Socket> txSocket; /**< The socket to transmit data on the given path. */
  ns3::Address dstAddress; /**< The path's destination address. */
  std::list<packetNumber_t> m_txBuffer; /**< Socket level transmit buffer */

  ResultsContainer &m_resContainer;
};

#endif /* unipath_transmitter_h */
