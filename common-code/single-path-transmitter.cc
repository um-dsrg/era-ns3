#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/simulator.h"
#include "ns3/inet-socket-address.h"

#include "single-path-transmitter.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SinglePathTransmitterApp");

SinglePathTransmitterApp::SinglePathTransmitterApp(const Flow& flow) :
ApplicationBase(flow.id), m_dataRate(flow.dataRate), m_packetSize(flow.packetSize) {
    auto path = flow.GetDataPaths().front();
    srcPort = path.srcPort;
    txSocket = CreateSocket(flow.srcNode->GetNode(), flow.protocol);
    dstAddress = Address(InetSocketAddress(flow.dstNode->GetIpAddress(), path.dstPort));

    // Calculate the transmission interval
    double pktSizeBits = static_cast<double>(m_packetSize * 8);
    double transmissionInterval = pktSizeBits / static_cast<double>(m_dataRate.GetBitRate());
    NS_ABORT_MSG_IF(transmissionInterval <= 0 || std::isnan(transmissionInterval),
                    "The transmission interval cannot be less than or equal to 0 OR nan. "
                    "Transmission interval: " << transmissionInterval);
    m_transmissionInterval = Seconds(transmissionInterval);
}

SinglePathTransmitterApp::~SinglePathTransmitterApp() {
    txSocket = 0;
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
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    txSocket->Send(packet);

    LogPacketTime(m_packetNumber);
    m_packetNumber++;

    m_sendEvent = Simulator::Schedule(m_transmissionInterval, &SinglePathTransmitterApp::TransmitPacket, this);
}
