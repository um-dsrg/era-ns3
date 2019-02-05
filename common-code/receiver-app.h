#ifndef receiver_app_h
#define receiver_app_h

#include "ns3/socket.h"
#include "ns3/ipv4-address.h"
#include "ns3/applications-module.h"

#include "flow.h"

class ReceiverApp : public ns3::Application
{
public:
  ReceiverApp(const Flow& flow);
  virtual ~ReceiverApp();

private:
  virtual void StartApplication ();
  virtual void StopApplication ();
  void PacketReceived (ns3::Ptr<ns3::Socket> socket);

  struct PathInformation
  {
    portNum_t dstPort;
    ns3::Ptr<ns3::Socket> rxSocket; //!< The socket to transmit data on the given path.
    ns3::Address dstAddress; //!< The path's destination address.
  };

  bool m_appRunning {false}; //!< Flag that determines the application's running stage
  dataRate_t m_dataRate {0.0}; //!< Application's data rate
  std::vector<PathInformation> m_pathInfoContainer;
};

#endif /* receiver_app_h */