#include "ns3/log.h"
#include "ns3/simulator.h"

#include "single-path-receiver.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SinglePathReceiverApp");

SinglePathReceiver::SinglePathReceiver(const Flow& flow) :
ApplicationBase(flow.id), protocol(flow.protocol) {
    auto path = flow.GetDataPaths().front();

    dstPort = path.dstPort;
    rxListenSocket = CreateSocket(flow.dstNode->GetNode(), flow.protocol);
    dstAddress = Address(InetSocketAddress(flow.dstNode->GetIpAddress(), path.dstPort));
}

SinglePathReceiver::~SinglePathReceiver() {
    rxListenSocket = 0;
    for (auto& acceptedSocket : m_rxAcceptedSockets) {
        acceptedSocket = 0;
    }
}

double SinglePathReceiver::GetMeanRxGoodput() {
    auto durationInSeconds = double{(m_lastRxPacket - m_firstRxPacket).GetSeconds()};
    auto currentGoodPut = double{((m_totalRecvBytes * 8) / durationInSeconds) /
                                 1'000'000};
    return currentGoodPut;
}

void SinglePathReceiver::StartApplication() {
    NS_LOG_INFO("Flow " << m_id << " started reception.");

    // Initialise socket connections
    if (rxListenSocket->Bind(dstAddress) == -1) {
        NS_ABORT_MSG("Failed to bind socket");
    }

    rxListenSocket->SetRecvCallback(MakeCallback(&SinglePathReceiver::HandleRead, this));

    if (protocol == FlowProtocol::Tcp) {
        rxListenSocket->Listen();
        rxListenSocket->ShutdownSend (); // Half close the connection;
        rxListenSocket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
                                          MakeCallback (&SinglePathReceiver::HandleAccept, this));
    }

}

void SinglePathReceiver::StopApplication() {
    NS_LOG_INFO("Flow " << m_id << " stopped reception.");
}

void SinglePathReceiver::HandleAccept(Ptr<Socket> socket, const Address& from) {
    socket->SetRecvCallback(MakeCallback(&SinglePathReceiver::HandleRead, this));
    m_rxAcceptedSockets.push_back(socket);
}

void SinglePathReceiver::HandleRead(Ptr<Socket> socket) {
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

        LogPacketTime(m_packetNumber);
        m_packetNumber++;
        m_totalRecvBytes += packet->GetSize();

        NS_LOG_INFO("Total Received bytes " << m_totalRecvBytes);
    }
}
