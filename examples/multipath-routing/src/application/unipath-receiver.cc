#include "ns3/log.h"
#include "ns3/simulator.h"

#include "unipath-receiver.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("UnipathReceiver");

UnipathReceiver::UnipathReceiver (const Flow &flow, ResultsContainer &resContainer)
    : ReceiverBase (flow.id), m_resContainer (resContainer), protocol (flow.protocol)
{
  auto path = flow.GetDataPaths ().front ();

  dstPort = path.dstPort;
  rxListenSocket = CreateSocket (flow.dstNode->GetNode (), flow.protocol);
  dstAddress = Address (InetSocketAddress (flow.dstNode->GetIpAddress (), path.dstPort));

  m_dataPacketSize = flow.packetSize - CalculateHeaderSize (flow.protocol);
  NS_LOG_INFO ("Flow " << flow.id << "\nPacket size including headers: " << flow.packetSize
                       << "bytes\nPacket size excluding headers is: " << m_dataPacketSize
                       << "bytes");
}

UnipathReceiver::~UnipathReceiver ()
{
  rxListenSocket = nullptr;
  for (auto &acceptedSocket : m_rxAcceptedSockets)
    {
      acceptedSocket = nullptr;
    }
}

void
UnipathReceiver::StartApplication ()
{
  NS_LOG_INFO ("Flow " << m_id << " started reception.");

  // Initialise socket connections
  if (rxListenSocket->Bind (dstAddress) == -1)
    {
      NS_ABORT_MSG ("Failed to bind socket");
    }

  rxListenSocket->SetRecvCallback (MakeCallback (&UnipathReceiver::HandleRead, this));

  if (protocol == FlowProtocol::Tcp)
    {
      rxListenSocket->Listen ();
      rxListenSocket->ShutdownSend (); // Half close the connection;
      rxListenSocket->SetAcceptCallback (MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
                                         MakeCallback (&UnipathReceiver::HandleAccept, this));
    }
}

void
UnipathReceiver::StopApplication ()
{
  NS_LOG_INFO ("Flow " << m_id << " stopped reception.");
}

void
UnipathReceiver::HandleAccept (Ptr<Socket> socket, const Address &from)
{
  socket->SetRecvCallback (MakeCallback (&UnipathReceiver::HandleRead, this));
  m_rxAcceptedSockets.push_back (socket);
}

void
UnipathReceiver::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;

  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }

      if (!m_firstPacketReceived)
        { // Log the time the first packet has been received
          m_firstPacketReceived = true;
          m_firstRxPacket = Simulator::Now ();
        }

      m_lastRxPacket = Simulator::Now (); // Log the time the last packet is received

      auto recvBytes = packet->GetSize ();
      NS_LOG_INFO ("Flow " << m_id << " received " << recvBytes << "bytes of data at "
                           << Simulator::Now ().GetSeconds ());

      pendingBytes += recvBytes;

      // The floor and division should be here
      auto packetsReceived = floor (pendingBytes / static_cast<double> (m_dataPacketSize));
      NS_LOG_INFO ("Flow " << m_id << " extracted " << packetsReceived << " packet(s) at "
                           << Simulator::Now ().GetSeconds ());

      // Log for every data packet size, packet received
      for (auto i = 0; i < packetsReceived; ++i)
        {
          m_resContainer.LogPacketReception (m_id, Simulator::Now (), m_packetNumber,
                                             m_dataPacketSize);
          m_packetNumber++;
        }

      // Subtract the number of bytes that have been logged already
      pendingBytes -= packetsReceived * m_dataPacketSize;
      NS_LOG_INFO ("Number of pending bytes remaining: " << pendingBytes);

      m_totalRecvBytes += packet->GetSize ();
      NS_LOG_INFO ("Total Received bytes " << m_totalRecvBytes);
    }
}
