#ifndef application_base_h
#define application_base_h

#include <map>

#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/application.h"

#include "flow.h"
#include "definitions.h"

class ApplicationBase : public ns3::Application {
public:
    const std::map<packetNumber_t, ns3::Time>& GetDelayLog() const;
    virtual double GetMeanRxGoodput();
    virtual double GetTxGoodput();
    std::map<packetNumber_t, ns3::Time> m_delayLog;

protected:
    ApplicationBase(id_t id);
    virtual ~ApplicationBase();
    virtual void StartApplication() = 0;
    virtual void StopApplication() = 0;

    ns3::Ptr<ns3::Socket> CreateSocket(ns3::Ptr<ns3::Node> srcNode, FlowProtocol protocol);
    void LogPacketTime(packetNumber_t packetNumber);
    virtual packetSize_t CalculateHeaderSize(FlowProtocol protocol);

    id_t m_id{0}; /**< The id of the flow that this application represents. */
};

#endif /* application_base_h */
