#ifndef transmitter_app_h
#define transmitter_app_h

#include <map>
#include <vector>

#include "ns3/socket.h"
#include "ns3/ipv4-address.h"
#include "ns3/applications-module.h"

#include "definitions.h"
#include "flow.h"

class TransmitterApp : public ns3::Application {
public:
  TransmitterApp (const Flow& flow);
  virtual ~TransmitterApp();

private:
  virtual void StartApplication ();
  virtual void StopApplication ();

  struct PathInformation
  {
    portNum_t srcPort;
    ns3::Ptr<ns3::Socket> txSocket; //!< The socket to transmit data on the given path.
    ns3::Address dstAddress; //!< The path's destination address.
  };

  bool m_appRunning {false}; //!< Flag that determines the application's running stage
  dataRate_t m_dataRate {0.0}; //!< Application's data rate
  std::map<id_t, PathInformation> m_pathInfoContainer; //!< Path Id | Path Information
  std::vector<std::pair<double, id_t>> m_pathSplitRatio; //!< Split Ratio | Path Id
};

#endif /* transmitter_app_h */
