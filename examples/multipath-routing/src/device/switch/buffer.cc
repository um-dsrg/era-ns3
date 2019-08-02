#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/simulator.h"
#include "ns3/tcp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/ipv4-l3-protocol.h"

#include "buffer.h"
#include "../../results-container.h"

using ns3::Ipv4Header;
using ns3::Ipv4L3Protocol;
using ns3::Packet;
using ns3::Ptr;
using ns3::Simulator;
using ns3::TcpHeader;
using ns3::TcpL4Protocol;

NS_LOG_COMPONENT_DEFINE ("CUSTOM_Buffer");

Buffer::Buffer (const id_t switchId, uint64_t bufferSize, ResultsContainer &resContainer)
    : m_switchId (switchId),
      m_bufferSize (bufferSize),
      m_usedCapacity (0),
      m_resContainer (resContainer)
{
}

void
Buffer::AddToBuffer (Ptr<const Packet> packet, uint16_t protocol)
{
  auto packetSize = packetSize_t{packet->GetSize ()};
  auto freeSpace = uint64_t{m_bufferSize - m_usedCapacity};

  if (freeSpace < packetSize)
    {
      m_resContainer.LogPacketDrop (m_switchId, Simulator::Now ());
      return; // The buffer is full. Packet is dropped
    }

  if (AckPacket (packet, protocol))
    {
      m_ackPktsQueue.emplace (packet->Copy ());
      NS_LOG_INFO (Simulator::Now ().GetSeconds ()
                   << "s: Packet added to Switch " << m_switchId << " ACK buffer. "
                   << "Switch buffer size: " << m_bufferSize << "bytes. "
                   << "Used buffer capacity: " << m_usedCapacity << "bytes. "
                   << "Remaining capacity: " << (m_bufferSize - m_usedCapacity) << "bytes.");
    }
  else
    {
      m_dataPktsQueue.emplace (packet->Copy ());
      NS_LOG_INFO (Simulator::Now ().GetSeconds ()
                   << "s: Packet added to Switch " << m_switchId << " data buffer. "
                   << "Switch buffer size: " << m_bufferSize << "bytes. "
                   << "Used buffer capacity: " << m_usedCapacity << "bytes. "
                   << "Remaining capacity: " << (m_bufferSize - m_usedCapacity) << "bytes.");
    }

  m_usedCapacity += packetSize;
  m_resContainer.LogBufferSize (m_switchId, m_usedCapacity);

  NS_ABORT_MSG_IF (m_usedCapacity > m_bufferSize,
                   "Switch " << m_switchId << ": Buffer used beyond capacity. Capacity: "
                             << m_bufferSize << "bytes | Used: " << m_usedCapacity << "bytes");
}

std::pair<bool, ns3::Ptr<ns3::Packet>>
Buffer::RetrievePacketFromBuffer ()
{
  Ptr<Packet> retrievedPacket;

  if (m_ackPktsQueue.size () > 0)
    {
      retrievedPacket = m_ackPktsQueue.front ();
      m_ackPktsQueue.pop ();
      NS_LOG_INFO (Simulator::Now ().GetSeconds ()
                   << "s: Packet retrieved from Switch " << m_switchId << " ACK buffer");
    }
  else if (m_dataPktsQueue.size () > 0)
    {
      retrievedPacket = m_dataPktsQueue.front ();
      m_dataPktsQueue.pop ();
      NS_LOG_INFO (Simulator::Now ().GetSeconds ()
                   << "s: Packet retrieved from Switch " << m_switchId << " data buffer");
    }
  else
    { // Both queues are empty
      return std::make_pair (false, nullptr);
    }

  auto packetSize = packetSize_t{retrievedPacket->GetSize ()};
  m_usedCapacity -= packetSize;

  NS_ABORT_MSG_IF (m_usedCapacity < 0, "Switch " << m_switchId
                                                 << " has a negative used buffer capacity. "
                                                 << "Used capacity: " << m_usedCapacity);

  m_resContainer.LogBufferSize (m_switchId, m_usedCapacity);

  NS_LOG_INFO (Simulator::Now ().GetSeconds ()
               << "s: Packet Size: " << packetSize << "bytes. "
               << "Buffer Used capacity: " << m_usedCapacity << "bytes");

  return std::make_pair (true, retrievedPacket);
}

bool
Buffer::AckPacket (Ptr<const Packet> packet, const uint16_t protocol)
{
  auto ackPacket{false};

  Ptr<Packet> receivedPacket = packet->Copy (); // Copy the packet for parsing purposes

  if (protocol == Ipv4L3Protocol::PROT_NUMBER)
    {
      Ipv4Header ipHeader;
      auto transportLayerProtocol = uint8_t{0};

      if (receivedPacket->PeekHeader (ipHeader))
        {
          transportLayerProtocol = ipHeader.GetProtocol ();
          receivedPacket->RemoveHeader (ipHeader); // Remove IP header
        }

      if (transportLayerProtocol == TcpL4Protocol::PROT_NUMBER)
        {
          TcpHeader tcpHeader;
          if (receivedPacket->PeekHeader (tcpHeader))
            {
              NS_LOG_INFO (Simulator::Now ().GetSeconds ()
                           << "s: TCP packet received with the following flags: "
                           << tcpHeader.FlagsToString (tcpHeader.GetFlags ()));

              if (tcpHeader.GetFlags () & (1 << 5))
                {
                  ackPacket = true;
                }
            }
        }
      {
        NS_ABORT_MSG ("Non-IP Packet received. Protocol value " << protocol);
      }
    }

  return ackPacket;
}
