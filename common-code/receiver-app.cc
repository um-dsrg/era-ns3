#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/inet-socket-address.h"

#include "mptcp-header.h"
#include "receiver-app.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ReceiverApp");

Ptr<Socket> CreateSocket(Ptr<Node> node, FlowProtocol protocol) {
    if (protocol == FlowProtocol::Tcp) { // Tcp Socket
        return Socket::CreateSocket(node, TcpSocketFactory::GetTypeId ());
    } else if (protocol == FlowProtocol::Udp) { // Udp Socket
        return Socket::CreateSocket(node, UdpSocketFactory::GetTypeId ());
    } else {
        NS_ABORT_MSG("Cannot create non TCP/UDP socket");
    }
}

ReceiverApp::ReceiverApp(const Flow& flow) : protocol(flow.protocol) {
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
    // TODO: Set all the sockets to 0 here.
}

double ReceiverApp::GetMeanRxGoodput() {
    // FIXME: When logging output the flow id.
    auto durationInSeconds = double{(m_lastRxPacket - m_firstRxPacket).GetSeconds()};
    auto currentGoodPut = double{((m_totalRecvBytes * 8) / durationInSeconds) /
                                 1'000'000};
    NS_LOG_INFO("The flow's good put is: " << currentGoodPut << "Mbps");
}

void ReceiverApp::StartApplication() {
    m_appRunning = true;
    NS_LOG_INFO("Receiver application started");

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
    NS_LOG_INFO("Receiver stopped");
}

void ReceiverApp::HandleAccept (Ptr<Socket> socket, const Address& from) {
    socket->SetRecvCallback(MakeCallback(&ReceiverApp::HandleRead, this));
    m_rxAcceptedSockets.push_back(socket);
}

void ReceiverApp::HandleRead(Ptr<Socket> socket) {
    NS_LOG_FUNCTION (this << socket);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    while ((packet = socket->RecvFrom (from))) {
        if (packet->GetSize () == 0) { //EOF
            break;
        }

        if (!m_firstPacketReceived) { // Log the time the first packet has been received
            m_firstPacketReceived = true;
            m_firstRxPacket = Simulator::Now();
        }

        m_lastRxPacket = Simulator::Now(); // Log the time the last packet is received

        packetNumber_t packetNumber;
        packetSize_t packetSize;
        std::tie(packetNumber, packetSize) = ExtractPacketDetails(packet);

        NS_LOG_INFO("Packet Number " << packetNumber << " received");
        NS_LOG_INFO("Packet size " << packetSize << "bytes");

        NS_ASSERT(packetNumber >= m_expectedPacketNum);

        if (packetNumber == m_expectedPacketNum) {
            m_totalRecvBytes += packetSize;
            m_expectedPacketNum++;

            popInOrderPacketsFromQueue();
        } else {
            m_recvBuffer.push(std::make_pair(packetNumber, packetSize));
        }
        NS_LOG_INFO("Total Received bytes " << m_totalRecvBytes);
        CalculateGoodPut();
    }
}

void ReceiverApp::popInOrderPacketsFromQueue() {
    bufferContents_t const * topElement = &m_recvBuffer.top();

    while (!m_recvBuffer.empty() && topElement->first == m_expectedPacketNum) {
        m_totalRecvBytes += topElement->second;
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
