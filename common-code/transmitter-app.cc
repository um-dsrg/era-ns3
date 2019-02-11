#include <math.h>

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/udp-socket-factory.h"

#include "mptcp-header.h"
#include "transmitter-app.h"
#include "random-generator-manager.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TransmitterApp");

TransmitterApp::TransmitterApp(const Flow& flow) : m_dataRate(flow.dataRate), m_packetSize(flow.packetSize) {
    for (const auto& path : flow.GetDataPaths()) {
        PathInformation pathInfo;
        pathInfo.srcPort = path.srcPort;
        pathInfo.txSocket = CreateSocket(flow.srcNode->GetNode(), flow.protocol);
        pathInfo.dstAddress = Address(InetSocketAddress(flow.dstNode->GetIpAddress(), path.dstPort));

        auto ret =  m_pathInfoContainer.emplace(path.id, pathInfo);
        NS_ABORT_MSG_IF(ret.second == false, "Trying to insert duplicate path: " << path.id <<
                                             " for flow " << flow.id);

        // Calculate the split ratio at path level
        m_pathSplitRatio.push_back(std::make_pair((path.dataRate.GetBitRate() / flow.dataRate.GetBitRate()),
                                                  path.id));

        if (flow.protocol == FlowProtocol::Tcp) {
            auto tcpSocket = ns3::DynamicCast<ns3::TcpSocket>(pathInfo.txSocket);
            ns3::UintegerValue segmentSize;
            tcpSocket->GetAttribute("SegmentSize", segmentSize);
            auto pktSizeInclHdrs {flow.packetSize + MptcpHeader().GetSerializedSize()};

            if (pktSizeInclHdrs > segmentSize.Get()) {
                NS_ABORT_MSG("The packet size for Flow " << flow.id << " is larger than the MSS.\n"
                             "Flow packet size (excl. Header): " << flow.packetSize << "\n" <<
                             "Flow packet size (incl. Header): " << pktSizeInclHdrs << "\n" <<
                             "Maximum Segment Size: " << segmentSize.Get());
            }
        }
    }

    // Sort the split ratio vector in ascending order
    std::sort(m_pathSplitRatio.begin(), m_pathSplitRatio.end(), std::greater<>());

    // Calculate the cumulative split ratio
    for (auto index = 1; index < m_pathSplitRatio.size(); ++index) {
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

    // Calculate the transmission interval
    double pktSizeBits = static_cast<double>(m_packetSize * 8);
    double transmissionInterval = pktSizeBits / static_cast<double>(m_dataRate.GetBitRate());
    NS_ABORT_MSG_IF(transmissionInterval <= 0 || isnan(transmissionInterval),
                    "The transmission interval cannot be less than or equal to 0 OR nan. "
                    "Transmission interval: " << transmissionInterval);
    m_transmissionInterval = Seconds(transmissionInterval);
}

Ptr<Socket> TransmitterApp::CreateSocket(Ptr<Node> srcNode, FlowProtocol protocol) {
    if (protocol == FlowProtocol::Tcp) { // Tcp Socket
        return Socket::CreateSocket(srcNode, TcpSocketFactory::GetTypeId ());
    } else if (protocol == FlowProtocol::Udp) { // Udp Socket
        return Socket::CreateSocket(srcNode, UdpSocketFactory::GetTypeId ());
    } else {
        NS_ABORT_MSG("Cannot create non TCP/UDP socket");
    }
}

TransmitterApp::~TransmitterApp() {
  // TODO: Set sockets to 0 to close them.
}

void TransmitterApp::StartApplication() {
    m_appRunning = true;

    NS_LOG_UNCOND("Transmitter application started");

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
    NS_LOG_UNCOND("Application stopped");

    m_appRunning = false;
    Simulator::Cancel (m_sendEvent);
}

void TransmitterApp::TransmitPacket() {
    if (m_appRunning) {
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

        MptcpHeader mptcpHeader;
        mptcpHeader.SetPacketNumber(m_packetNumber++);
        Ptr<Packet> packet = Create<Packet>(m_packetSize);
        packet->AddHeader(mptcpHeader);
        pathInfo.txSocket->Send(packet);
        m_sendEvent = Simulator::Schedule(m_transmissionInterval, &TransmitterApp::TransmitPacket, this);
    }
}

inline double TransmitterApp::GetRandomNumber() {
    return m_uniformRandomVariable->GetValue();
}
