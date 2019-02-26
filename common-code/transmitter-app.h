#ifndef transmitter_app_h
#define transmitter_app_h

#include <map>
#include <list>
#include <vector>

#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"
#include "ns3/application.h"
#include "ns3/ipv4-address.h"
#include "ns3/random-variable-stream.h"

#include "flow.h"
#include "definitions.h"
#include "application-base.h"

class TransmitterApp : public ApplicationBase {
public:
    explicit TransmitterApp (const Flow& flow);
    virtual ~TransmitterApp();

private:
    void StartApplication() override;
    void StopApplication() override;

    void SchedulePacketTransmission();
    void TxBufferAvailable(ns3::Ptr<ns3::Socket> socket, uint32_t txSpace);
    void SendPackets(ns3::Ptr<ns3::Socket> socket);

    void SetDataPacketSize(const Flow& flow);
    void SetApplicationGoodputRate(const Flow& flow);

    void RtoChanged(std::string context, ns3::Time oldVal, ns3::Time newVal);

    packetSize_t CalculateHeaderSize(FlowProtocol protocol);
    inline double GetRandomNumber();

    struct PathInformation {
        portNum_t srcPort;
        ns3::Ptr<ns3::Socket> txSocket; /**< The socket to transmit data on the given path. */
        ns3::Address dstAddress; /**< The path's destination address. */
    };

    double m_dataRateBps {0}; /**< Application's bit rate in bps. */
    ns3::Time m_transmissionInterval; /**< The time between packet transmissions. */
    packetSize_t m_dataPacketSize {0}; /**< The data packet size in bytes. */
    packetNumber_t m_packetNumber {0}; /**< The number of the packet currently transmitted. */

    std::map<id_t, PathInformation> m_pathInfoContainer; /**< Path Id | Path Information */
    std::vector<std::pair<double, id_t>> m_pathSplitRatio; /**< Split Ratio | Path Id */
    ns3::EventId m_sendEvent; /**< The Event Id of the pending TransmitPacket event */
    ns3::Ptr<ns3::UniformRandomVariable> m_uniformRandomVariable; /**< Random variable */
    std::map<ns3::Ptr<ns3::Socket>, std::list<packetNumber_t>> m_socketTxBuffer; /**< Socket level transmit buffer */
};

#endif /* transmitter_app_h */
