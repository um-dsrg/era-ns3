#ifndef app_container_h
#define app_container_h

#include <map>
#include <memory>
#include <tinyxml2.h>

#include "ns3/ipv4.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"

#include "../flow.h"
#include "../terminal.h"
#include "../definitions.h"
#include "../topology-builder.h"
#include "application-base.h"

class AppContainer
{
public:
  using transmitterAppContainer_t = std::map<id_t, std::unique_ptr<TransmitterBase>>;
  using receiverAppContainer_t = std::map<id_t, std::unique_ptr<ReceiverBase>>;
  // using applicationContainer_t = std::map<id_t, ns3::Ptr<ns3::Application>>;

  void InstallApplicationsOnTerminals (const Flow::FlowContainer &flows,
                                       const Terminal::TerminalContainer &terminals,
                                       bool usePpfsSwitches);

  // const applicationContainer_t &GetTransmitterApps () const;
  // const applicationContainer_t &GetReceiverApps () const;
  // const std::map<id_t, std::pair<uint64_t, double>> &GetCompressedDelayLog () const;

  // void CompressDelayLog ();

private:
  template <typename TransmitterApp, typename ReceiverApp>
  void InstallApplicationOnTerminal (const Flow &flow);

  std::map<id_t, std::unique_ptr<TransmitterBase>> m_transmitterApplications;
  std::map<id_t, std::unique_ptr<ReceiverBase>> m_receiverApplications;

  // applicationContainer_t m_transmitterApplications;
  // applicationContainer_t m_receiverApplications;

  // TODO The below needs to be removed from this class

  /* Store the per flow compressed delay log */
  // std::map<id_t /* flow id */,
  //          std::pair<uint64_t /* number of packets */, double /* total delay */>>
  //     m_flowDelayLog;
};

#endif /* app_container_h */
