#ifndef single_path_transmitter_h
#define single_path_transmitter_h

#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"
#include "ns3/application.h"
#include "ns3/ipv4-address.h"

#include "flow.h"
#include "definitions.h"
#include "application-base.h"

class SinglePathTransmitterApp : public ApplicationBase {
    
    ns3::DataRate m_dataRate; /**< Application's data rate. */
    ns3::Time m_transmissionInterval; /**< The time between packet transmissions. */
    packetSize_t m_packetSize {0}; /**< The packet size in bytes. */
    ns3::EventId m_sendEvent; /**< The Event Id of the pending TransmitPacket event. */
    packetNumber_t m_packetNumber {0};

    portNum_t srcPort;
    ns3::Ptr<ns3::Socket> txSocket; /**< The socket to transmit data on the given path. */
    ns3::Address dstAddress; /**< The path's destination address. */

public:
    SinglePathTransmitterApp (const Flow& flow);
    ~SinglePathTransmitterApp();

private:
    void StartApplication();
    void StopApplication();
    void TransmitPacket();
};

#endif /* single_path_transmitter_h */