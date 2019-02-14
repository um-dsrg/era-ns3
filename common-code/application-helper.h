#ifndef application_helper_h
#define application_helper_h

#include <map>
#include <tinyxml2.h>

#include "ns3/ipv4.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"

#include "flow.h"
#include "terminal.h"
#include "definitions.h"
#include "topology-builder.h"


class ApplicationHelper {
public:

    using applicationContainer_t = std::map<id_t, ns3::Ptr<ns3::Application>>;

    void InstallApplicationsOnTerminals (const Flow::FlowContainer& flows,
                                         const Terminal::TerminalContainer& terminals);

    const applicationContainer_t& GetTransmitterApps() const;
    const applicationContainer_t& GetReceiverApps() const;

private:
    applicationContainer_t m_transmitterApplications;
    applicationContainer_t m_receiverApplications;
};

#endif /* application_helper_h */
