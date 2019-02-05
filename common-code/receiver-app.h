#ifndef receiver_app_h
#define receiver_app_h

#include <queue>
#include <tuple>

#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/ipv4-address.h"
#include "ns3/application.h"

#include "flow.h"

class ReceiverApp : public ns3::Application
{
public:
  ReceiverApp(const Flow& flow);
  virtual ~ReceiverApp();

private:
  virtual void StartApplication ();
  virtual void StopApplication ();
  void HandleRead (ns3::Ptr<ns3::Socket> socket);

  struct PathInformation
  {
    portNum_t dstPort;
    ns3::Ptr<ns3::Socket> rxSocket; //!< The socket to transmit data on the given path.
    ns3::Address dstAddress; //!< The path's destination address.
  };

  bool m_appRunning {false}; //!< Flag that determines the application's running stage
  std::vector<PathInformation> m_pathInfoContainer;

  /* Goodput calculation related variables */
  uint64_t m_totalRecvBytes{0};

  /* Buffer related variables */
  packetNumber_t m_expectedPacketNum{0};

  void popInOrderPacketsFromQueue();

  // TODO: Set this to a struct to make the code more readable
  typedef std::pair<packetNumber_t, packetSize_t> bufferContents_t;
  std::priority_queue<bufferContents_t, std::vector<bufferContents_t>, std::greater<>> m_recvBuffer;
};

std::tuple<packetNumber_t, packetSize_t> ExtractPacketDetails(ns3::Ptr<ns3::Packet> packet);

#endif /* receiver_app_h */
