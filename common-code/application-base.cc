#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/udp-socket-factory.h"

#include "application-base.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ApplicationBase");

const std::map<packetNumber_t, ns3::Time>& ApplicationBase::GetDelayLog() const {
    return m_delayLog;
}

ApplicationBase::ApplicationBase(id_t id) : m_id(id) {
}

ApplicationBase::~ApplicationBase() {
}

Ptr<Socket> ApplicationBase::CreateSocket(Ptr<Node> srcNode, FlowProtocol protocol) {
    if (protocol == FlowProtocol::Tcp) { // Tcp Socket
        return Socket::CreateSocket(srcNode, TcpSocketFactory::GetTypeId ());
    } else if (protocol == FlowProtocol::Udp) { // Udp Socket
        return Socket::CreateSocket(srcNode, UdpSocketFactory::GetTypeId ());
    } else {
        NS_ABORT_MSG("Cannot create non TCP/UDP socket");
    }
}

void ApplicationBase::LogPacketTime(packetNumber_t packetNumber) {
    NS_LOG_DEBUG("Logging Packet " << packetNumber << " at " << Simulator::Now());
    m_delayLog.emplace(packetNumber, Simulator::Now());
}

double ApplicationBase::GetMeanRxGoodput() {
    NS_ABORT_MSG("Error: The GetMeanRxGoodput function from ApplicationBase cannot "
                 "be used directly");
    return 0.0;
}
