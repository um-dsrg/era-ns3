#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/simulator.h"
#include "ns3/inet-socket-address.h"

#include "single-path-transmitter.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SinglePathTransmitterApp");

SinglePathTransmitterApp::SinglePathTransmitterApp(const Flow& flow) : ApplicationBase(flow.id) {
    auto path = flow.GetDataPaths().front();
    srcPort = path.srcPort;
    txSocket = CreateSocket(flow.srcNode->GetNode(), flow.protocol);
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
    NS_LOG_INFO("Flow " << m_id << " started transmission.");

    InetSocketAddress srcAddr = InetSocketAddress(Ipv4Address::GetAny(), srcPort);
    if (txSocket->Bind(srcAddr) == -1) {
        NS_ABORT_MSG("Failed to bind socket");
    }

    if (txSocket->Connect(dstAddress) == -1) {
        NS_ABORT_MSG("Failed to connect socket");
    }

    TransmitPacket();
}

void SinglePathTransmitterApp::StopApplication() {
    NS_LOG_INFO("Flow " << m_id << " stopped transmitting.");
    Simulator::Cancel (m_sendEvent);
}

void SinglePathTransmitterApp::TransmitPacket() {
    Ptr<Packet> packet = Create<Packet>(m_dataPacketSize);
    txSocket->Send(packet);

    LogPacketTime(m_packetNumber);
    m_packetNumber++;

    m_sendEvent = Simulator::Schedule(m_transmissionInterval, &SinglePathTransmitterApp::TransmitPacket, this);
}

void SinglePathTransmitterApp::SetDataPacketSize(const Flow& flow) {
    m_dataPacketSize = flow.packetSize - CalculateHeaderSize(flow.protocol);
}

void SinglePathTransmitterApp::SetApplicationGoodputRate(const Flow& flow) {
    auto pktSizeExclHdr {flow.packetSize - CalculateHeaderSize(flow.protocol)};

    m_dataRateBps = (pktSizeExclHdr * flow.dataRate.GetBitRate()) /
                    static_cast<double>(flow.packetSize);
}
