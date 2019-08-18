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

TransmitQueue::TransmitQueue (const std::string &retrievalMethod, id_t switchId)
    : m_switchId (switchId), NS_LOG_TEMPLATE_DEFINE ("TransmitQueue")
{
  if (retrievalMethod.compare ("AckPriority") == 0)
    {
      m_retrievalMethod = RetrievalMethod::AckPriority;
      m_retrievalFunction = std::bind (&TransmitQueue::AckPriorityRetrieval, this);
    }
  else if (retrievalMethod.compare ("InOrder") == 0)
    {
      m_retrievalMethod = RetrievalMethod::InOrder;
      m_retrievalFunction = std::bind (&TransmitQueue::InOrderRetrieval, this);
    }
  else if (retrievalMethod.compare ("RoundRobin") == 0)
    {
      m_retrievalMethod = RetrievalMethod::RoundRobin;
      m_retrievalFunction = std::bind (&TransmitQueue::RoundRobinRetrieval, this);
    }
  else
    NS_ABORT_MSG ("Retrieval method " << retrievalMethod << " does not exist");
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
          NS_ABORT_MSG ("Switch " << m_switchId << ": Adding packet to transmit buffer failed.");
          return false;
        }
    }

  return true;
}

ns3::Ptr<ns3::Packet>
TransmitQueue::Dequeue ()
{
  return m_retrievalFunction ();
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

  PppHeader pppHeader;
  if (receivedPacket->RemoveHeader (pppHeader))
    {
      Ipv4Header ipHeader;

      if (receivedPacket->RemoveHeader (ipHeader))
        { // Extract the transport layer protocol (UDP/TCP)
          uint8_t ipProtocol{ipHeader.GetProtocol ()};

          if (ipProtocol == TcpL4Protocol::PROT_NUMBER) // TCP Packet
            {
              TcpHeader tcpHeader;
              receivedPacket->RemoveHeader (tcpHeader);
              uint8_t tcpFlags{tcpHeader.GetFlags ()};

              // If ACK flag is set AND no data carried by packet, this is an ACK packet
              if (((tcpFlags & TcpHeader::ACK) == TcpHeader::ACK) &&
                  (receivedPacket->GetSize () == 0))
                {
                  packetType = PacketType::Ack;
                }
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

ns3::Ptr<ns3::Packet>
TransmitQueue::GetAckPacket ()
{
  if (m_ackQueue.empty ())
    return 0;

  auto retrievedPacket = m_ackQueue.front ();
  m_ackQueue.pop ();

  NS_LOG_INFO (Simulator::Now ().GetSeconds ()
               << "s - Switch " << m_switchId << ": Packet retrieved from ACK buffer");

  return retrievedPacket;
}

ns3::Ptr<ns3::Packet>
TransmitQueue::GetDataPacket ()
{
  if (m_dataQueue.empty ())
    return 0;

  auto retrievedPacket = m_dataQueue.front ();
  m_dataQueue.pop ();

  NS_LOG_INFO (Simulator::Now ().GetSeconds ()
               << "s - Switch " << m_switchId << ": Packet retrieved from DATA buffer");

  return retrievedPacket;
}

ns3::Ptr<ns3::Packet>
TransmitQueue::InOrderRetrieval ()
{
  return GetDataPacket ();
}

ns3::Ptr<ns3::Packet>
TransmitQueue::RoundRobinRetrieval ()
{
  ns3::Ptr<ns3::Packet> retrievedPacket{0};

  if (m_rrTransmitAckPacket)
    {
      retrievedPacket = GetAckPacket ();
      if (retrievedPacket == 0) // ACK queue empty, get from DATA queue
        retrievedPacket = GetDataPacket ();
    }
  else
    {
      retrievedPacket = GetDataPacket ();
      if (retrievedPacket == 0) // DATA queue empty, get from ACK queue
        retrievedPacket = GetAckPacket ();
    }

  m_rrTransmitAckPacket = !m_rrTransmitAckPacket;

  return retrievedPacket;
}

ns3::Ptr<ns3::Packet>
TransmitQueue::AckPriorityRetrieval ()
{
  auto retrievedPacket = GetAckPacket ();

  if (retrievedPacket == 0) // Ack Queue is empty, try to transmit data queue
    retrievedPacket = GetDataPacket ();

  return retrievedPacket;
}
