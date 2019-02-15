#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/inet-socket-address.h"

#include "mptcp-header.h"
#include "receiver-app.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ReceiverApp");

/**
 AggregateBuffer implementation
 */

AggregateBuffer::AggregateBuffer(packetSize_t packetSize) : m_packetSize(packetSize) {
}

void AggregateBuffer::AddPacketToBuffer(ns3::Ptr<ns3::Packet> packet) {
    uint8_t* tmpBuffer = new uint8_t [packet->GetSize()];
    packet->CopyData(tmpBuffer, packet->GetSize());

    for (uint32_t i = 0; i < packet->GetSize(); ++i) {
        m_buffer.push_back(tmpBuffer[i]);
    }

    delete [] tmpBuffer;
}

std::list<Ptr<Packet>> AggregateBuffer::RetrievePacketFromBuffer() {
    std::list<Ptr<Packet>> retreivedPackets;

    while (m_buffer.size() >= m_packetSize) {
        retreivedPackets.emplace_back(Create<Packet>(m_buffer.data(), m_packetSize));
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + m_packetSize);
    }

    return retreivedPackets;
}

/**
 ReceiverApp implementation
 */
std::tuple<packetNumber_t, packetSize_t> ExtractPacketDetails(ns3::Ptr<ns3::Packet> packet);

ReceiverApp::ReceiverApp(const Flow& flow) :
ApplicationBase(flow.id), protocol(flow.protocol), pktSize(flow.packetSize),
aggregateBuffer(AggregateBuffer(pktSize + MptcpHeader().GetSerializedSize()))
{
    for (const auto& path : flow.GetDataPaths()) {
        PathInformation pathInfo;
        pathInfo.dstPort = path.dstPort;
        pathInfo.rxListenSocket = CreateSocket(flow.dstNode->GetNode(),
                                               flow.protocol);
        pathInfo.dstAddress = Address(InetSocketAddress(flow.dstNode->GetIpAddress(),
                                                        path.dstPort));

        m_pathInfoContainer.push_back(pathInfo);
    }
}

ReceiverApp::~ReceiverApp() {
    for (auto& acceptedSocket : m_rxAcceptedSockets) {
        acceptedSocket = 0;
    }

    for (auto& pathInfo : m_pathInfoContainer) {
        pathInfo.rxListenSocket = 0;
    }
}

double ReceiverApp::GetMeanRxGoodput() {
    auto durationInSeconds = double{(m_lastRxPacket - m_firstRxPacket).GetSeconds()};
    auto currentGoodPut = double{((m_totalRecvBytes * 8) / durationInSeconds) /
                                 1'000'000};
    return currentGoodPut;
}

void ReceiverApp::StartApplication() {
    NS_LOG_INFO("Flow " << m_id << " started reception.");

    // Initialise socket connections
    for (const auto& pathInfo : m_pathInfoContainer) {
        if (pathInfo.rxListenSocket->Bind(pathInfo.dstAddress) == -1) {
            NS_ABORT_MSG("Failed to bind socket");
        }

        pathInfo.rxListenSocket->SetRecvCallback(MakeCallback(&ReceiverApp::HandleRead, this));

        if (protocol == FlowProtocol::Tcp) {
            pathInfo.rxListenSocket->Listen();
            pathInfo.rxListenSocket->ShutdownSend (); // Half close the connection;
            pathInfo.rxListenSocket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
                                                       MakeCallback (&ReceiverApp::HandleAccept, this));
        }
    }
}

void ReceiverApp::StopApplication() {
    NS_LOG_INFO("Flow " << m_id << " stopped reception.");
}

void ReceiverApp::HandleAccept (Ptr<Socket> socket, const Address& from) {
    socket->SetRecvCallback(MakeCallback(&ReceiverApp::HandleRead, this));
    m_rxAcceptedSockets.push_back(socket);
}

void ReceiverApp::HandleRead(Ptr<Socket> socket) {
    NS_LOG_FUNCTION (this << socket);
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom (from))) {
        if (packet->GetSize () == 0) { //EOF
            break;
        }

        if (m_firstPacketReceived == false) { // Log the time the first packet has been received
            m_firstPacketReceived = true;
            m_firstRxPacket = Simulator::Now();
        }

        m_lastRxPacket = Simulator::Now(); // Log the time the last packet is received

        aggregateBuffer.AddPacketToBuffer(packet);

        auto retreivedPackets {aggregateBuffer.RetrievePacketFromBuffer()};

        for (auto& retreivedPacket : retreivedPackets) {
            packetNumber_t packetNumber;
            packetSize_t packetSize;
            std::tie(packetNumber, packetSize) = ExtractPacketDetails(retreivedPacket);

            NS_LOG_INFO("Packet Number " << packetNumber << " received");
            NS_LOG_INFO("Packet size " << packetSize << "bytes");

            NS_ASSERT(packetNumber >= m_expectedPacketNum);

            if (packetNumber == m_expectedPacketNum) {
                LogPacketTime(packetNumber);
                m_totalRecvBytes += packetSize;

                m_expectedPacketNum++;
                popInOrderPacketsFromQueue();
            } else {
                m_recvBuffer.push(std::make_pair(packetNumber, packetSize));
            }
        }

        NS_LOG_INFO("Total Received bytes " << m_totalRecvBytes);
    }
}

void ReceiverApp::popInOrderPacketsFromQueue() {
    bufferContents_t const * topElement = &m_recvBuffer.top();

    while (!m_recvBuffer.empty() && topElement->first == m_expectedPacketNum) {
        m_totalRecvBytes += topElement->second;
        LogPacketTime(m_expectedPacketNum);

        m_expectedPacketNum++;
        m_recvBuffer.pop();
        topElement = &m_recvBuffer.top();
    }
}

std::tuple<packetNumber_t, packetSize_t> ExtractPacketDetails(ns3::Ptr<ns3::Packet> packet) {
    MptcpHeader mptcpHeader;
    packet->RemoveHeader(mptcpHeader);
    return std::make_tuple(mptcpHeader.GetPacketNumber(), packet->GetSize());
}
