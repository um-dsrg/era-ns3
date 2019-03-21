#ifndef unipath_receiver_h
#define unipath_receiver_h

#include <list>

#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/data-rate.h"
#include "ns3/ipv4-address.h"

#include "../flow.h"
#include "application-base.h"
#include "../results-container.h"

class UnipathReceiver : public ReceiverBase
{

  portNum_t dstPort;
  ns3::Ptr<ns3::Socket> rxListenSocket; /**< The socket to listen for incoming connections. */
  ns3::Address dstAddress; /**< The path's destination address. */

  FlowProtocol protocol{FlowProtocol::Undefined};

  /**
     In the case of TCP, each socket accept returns a new socket, so the
     listening socket is stored separately from the accepted sockets.
     */
  std::list<ns3::Ptr<ns3::Socket>> m_rxAcceptedSockets; /**< List of the accepted sockets. */

  packetNumber_t m_packetNumber{0};

public:
  UnipathReceiver (const Flow &flow, ResultsContainer &resContainer);
  ~UnipathReceiver ();

private:
  void StartApplication ();
  void StopApplication ();

  void HandleAccept (ns3::Ptr<ns3::Socket> socket, const ns3::Address &from);
  void HandleRead (ns3::Ptr<ns3::Socket> socket);

  ResultsContainer &m_resContainer;
};
#endif /* unipath_receiver_h */
