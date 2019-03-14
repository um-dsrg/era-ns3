#include <math.h>
#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>

#include "ns3/log.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket.h"

#include "mptcp-header.h"
#include "transmitter-app.h"
#include "random-generator-manager.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TransmitterApp");

TransmitterApp::TransmitterApp(const Flow& flow) : ApplicationBase(flow.id) {
    if (flow.dataRate.GetBitRate() <= 1e-5) { // Flow was assigned no data rate
        NS_LOG_INFO("Flow " << m_id << " assigned no data rate.");
        return;
    }

    for (const auto& path : flow.GetDataPaths()) {
        PathInformation pathInfo;
        pathInfo.srcPort = path.srcPort;
        pathInfo.txSocket = CreateSocket(flow.srcNode->GetNode(), flow.protocol);
        pathInfo.txSocket->TraceConnect("RTO", std::to_string(path.id),
                                        MakeCallback(&TransmitterApp::RtoChanged, this));
        pathInfo.dstAddress = Address(InetSocketAddress(flow.dstNode->GetIpAddress(), path.dstPort));

        // Create entry in the socket transmit buffer
        m_socketTxBuffer.emplace(pathInfo.txSocket, std::list<packetNumber_t>());

        auto ret =  m_pathInfoContainer.emplace(path.id, pathInfo);
        NS_ABORT_MSG_IF(!ret.second, "Trying to insert duplicate path: " << path.id <<
                                     " for flow " << flow.id);

        // Calculate the split ratio at path level
        auto splitRatio {path.dataRate.GetBitRate() /
                         static_cast<double>(flow.dataRate.GetBitRate())};

        m_pathSplitRatio.emplace_back(std::make_pair(splitRatio, path.id));
        NS_LOG_INFO("Path " << path.id <<
                    " | Flow Data Rate (bps): " << flow.dataRate <<
                    " | Path Data Rate (bps): " << path.dataRate <<
                    " | Split Ratio: " << splitRatio);

        if (flow.protocol == FlowProtocol::Tcp) {
            auto tcpSocket = ns3::DynamicCast<ns3::TcpSocket>(pathInfo.txSocket);
            ns3::UintegerValue segmentSize;
            tcpSocket->GetAttribute("SegmentSize", segmentSize);
            auto pktSizeInclMptcpHdr {m_dataPacketSize + MptcpHeader().GetSerializedSize()};

            if (pktSizeInclMptcpHdr > segmentSize.Get()) {
                NS_ABORT_MSG("The packet size for Flow " << flow.id <<
                             " is larger than the MSS.\n" <<
                             "Flow packet size (excl. Header): " <<
                             flow.packetSize << "\n" <<
                             "Flow packet size (incl. Header): " <<
                             pktSizeInclMptcpHdr << "\n" <<
                             "Maximum Segment Size: " << segmentSize.Get());
            }
        }
    }

    // Sort the split ratio vector in ascending order
    std::sort(m_pathSplitRatio.begin(), m_pathSplitRatio.end(), std::greater<>());

    // Calculate the cumulative split ratio
    NS_LOG_INFO("Path: " << m_pathSplitRatio[0].second << " Split Ratio: " << m_pathSplitRatio[0].first);
    for (size_t index = 1; index < m_pathSplitRatio.size(); ++index) {
        m_pathSplitRatio[index].first += m_pathSplitRatio[index - 1].first;
        NS_LOG_INFO("Path: " << m_pathSplitRatio[index].second << " Split Ratio: " << m_pathSplitRatio[index].first);
    }

    // Setup the random number generator
    m_uniformRandomVariable = RandomGeneratorManager::CreateUniformRandomVariable(0.0, 1.0);

    // If the number is very close to 1, set it equal to 1
    if (Abs(m_pathSplitRatio.back().first - 1.0) <= 1e-5) {
        m_pathSplitRatio.back().first = 1.0;
    }

    NS_ABORT_MSG_IF(m_pathSplitRatio.back().first != 1.0,
                    "The final split ratio is not equal to 1. Split Ratio: " <<
                    m_pathSplitRatio.back().first);

    // Set the data packet size
    SetDataPacketSize(flow);

    // Set the application's good put rate in bps
    SetApplicationGoodputRate(flow);

    // Calculate the transmission interval
    auto pktSizeBits = static_cast<double>(m_dataPacketSize * 8);
    auto transmissionInterval = double{pktSizeBits / m_dataRateBps};
    NS_ABORT_MSG_IF(transmissionInterval <= 0 || std::isnan(transmissionInterval),
                    "The transmission interval cannot be less than or equal to 0 OR nan. "
                    "Transmission interval: " << transmissionInterval);
    m_transmissionInterval = Seconds(transmissionInterval);
}

TransmitterApp::~TransmitterApp() {
    for (auto& pathInfoPair : m_pathInfoContainer) {
        auto& pathInfo {pathInfoPair.second};
        pathInfo.txSocket = 0;
    }
}

double TransmitterApp::GetTxGoodput() {
    return m_dataRateBps / 1000000;
}

void TransmitterApp::StartApplication() {
    if (m_dataRateBps <= 1e-5) { // Do not transmit anything if not allocated any data rate
        NS_LOG_INFO("Flow " << m_id << " did NOT start transmission on multiple paths");
        return;
    }

    NS_LOG_INFO("Flow " << m_id << " started transmitting on multiple paths");

    // Initialise socket connections
    for (const auto& pathPair : m_pathInfoContainer) {
        const auto& pathInfo = pathPair.second;

        // Create source socket
        InetSocketAddress srcAddr = InetSocketAddress(Ipv4Address::GetAny(), pathInfo.srcPort);

        if (pathInfo.txSocket->Bind(srcAddr) == -1) {
            NS_ABORT_MSG("Failed to bind socket");
        }

        if (pathInfo.txSocket->Connect(pathInfo.dstAddress) == -1) {
            NS_ABORT_MSG("Failed to connect socket");
        }
    }

    TransmitPacket();
}

void TransmitterApp::StopApplication() {
    NS_LOG_INFO("Flow " << m_id << " stopped transmitting.");
    Simulator::Cancel (m_sendEvent);
}


void TransmitterApp::TransmitPacket() {

    auto randNum = GetRandomNumber();
    auto transmitPathId = id_t{0};
    bool transmitPathFound {false};

    for (const auto& pathSplitPair : m_pathSplitRatio) {
        const auto& splitRatio {pathSplitPair.first};
        if (randNum <= splitRatio) {
            transmitPathId = pathSplitPair.second;
            transmitPathFound = true;
            break;
        }
    }

    NS_ABORT_MSG_IF(!transmitPathFound, "Flow " << m_id << " failed to find a transmit path."
                                        "Generated random number: " << randNum);

    // Packet transmission
    auto& txSocket {m_pathInfoContainer.at(transmitPathId).txSocket};

    if (txSocket->GetTxAvailable() >= (m_dataPacketSize + MptcpHeader().GetSerializedSize())) {
        auto packetNumber = m_packetNumber++;

        MptcpHeader mptcpHeader;
        mptcpHeader.SetPacketNumber(packetNumber);
        Ptr<Packet> packet = Create<Packet>(m_dataPacketSize);
        packet->AddHeader(mptcpHeader);

        auto numBytesSent = txSocket->Send(packet);

        if (numBytesSent == -1) {
            std::stringstream ss;
            ss << "Packet " << packetNumber << " failed to transmit. Packet size " << packet->GetSize() << "\n";
            auto error = txSocket->GetErrno();

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
            }

            NS_ABORT_MSG(ss.str());
        }

        LogPacketTime(packetNumber);
        NS_LOG_INFO("Flow " << m_id << " sent packet " << packetNumber << " at " << Simulator::Now());
    }

    m_sendEvent = Simulator::Schedule(m_transmissionInterval, &TransmitterApp::TransmitPacket, this);
}

void TransmitterApp::SetDataPacketSize(const Flow& flow) {
    m_dataPacketSize = flow.packetSize - CalculateHeaderSize(flow.protocol);

    NS_LOG_INFO("Packet size including headers is: " << flow.packetSize << "bytes\n" <<
                "Packet size excluding headers is: " << m_dataPacketSize <<"bytes");
}

void TransmitterApp::SetApplicationGoodputRate(const Flow& flow) {
    auto pktSizeExclHdr {flow.packetSize - CalculateHeaderSize(flow.protocol)};

    m_dataRateBps = (pktSizeExclHdr * flow.dataRate.GetBitRate()) /
                    static_cast<double>(flow.packetSize);

    NS_LOG_INFO("Flow throughput: " << flow.dataRate << "\n" <<
                "Flow goodput: " << m_dataRateBps << "bps");
}

void TransmitterApp::RtoChanged(std::string context, ns3::Time oldVal, ns3::Time newVal) {
    NS_LOG_INFO("RTO value changed for path " << context << ".\n" <<
                 "  Old Value " << oldVal.GetSeconds() << "\n" <<
                 "  New Value: " << newVal.GetSeconds());
}

packetSize_t TransmitterApp::CalculateHeaderSize(FlowProtocol protocol) {
    auto headerSize = ApplicationBase::CalculateHeaderSize(protocol);

    // Add the MultiStream TCP header
    headerSize += MptcpHeader().GetSerializedSize();

    return headerSize;
}

inline double TransmitterApp::GetRandomNumber() {
    return m_uniformRandomVariable->GetValue();
}
