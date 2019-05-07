#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/inet-socket-address.h"

#include "../mptcp-header.h"
#include "multipath-receiver.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MultipathReceiver");

/**************************************************************************************************/
/* Aggregate Buffer                                                                               */
/**************************************************************************************************/

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
  std::list<Ptr<Packet>> retrievedPackets;

  while (m_buffer.size () >= m_packetSize)
    {
      retrievedPackets.emplace_back (Create<Packet> (m_buffer.data (), m_packetSize));
      m_buffer.erase (m_buffer.begin (), m_buffer.begin () + m_packetSize);
    }

  return retrievedPackets;
}

void
AggregateBuffer::SetPacketSize (packetSize_t packetSize)
{
  m_packetSize = packetSize;
  NS_LOG_INFO ("The aggregate buffer packet size is: " << m_packetSize);
}

/**************************************************************************************************/
/* Receiver Buffer                                                                                */
/**************************************************************************************************/

ReceiverBuffer::ReceiverBuffer (id_t flowId, ResultsContainer &resContainer)
    : m_flowId (flowId), m_resContainer (resContainer)
{
}

void
ReceiverBuffer::AddPacketToBuffer (packetNumber_t packetNumber, packetSize_t packetSize)
{
  m_recvBuffer.push (std::make_pair (packetNumber, packetSize));
  m_bufferSize += packetSize;
  NS_LOG_INFO (Simulator::Now ().GetSeconds ()
               << "s: Packet " << packetNumber << ", " << packetSize
               << "bytes stored in the buffer. Buffer size: " << m_bufferSize << "bytes");
  m_resContainer.LogMstcpReceiverBufferSize (m_flowId, m_bufferSize);
}

std::pair<ReceiverBuffer::bufferContents_t, bool>
ReceiverBuffer::RetrievePacketFromBuffer (packetNumber_t packetNumber)
{
  auto packetRetrieved = bool{false};
  ReceiverBuffer::bufferContents_t retrievedPacket (0, 0);

  if (m_recvBuffer.empty () == false)
    {
      const auto &topPacket{m_recvBuffer.top ()};

      if (m_recvBuffer.top ().first == packetNumber)
        {
          packetRetrieved = true;
          retrievedPacket = topPacket;

          m_recvBuffer.pop ();
          m_bufferSize -= topPacket.second;
          NS_LOG_INFO (Simulator::Now ().GetSeconds ()
                       << "s: Packet " << retrievedPacket.first
                       << " retrieved from the buffer. Buffer size: " << m_bufferSize << "bytes");
        }
      else
        {
          NS_LOG_INFO (Simulator::Now ().GetSeconds ()
                       << "s: Packet " << packetNumber
                       << " not found in buffer. Buffer size:" << m_bufferSize << "bytes");
        }
    }

  return std::make_pair (retrievedPacket, packetRetrieved);
}

/**************************************************************************************************/
/* Multipath Receiver                                                                             */
/**************************************************************************************************/
std::tuple<packetNumber_t, packetSize_t> ExtractPacketDetails (ns3::Ptr<ns3::Packet> packet);

MultipathReceiver::MultipathReceiver (const Flow &flow, ResultsContainer &resContainer)
    : ReceiverBase (flow.id),
      m_receiverBuffer (flow.id, resContainer),
      m_resContainer (resContainer),
      m_protocol (flow.protocol)
{
  SetDataPacketSize (flow);
  m_aggregateBuffer.SetPacketSize (m_dataPacketSize + MptcpHeader ().GetSerializedSize ());

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
  NS_LOG_INFO (Simulator::Now ().GetSeconds () << "s: Flow " << m_id << " started reception.");

  // Initialise socket connections
  for (const auto &pathInfo : m_pathInfoContainer)
    {
      if (pathInfo.rxListenSocket->Bind (pathInfo.dstAddress) == -1)
        {
          NS_ABORT_MSG ("Failed to bind socket");
        }

      pathInfo.rxListenSocket->SetRecvCallback (
          MakeCallback (&MultipathReceiver::HandleRead, this));

      if (m_protocol == FlowProtocol::Tcp)
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
  NS_LOG_INFO (Simulator::Now ().GetSeconds () << "s: Flow " << m_id << " received some packets");

  Ptr<Packet> packet;
  Address from;

  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }

      m_aggregateBuffer.AddPacketToBuffer (packet);

      auto retrievedPackets{m_aggregateBuffer.RetrievePacketFromBuffer ()};

      for (auto &retrievedPacket : retrievedPackets)
        {
          packetNumber_t packetNumber;
          packetSize_t packetSize;
          std::tie (packetNumber, packetSize) = ExtractPacketDetails (retrievedPacket);

          NS_LOG_INFO (Simulator::Now ().GetSeconds ()
                       << "s: Packet " << packetNumber << " received. Data packet size "
                       << packetSize << "bytes | Expected packet number: " << m_expectedPacketNum);

          NS_ASSERT (packetNumber >= m_expectedPacketNum);

          if (packetNumber == m_expectedPacketNum)
            {
              m_resContainer.LogPacketReception (m_id, Simulator::Now (), packetNumber, packetSize);
              m_expectedPacketNum++;
              RetrievePacketsFromBuffer ();
            }
          else
            {
              m_receiverBuffer.AddPacketToBuffer (packetNumber, packetSize);
            }
        }
    }
}

void
MultipathReceiver::RetrievePacketsFromBuffer ()
{
  auto packetRetrieved = bool{false};
  ReceiverBuffer::bufferContents_t bufferContents (0, 0);

  do
    {
      std::tie (bufferContents, packetRetrieved) =
          m_receiverBuffer.RetrievePacketFromBuffer (m_expectedPacketNum);

      if (packetRetrieved)
        {
          m_resContainer.LogPacketReception (m_id, Simulator::Now (), bufferContents.first,
                                             bufferContents.second);
          m_expectedPacketNum++;
          NS_LOG_INFO ("Expected packet number: " << m_expectedPacketNum);
        }
    }
  while (packetRetrieved == true);
}

std::tuple<packetNumber_t, packetSize_t>
ExtractPacketDetails (ns3::Ptr<ns3::Packet> packet)
{
  MptcpHeader mptcpHeader;
  packet->RemoveHeader (mptcpHeader);
  return std::make_tuple (mptcpHeader.GetPacketNumber (), packet->GetSize ());
}
