#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/simulator.h"
#include "ns3/inet-socket-address.h"

#include "single-path-transmitter.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SinglePathTransmitterApp");

SinglePathTransmitterApp::SinglePathTransmitterApp(const Flow& flow) : ApplicationBase(flow.id) {
    auto path = flow.GetDataPaths().front();
    srcPort = path.srcPort;txSocket = CreateSocket(flow.srcNode->GetNode(), flow.protocol);
    txSocket->SetSendCallback(MakeCallback(&SinglePathTransmitterApp::TxBufferAvailable, this));
    txSocket->TraceConnectWithoutContext("RTO", MakeCallback(&SinglePathTransmitterApp::RtoChanged, this));
    dstAddress = Address(InetSocketAddress(flow.dstNode->GetIpAddress(), path.dstPort));

    // Set the data packet size
    SetDataPacketSize(flow);

    // Set the application's good put rate in bps
    SetApplicationGoodputRate(flow);

    // Calculate the transmission interval
    auto pktSizeBits = static_cast<double>(m_dataPacketSize * 8);
    double transmissionInterval = pktSizeBits / m_dataRateBps;
    NS_ABORT_MSG_IF(transmissionInterval <= 0 || std::isnan(transmissionInterval),
                    "The transmission interval cannot be less than or equal to 0 OR nan. "
                    "Transmission interval: " << transmissionInterval);
    m_transmissionInterval = Seconds(transmissionInterval);
}

SinglePathTransmitterApp::~SinglePathTransmitterApp() {
    txSocket = nullptr;
}

void SinglePathTransmitterApp::StartApplication() {
    NS_LOG_INFO("Flow " << m_id << " started transmission on a single path");

    InetSocketAddress srcAddr = InetSocketAddress(Ipv4Address::GetAny(), srcPort);
    if (txSocket->Bind(srcAddr) == -1) {
        NS_ABORT_MSG("Failed to bind socket");
    }

    if (txSocket->Connect(dstAddress) == -1) {
        NS_ABORT_MSG("Failed to connect socket");
    }

    SchedulePacketTransmission();
}

void SinglePathTransmitterApp::StopApplication() {
    NS_LOG_INFO("Flow " << m_id << " stopped transmitting.");
    Simulator::Cancel (m_sendEvent);
}

void SinglePathTransmitterApp::SchedulePacketTransmission() {
    m_txBuffer.emplace_back(m_packetNumber++);
    SendPackets(txSocket);
    m_sendEvent = Simulator::Schedule(m_transmissionInterval, &SinglePathTransmitterApp::SchedulePacketTransmission, this);
}

void SinglePathTransmitterApp::TxBufferAvailable(ns3::Ptr<ns3::Socket> socket, uint32_t txSpace) {
    if (txSpace >= m_dataPacketSize) {
        SendPackets(socket);
    }
}

void SinglePathTransmitterApp::SendPackets(ns3::Ptr<ns3::Socket> socket) {
    while(socket->GetTxAvailable() >= m_dataPacketSize && !m_txBuffer.empty()) {
        auto packetNumber = m_txBuffer.front();
        NS_LOG_INFO("Flow " << m_id << " transmitting packet " << packetNumber << " at " << Simulator::Now());

        Ptr<Packet> packet = Create<Packet>(m_dataPacketSize);
        auto numBytesSent = txSocket->Send(packet);

        if (numBytesSent == -1) {
            std::stringstream ss;
            ss << "Packet " << packetNumber << " failed to transmit. Packet size " << packet->GetSize() << "\n";
            auto error = txSocket->Send(packet);

            switch (error) {
                case Socket::SocketErrno::ERROR_NOTERROR:
                    ss << "ERROR_NOTERROR";
                    break;
                case Socket::SocketErrno::ERROR_ISCONN:
                    ss << "ERROR_ISCONN";
                    break;
                case Socket::SocketErrno::ERROR_NOTCONN:
                    ss << "ERROR_NOTCONN";
                    break;
                case Socket::SocketErrno::ERROR_MSGSIZE:
                    ss << "ERROR_MSGSIZE";
                    break;
                case Socket::SocketErrno::ERROR_AGAIN:
                    ss << "ERROR_AGAIN";
                    break;
                case Socket::SocketErrno::ERROR_SHUTDOWN:
                    ss << "ERROR_SHUTDOWN";
                    break;
                case Socket::SocketErrno::ERROR_OPNOTSUPP:
                    ss << "ERROR_OPNOTSUPP";
                    break;
                case Socket::SocketErrno::ERROR_AFNOSUPPORT:
                    ss << "ERROR_AFNOSUPPORT";
                    break;
                case Socket::SocketErrno::ERROR_INVAL:
                    ss << "ERROR_INVAL";
                    break;
                case Socket::SocketErrno::ERROR_BADF:
                    ss << "ERROR_BADF";
                    break;
                case Socket::SocketErrno::ERROR_NOROUTETOHOST:
                    ss << "ERROR_NOROUTETOHOST";
                    break;
                case Socket::SocketErrno::ERROR_NODEV:
                    ss << "ERROR_NODEV";
                    break;
                case Socket::SocketErrno::ERROR_ADDRNOTAVAIL:
                    ss << "ERROR_ADDRNOTAVAIL";
                    break;
                case Socket::SocketErrno::ERROR_ADDRINUSE:
                    ss << "ERROR_ADDRINUSE";
                    break;
                case Socket::SocketErrno::SOCKET_ERRNO_LAST:
                    ss << "SOCKET_ERRNO_LAST";
                    break;
                default:
                    ss << "UNKNOWN ERROR!";
            }

            NS_ABORT_MSG(ss.str());
        }

        LogPacketTime(packetNumber);
        m_txBuffer.pop_front();
    }
}

void SinglePathTransmitterApp::RtoChanged(ns3::Time oldVal, ns3::Time newVal) {
    NS_LOG_INFO("RTO value changed for flow " << m_id << ".\n" <<
                "  Old Value " << oldVal.GetSeconds() << "\n" <<
                "  New Value: " << newVal.GetSeconds());
}

void SinglePathTransmitterApp::SetDataPacketSize(const Flow& flow) {
    m_dataPacketSize = flow.packetSize - CalculateHeaderSize(flow.protocol);
}

void SinglePathTransmitterApp::SetApplicationGoodputRate(const Flow& flow) {
    auto pktSizeExclHdr {flow.packetSize - CalculateHeaderSize(flow.protocol)};

    m_dataRateBps = (pktSizeExclHdr * flow.dataRate.GetBitRate()) /
                    static_cast<double>(flow.packetSize);
}
