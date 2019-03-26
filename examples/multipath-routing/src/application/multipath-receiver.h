#ifndef multipath_receiver_h
#define multipath_receiver_h

#include <list>
#include <queue>
#include <tuple>
#include <vector>

#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/buffer.h"
#include "ns3/data-rate.h"
#include "ns3/ipv4-address.h"
#include "ns3/application.h"

#include "../flow.h"
#include "application-base.h"
#include "../results-container.h"

class AggregateBuffer
{
  packetSize_t m_packetSize{0}; /**< The packet size including any custom headers */
  std::vector<uint8_t> m_buffer;

public:
  AggregateBuffer () = default;
  explicit AggregateBuffer (packetSize_t packetSize);

  void SetPacketSize (packetSize_t packetSize);

  void AddPacketToBuffer (ns3::Ptr<ns3::Packet> packet);
  std::list<ns3::Ptr<ns3::Packet>> RetrievePacketFromBuffer ();
};

class MultipathReceiver : public ReceiverBase
{

  struct PathInformation
  {
    portNum_t dstPort;
    ns3::Ptr<ns3::Socket> rxListenSocket; /**< The socket to listen for incoming connections. */
    ns3::Address dstAddress; /**< The path's destination address. */
  };
  std::vector<PathInformation> m_pathInfoContainer;

  FlowProtocol protocol{FlowProtocol::Undefined};
  packetSize_t m_dataPacketSize{0}; /**< The data packet size in bytes. */
  AggregateBuffer aggregateBuffer;

  /**
     In the case of TCP, each socket accept returns a new socket, so the
     listening socket is stored separately from the accepted sockets.
     */
  std::list<ns3::Ptr<ns3::Socket>> m_rxAcceptedSockets; /**< List of the accepted sockets. */

public:
  MultipathReceiver (const Flow &flow, ResultsContainer& resContainer);
  virtual ~MultipathReceiver ();

private:
  void StartApplication ();
  void StopApplication ();

  packetSize_t CalculateHeaderSize (FlowProtocol protocol);
  void SetDataPacketSize (const Flow &flow);

  void HandleAccept (ns3::Ptr<ns3::Socket> socket, const ns3::Address &from);
  void HandleRead (ns3::Ptr<ns3::Socket> socket);

  ResultsContainer &m_resContainer;

  /* Buffer related variables */
  // TODO: Set this to a class to make the code more readable, a class may be worthwhile because we need to log this
  packetNumber_t m_expectedPacketNum{0};
  void popInOrderPacketsFromQueue ();
  typedef std::pair<packetNumber_t, packetSize_t> bufferContents_t;
  std::priority_queue<bufferContents_t, std::vector<bufferContents_t>, std::greater<>> m_recvBuffer;
};

#endif /* multipath_receiver_h */