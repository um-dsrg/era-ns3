#include "ns3/log.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/udp-socket-factory.h"

#include "transmitter-app.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Transmitter");

Ptr<Socket> createSocket(Ptr<Node> srcNode, FlowProtocol protocol)
{
  if (protocol == FlowProtocol::Tcp) { // Tcp Socket
    return Socket::CreateSocket(srcNode, TcpSocketFactory::GetTypeId ());
  } else if (protocol == FlowProtocol::Udp) { // Udp Socket
    return Socket::CreateSocket(srcNode, UdpSocketFactory::GetTypeId ());
  } else {
    NS_ABORT_MSG("Cannot create non TCP/UDP socket");
  }
}

TransmitterApp::TransmitterApp(const Flow& flow)
{
  for (const auto& path : flow.GetPaths()) {

    // Store the path information
    PathInformation pathInfo;
    pathInfo.srcPort = path.srcPort;
    pathInfo.txSocket = createSocket(flow.srcNode->GetNode(), flow.protocol);
    pathInfo.dstAddress = Address(InetSocketAddress(flow.dstNode->GetIpAddress(), path.dstPort));

    auto ret =  m_pathInfoContainer.emplace(path.id, pathInfo);
    NS_ABORT_MSG_IF(ret.second == false, "Trying to insert duplicate path: " << path.id <<
                                         " for flow " << flow.id);

    // Calculate the split ratio at path level
    m_pathSplitRatio.push_back(std::make_pair((path.dataRate / flow.dataRate), path.id));
  }

  // Sort the split ratio vector in ascending order
  std::sort(m_pathSplitRatio.begin(), m_pathSplitRatio.end(), std::greater<>());

  // Calculating the cumulative split ratio
  for (auto index = 1; index < m_pathSplitRatio.size(); ++index) {
    m_pathSplitRatio[index].first += m_pathSplitRatio[index - 1].first;
  }

  // FIXME: Make this equality to be close to one, not exactly one
  NS_ABORT_MSG_IF(m_pathSplitRatio.back().first == 1.0,
                  "The final split ratio is not equal to 1. Split Ratio: " <<
                  m_pathSplitRatio.back().first);
}

TransmitterApp::~TransmitterApp()
{
  // TODO: Set sockets to 0 to close them.
}

void TransmitterApp::StartApplication()
{
  m_appRunning = true;

  NS_LOG_UNCOND("Application started");

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

    // TODO: Remove the code from here and schedule events!
    Ptr<Packet> dummyPacket = Create<Packet>(100);
    pathInfo.txSocket->Send(dummyPacket);
  }
}

void TransmitterApp::StopApplication()
{
  NS_LOG_UNCOND("Application stopped");
}
