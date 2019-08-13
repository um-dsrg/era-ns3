#include "ns3/simulator.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/ppp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/icmpv4-l4-protocol.h"

#include "transmit-queue.h"

NS_LOG_COMPONENT_DEFINE ("TransmitQueue");

using namespace ns3;

TransmitQueue::TransmitQueue () : NS_LOG_TEMPLATE_DEFINE ("TransmitQueue")
{
}

bool
TransmitQueue::Enqueue (ns3::Ptr<ns3::Packet> packet)
{
  if (m_retrievalMethod == RetrievalMethod::InOrder)
    {
      m_dataQueue.push (packet);
      NS_LOG_INFO (Simulator::Now ().GetSeconds ()
                   << "s - Switch " << m_switchId
                   << ": Add packet to DATA transmit buffer. Packets in DATA buffer: "
                   << m_dataQueue.size () << " Packets in ACK buffer: " << m_ackQueue.size ());
    }
  else
    {
      auto packetType = GetPacketType (packet);

      if (packetType == PacketType::Data)
        {
          m_dataQueue.push (packet);
          NS_LOG_INFO (Simulator::Now ().GetSeconds ()
                       << "s - Switch " << m_switchId
                       << ": Add packet to DATA transmit buffer. Packets in DATA buffer: "
                       << m_dataQueue.size () << " Packets in ACK buffer: " << m_ackQueue.size ());
        }
      else if (packetType == PacketType::Ack)
        {
          m_ackQueue.push (packet);
          NS_LOG_INFO (Simulator::Now ().GetSeconds ()
                       << "s - Switch " << m_switchId
                       << ": Add packet to ACK transmit buffer. Packets in DATA buffer: "
                       << m_dataQueue.size () << " Packets in ACK buffer: " << m_ackQueue.size ());
        }
      else
        {
          NS_ABORT_MSG ("Switch " << m_switchId << "Adding packet to transmit buffer failed.");
          return false;
        }
    }

  return true;
}

ns3::Ptr<ns3::Packet>
TransmitQueue::Dequeue ()
{
  if (m_dataQueue.empty ())
    return 0;

  ns3::Ptr<ns3::Packet> packet = m_dataQueue.front ();
  m_dataQueue.pop ();
  return packet;
}

ns3::Ptr<ns3::Packet>
TransmitQueue::Remove ()
{
  NS_ABORT_MSG ("TransmitQueue::Remove is not implemented and should not be used");
  return 0;
}

ns3::Ptr<const ns3::Packet>
TransmitQueue::Peek () const
{
  NS_ABORT_MSG ("TransmitQueue::Peek is not implemented and should not be used");
  return 0;
}

PacketType
TransmitQueue::GetPacketType (ns3::Ptr<const ns3::Packet> packet) const
{
  PacketType packetType{PacketType::Data};
  Ptr<Packet> receivedPacket = packet->Copy (); // Copy the packet for parsing purposes

  // Remove the PPP header
  PppHeader pppHeader;
  if (receivedPacket->PeekHeader (pppHeader))
    {
      receivedPacket->RemoveHeader (pppHeader);

      Ipv4Header ipHeader;
      uint8_t ipProtocol{0};

      if (receivedPacket->PeekHeader (ipHeader))
        { // Extract the transport layer protocol (UDP/TCP)
          ipProtocol = ipHeader.GetProtocol ();
          receivedPacket->RemoveHeader (ipHeader);

          if (ipProtocol == TcpL4Protocol::PROT_NUMBER) // TCP Packet
            {
              TcpHeader tcpHeader;
              uint8_t tcpFlags{0};

              if (receivedPacket->PeekHeader (tcpHeader))
                {
                  tcpFlags = tcpHeader.GetFlags ();
                  receivedPacket->RemoveHeader (tcpHeader);
                }

              // If ACK flag is set AND no data carried by packet, this is an ACK packet
              if ((tcpFlags & (1 << 5)) && (receivedPacket->GetSize () == 0))
                packetType = PacketType::Ack;
            }
        }
      else
        {
          NS_ABORT_MSG ("Non-IP Packet received");
        }
    }
  else
    {
      NS_ABORT_MSG ("Non P2P Packet received");
    }

  return packetType;
}
