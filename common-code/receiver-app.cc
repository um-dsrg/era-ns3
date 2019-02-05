#include "ns3/log.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/inet-socket-address.h"

#include "receiver-app.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ReceiverApp");

// TODO: Set this in the app base class and remove the node parameter
Ptr<Socket> CreateSocket(Ptr<Node> node, FlowProtocol protocol)
{
  if (protocol == FlowProtocol::Tcp) { // Tcp Socket
    return Socket::CreateSocket(node, TcpSocketFactory::GetTypeId ());
  } else if (protocol == FlowProtocol::Udp) { // Udp Socket
    return Socket::CreateSocket(node, UdpSocketFactory::GetTypeId ());
  } else {
    NS_ABORT_MSG("Cannot create non TCP/UDP socket");
  }
}

ReceiverApp::ReceiverApp(const Flow& flow)
{
  for (const auto& path : flow.GetPaths()) {
    PathInformation pathInfo;
    pathInfo.dstPort = path.dstPort;
    pathInfo.rxSocket = CreateSocket(flow.dstNode->GetNode(), flow.protocol);
    pathInfo.dstAddress = Address(InetSocketAddress(flow.dstNode->GetIpAddress(), path.dstPort));

    m_pathInfoContainer.push_back(pathInfo);
  }
}

ReceiverApp::~ReceiverApp()
{
  // TODO: Set all the sockets to 0 here.
}

void ReceiverApp::StartApplication()
{
  m_appRunning = true;
  NS_LOG_UNCOND("Receiver application started");

  // Initialise socket connections
  for (const auto& pathInfo : m_pathInfoContainer) {
    if (pathInfo.rxSocket->Bind(pathInfo.dstAddress) == -1) {
      NS_ABORT_MSG("Failed to bind socket");
    }

    pathInfo.rxSocket->Listen();
    pathInfo.rxSocket->ShutdownSend (); // Half close the connection;
    pathInfo.rxSocket->SetRecvCallback (MakeCallback(&ReceiverApp::PacketReceived, this));
  }
}

void ReceiverApp::StopApplication()
{
  NS_LOG_UNCOND("Receiver stopped");
}

void ReceiverApp::PacketReceived(Ptr<Socket> socket)
{
  NS_LOG_UNCOND("A packet has been received");
}
