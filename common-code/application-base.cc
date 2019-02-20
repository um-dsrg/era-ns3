#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-header.h"
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

packetSize_t ApplicationBase::CalculateHeaderSize(FlowProtocol protocol) {
    packetSize_t headerSize {0};

    // Add TCP/UDP header size
    if(protocol == FlowProtocol::Tcp) {
        headerSize += 32; // 20 bytes header + 12 bytes options
    } else if(protocol == FlowProtocol::Udp) {
        headerSize += 8; // 8 bytes udp header
    } else {
        NS_ABORT_MSG("Unknown protocol. Protocol " << static_cast<char>(protocol));
    }

    // Add Ip header size
    headerSize += Ipv4Header().GetSerializedSize();

    // Add Point To Point size
    headerSize += 2;

    return headerSize;
}
