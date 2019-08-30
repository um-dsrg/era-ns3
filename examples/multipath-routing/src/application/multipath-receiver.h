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

class ReceiverBuffer
{
public:
  using bufferContents_t = std::pair<packetNumber_t, packetSize_t>;

  ReceiverBuffer (id_t flowId, ResultsContainer &resContainer);

  void AddPacketToBuffer (packetNumber_t packetNumber, packetNumber_t expectedPacketNumber,
                          packetSize_t packetSize);
  std::pair<bufferContents_t, bool> RetrievePacketFromBuffer (packetNumber_t packetNumber);

private:
  id_t m_flowId{0};
  bufferSize_t m_bufferSize{0};
  std::priority_queue<bufferContents_t, std::vector<bufferContents_t>, std::greater<>> m_recvBuffer;

  ResultsContainer &m_resContainer;
};

class MultipathReceiver : public ReceiverBase
{
public:
  MultipathReceiver (const Flow &flow, ResultsContainer &resContainer);
  virtual ~MultipathReceiver ();

private:
  void StartApplication ();
  void StopApplication ();

  packetSize_t CalculateHeaderSize (FlowProtocol protocol) override;
  void SetDataPacketSize (const Flow &flow) override;

  void HandleAccept (ns3::Ptr<ns3::Socket> socket, const ns3::Address &from);
  void HandleRead (ns3::Ptr<ns3::Socket> socket);

  void RetrievePacketsFromBuffer ();

  ReceiverBuffer m_receiverBuffer;
  AggregateBuffer m_aggregateBuffer;
  ResultsContainer &m_resContainer;
  packetNumber_t m_expectedPacketNum{0}; /**< The id of the expected packet */
  FlowProtocol m_protocol{FlowProtocol::Undefined}; /**< The flow's protocol */

  /**
     In the case of TCP, each socket accept returns a new socket, so the
     listening socket is stored separately from the accepted sockets.
     */
  std::list<ns3::Ptr<ns3::Socket>> m_rxAcceptedSockets;

  struct PathInformation
  {
    portNum_t dstPort;
    ns3::Ptr<ns3::Socket> rxListenSocket; /**< The socket to listen for incoming connections. */
    ns3::Address dstAddress; /**< The path's destination address. */
  };
  std::vector<PathInformation> m_pathInfoContainer;
};

#endif /* multipath_receiver_h */
