#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/inet-socket-address.h"

#include "../mptcp-header.h"
#include "multipath-receiver.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MultipathReceiver");

/**
 AggregateBuffer implementation
 */

AggregateBuffer::AggregateBuffer (packetSize_t packetSize) : m_packetSize (packetSize)
{
}

void
AggregateBuffer::AddPacketToBuffer (ns3::Ptr<ns3::Packet> packet)
{
  uint8_t *tmpBuffer = new uint8_t[packet->GetSize ()];
  packet->CopyData (tmpBuffer, packet->GetSize ());

  for (uint32_t i = 0; i < packet->GetSize (); ++i)
    {
      m_buffer.push_back (tmpBuffer[i]);
    }

  delete[] tmpBuffer;
}

std::list<Ptr<Packet>>
AggregateBuffer::RetrievePacketFromBuffer ()
{
  std::list<Ptr<Packet>> retreivedPackets;

  while (m_buffer.size () >= m_packetSize)
    {
      retreivedPackets.emplace_back (Create<Packet> (m_buffer.data (), m_packetSize));
      m_buffer.erase (m_buffer.begin (), m_buffer.begin () + m_packetSize);
    }

  return retreivedPackets;
}

void
AggregateBuffer::SetPacketSize (packetSize_t packetSize)
{
  m_packetSize = packetSize;
  NS_LOG_INFO ("The aggregate buffer packet size is: " << m_packetSize);
}

/**
 ReceiverApp implementation
 */
std::tuple<packetNumber_t, packetSize_t> ExtractPacketDetails (ns3::Ptr<ns3::Packet> packet);

MultipathReceiver::MultipathReceiver (const Flow &flow, ResultsContainer &resContainer)
    : ReceiverBase (flow.id), protocol (flow.protocol), m_resContainer (resContainer)
{

  SetDataPacketSize (flow);
  aggregateBuffer.SetPacketSize (m_dataPacketSize + MptcpHeader ().GetSerializedSize ());

  for (const auto &path : flow.GetDataPaths ())
    {
      PathInformation pathInfo;
      pathInfo.dstPort = path.dstPort;
      pathInfo.rxListenSocket = CreateSocket (flow.dstNode->GetNode (), flow.protocol);
      pathInfo.dstAddress =
          Address (InetSocketAddress (flow.dstNode->GetIpAddress (), path.dstPort));

      m_pathInfoContainer.push_back (pathInfo);
    }
}

MultipathReceiver::~MultipathReceiver ()
{
  for (auto &acceptedSocket : m_rxAcceptedSockets)
    {
      acceptedSocket = 0;
    }

  for (auto &pathInfo : m_pathInfoContainer)
    {
      pathInfo.rxListenSocket = 0;
    }
}

void
MultipathReceiver::StartApplication ()
{
  NS_LOG_INFO ("Flow " << m_id << " started reception.");

  // Initialise socket connections
  for (const auto &pathInfo : m_pathInfoContainer)
    {
      if (pathInfo.rxListenSocket->Bind (pathInfo.dstAddress) == -1)
        {
          NS_ABORT_MSG ("Failed to bind socket");
        }

      pathInfo.rxListenSocket->SetRecvCallback (
          MakeCallback (&MultipathReceiver::HandleRead, this));

      if (protocol == FlowProtocol::Tcp)
        {
          pathInfo.rxListenSocket->Listen ();
          pathInfo.rxListenSocket->ShutdownSend (); // Half close the connection;
          pathInfo.rxListenSocket->SetAcceptCallback (
              MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
              MakeCallback (&MultipathReceiver::HandleAccept, this));
        }
    }
}

void
MultipathReceiver::StopApplication ()
{
  NS_LOG_INFO ("Flow " << m_id << " stopped reception.");
}

packetSize_t
MultipathReceiver::CalculateHeaderSize (FlowProtocol protocol)
{
  auto headerSize = ApplicationBase::CalculateHeaderSize (protocol);

  // Add the MultiStream TCP header
  headerSize += MptcpHeader ().GetSerializedSize ();

  return headerSize;
}

void
MultipathReceiver::SetDataPacketSize (const Flow &flow)
{
  m_dataPacketSize = flow.packetSize - CalculateHeaderSize (flow.protocol);

  NS_LOG_INFO ("Packet size including headers is: " << flow.packetSize << "bytes\n"
                                                    << "Packet size excluding headers is: "
                                                    << m_dataPacketSize << "bytes");
}

void
MultipathReceiver::HandleAccept (Ptr<Socket> socket, const Address &from)
{
  socket->SetRecvCallback (MakeCallback (&MultipathReceiver::HandleRead, this));
  m_rxAcceptedSockets.push_back (socket);
}

void
MultipathReceiver::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_INFO ("Flow " << m_id << " received some packets at " << Simulator::Now ());

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

      aggregateBuffer.AddPacketToBuffer (packet);

      auto retrievedPackets{aggregateBuffer.RetrievePacketFromBuffer ()};

      for (auto &retrievedPacket : retrievedPackets)
        {
          packetNumber_t packetNumber;
          packetSize_t packetSize;
          std::tie (packetNumber, packetSize) = ExtractPacketDetails (retrievedPacket);

          NS_LOG_INFO ("Expected packet number: " << m_expectedPacketNum);
          NS_LOG_INFO ("Packet " << packetNumber << " received at " << Simulator::Now ()
                                 << " data packet size " << packetSize << "bytes");

          NS_ASSERT (packetNumber >= m_expectedPacketNum);

          if (packetNumber == m_expectedPacketNum)
            {
              m_resContainer.LogPacketReception (m_id, Simulator::Now (), packetNumber, packetSize);
              m_totalRecvBytes += packetSize;
              NS_LOG_INFO ("The expected packet is received. Total Received bytes "
                           << m_totalRecvBytes);

              m_expectedPacketNum++;
              popInOrderPacketsFromQueue ();
            }
          else
            {
              NS_LOG_INFO ("Packet " << packetNumber << " stored in the buffer");
              m_recvBuffer.push (std::make_pair (packetNumber, packetSize));
            }
        }
    }
}

void
MultipathReceiver::popInOrderPacketsFromQueue ()
{
  bufferContents_t const *topElement = &m_recvBuffer.top ();

  NS_LOG_INFO ("Checking contents of the buffer. Expected packet number: " << m_expectedPacketNum);

  while (!m_recvBuffer.empty () && topElement->first == m_expectedPacketNum)
    {
      m_totalRecvBytes += topElement->second;
      m_resContainer.LogPacketReception (m_id, Simulator::Now (), m_expectedPacketNum,
                                         topElement->second);

      NS_LOG_INFO ("Retrieved packet " << topElement->first << " from the buffer.\n"
                                       << "Total received bytes: " << m_totalRecvBytes << "\n");

      m_expectedPacketNum++;
      m_recvBuffer.pop ();
      topElement = &m_recvBuffer.top ();
      NS_LOG_INFO ("Expected packet number: " << m_expectedPacketNum);
    }
}

std::tuple<packetNumber_t, packetSize_t>
ExtractPacketDetails (ns3::Ptr<ns3::Packet> packet)
{
  MptcpHeader mptcpHeader;
  packet->RemoveHeader (mptcpHeader);
  return std::make_tuple (mptcpHeader.GetPacketNumber (), packet->GetSize ());
}
