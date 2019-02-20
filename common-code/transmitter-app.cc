#include <math.h>
#include <algorithm>

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket.h"

#include "mptcp-header.h"
#include "transmitter-app.h"
#include "random-generator-manager.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TransmitterApp");

TransmitterApp::TransmitterApp(const Flow& flow) : ApplicationBase(flow.id) {
    for (const auto& path : flow.GetDataPaths()) {
        PathInformation pathInfo;
        pathInfo.srcPort = path.srcPort;
        pathInfo.txSocket = CreateSocket(flow.srcNode->GetNode(), flow.protocol);
        pathInfo.dstAddress = Address(InetSocketAddress(flow.dstNode->GetIpAddress(), path.dstPort));

        auto ret =  m_pathInfoContainer.emplace(path.id, pathInfo);
        NS_ABORT_MSG_IF(ret.second == false, "Trying to insert duplicate path: " << path.id <<
                                             " for flow " << flow.id);

        // Calculate the split ratio at path level
        auto splitRatio {path.dataRate.GetBitRate() /
                         static_cast<double>(flow.dataRate.GetBitRate())};

        m_pathSplitRatio.push_back(std::make_pair(splitRatio, path.id));
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
    for (size_t index = 1; index < m_pathSplitRatio.size(); ++index) {
        m_pathSplitRatio[index].first += m_pathSplitRatio[index - 1].first;
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
    double pktSizeBits = static_cast<double>(m_dataPacketSize * 8);
    double transmissionInterval = pktSizeBits / m_dataRateBps;
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

void TransmitterApp::StartApplication() {
    NS_LOG_INFO("Flow " << m_id << " started transmitting.");

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

    for (const auto& pathSplitPair : m_pathSplitRatio) {
        const auto& splitRatio {pathSplitPair.first};
        if (randNum <= splitRatio) {
            transmitPathId = pathSplitPair.second;
            break;
        }
    }

    auto& pathInfo = m_pathInfoContainer.at(transmitPathId);

    auto pktNumber {m_packetNumber++};

    MptcpHeader mptcpHeader;
    mptcpHeader.SetPacketNumber(pktNumber);
    Ptr<Packet> packet = Create<Packet>(m_dataPacketSize);
    packet->AddHeader(mptcpHeader);
    pathInfo.txSocket->Send(packet);

    LogPacketTime(pktNumber);

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

packetSize_t TransmitterApp::CalculateHeaderSize(FlowProtocol protocol) {
    auto headerSize = ApplicationBase::CalculateHeaderSize(protocol);

    // Add the MultiStream TCP header
    headerSize += MptcpHeader().GetSerializedSize();

    return headerSize;
}

inline double TransmitterApp::GetRandomNumber() {
    return m_uniformRandomVariable->GetValue();
}
