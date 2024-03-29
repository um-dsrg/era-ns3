#ifndef app_container_h
#define app_container_h

#include <map>
#include <memory>
#include <tinyxml2.h>

#include "ns3/ipv4.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"

#include "../flow.h"
#include "../definitions.h"
#include "../topology-builder.h"
#include "application-base.h"
#include "../results-container.h"
#include "../device/terminal/terminal.h"

class AppContainer
{
public:
  using transmitterAppContainer_t = std::map<id_t, std::unique_ptr<TransmitterBase>>;
  using receiverAppContainer_t = std::map<id_t, std::unique_ptr<ReceiverBase>>;

  void InstallApplicationsOnTerminals (const Flow::flowContainer_t &flows, bool usePpfsSwitches,
                                       ResultsContainer &resContainer);

private:
  template <typename TransmitterApp, typename ReceiverApp>
  void InstallApplicationOnTerminal (const Flow &flow, ResultsContainer &resContainer);

  std::map<id_t, std::unique_ptr<TransmitterBase>> m_transmitterApplications;
  std::map<id_t, std::unique_ptr<ReceiverBase>> m_receiverApplications;
};

#endif /* app_container_h */
