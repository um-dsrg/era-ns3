#include "ns3/log.h"
#include "ns3/simulator.h"

#include "transmit-buffer.h"

NS_LOG_COMPONENT_DEFINE ("TransmitBuffer");

using namespace ns3;

const std::map<std::string, RetrievalMethod> TransmitBuffer::RetreivalMethodMap =
  {{"RoundRobin", RetrievalMethod::RoundRobin},
   {"AckPriority", RetrievalMethod::AckPriority},
   {"InOrder", RetrievalMethod::InOrder}};

TransmitBuffer::TransmitBuffer(const std::string& retrievalMethod, id_t switchId):
  m_switchId (switchId)
{
  try
  {
    m_retrievalMethod = TransmitBuffer::RetreivalMethodMap.at(retrievalMethod);

    switch(m_retrievalMethod)
    {
      case RetrievalMethod::RoundRobin:
        m_retrievalFunction = std::bind(&TransmitBuffer::RoundRobinRetrieval, this);
        break;
      case RetrievalMethod::AckPriority:
        m_retrievalFunction = std::bind(&TransmitBuffer::AckPriorityRetrieval, this);
        break;
      case RetrievalMethod::InOrder:
        m_retrievalFunction = std::bind(&TransmitBuffer::InOrderRetrieval, this);
        break;
    }
  } catch (const std::out_of_range& e)
  {
    NS_ABORT_MSG("Retrieval method " << retrievalMethod << " does not exist");
  }
}

void
TransmitBuffer::AddPacket(TransmitBuffer::QueueEntry queueEntry)
{
  if ((m_retrievalMethod == RetrievalMethod::InOrder) ||
      (queueEntry.packetType == PacketType::Data))
  {
    m_dataQueue.emplace(queueEntry);
    NS_LOG_INFO(Simulator::Now ().GetSeconds () << "s - Switch " << m_switchId <<
                ": Add packet to DATA transmit buffer. Packets in DATA buffer: " <<
                m_dataQueue.size() << " Packets in ACK buffer: " << m_ackQueue.size());
  }
  else if (queueEntry.packetType == PacketType::Ack)
  {
    m_ackQueue.emplace(queueEntry);
    NS_LOG_INFO(Simulator::Now ().GetSeconds () << "s - Switch " << m_switchId <<
                ": Add packet to ACK transmit buffer. Packets in DATA buffer: " <<
                m_dataQueue.size() << " Packets in ACK buffer: " << m_ackQueue.size());
  }
  else
  {
    NS_ABORT_MSG("Switch " << m_switchId << "Adding packet to transmit buffer failed.");
  }
}

std::pair<bool, TransmitBuffer::QueueEntry>
TransmitBuffer::RetrievePacket ()
{
  return m_retrievalFunction();
}

std::pair<bool, TransmitBuffer::QueueEntry>
TransmitBuffer::InOrderRetrieval()
{
  if (m_dataQueue.size() == 0)
    return std::make_pair(false, QueueEntry());

  auto retrievedQueueEntry = m_dataQueue.front ();
  m_dataQueue.pop ();

  NS_LOG_INFO(Simulator::Now ().GetSeconds () << "s: InOrderRetrieval - Packet retrieved from "
              "Switch " << m_switchId << " data buffer");

  return std::make_pair(true, retrievedQueueEntry);
}

std::pair<bool, TransmitBuffer::QueueEntry>
TransmitBuffer::RoundRobinRetrieval()
{
  if (m_ackQueue.size() > 0 && m_dataQueue.size() > 0) // Both queues have data to transmit
  {
    m_rrTransmitAckPacket = !m_rrTransmitAckPacket;

    if (m_rrTransmitAckPacket == true)
    {
      auto retrievedQueueEntry = m_ackQueue.front ();
      m_ackQueue.pop ();

      NS_LOG_INFO(Simulator::Now ().GetSeconds () << "s: RoundRobinRetrieval - "
                  "Both Queues have packets - Packet retrieved from Switch " << m_switchId <<
                  " ACK buffer");

      return std::make_pair(true, retrievedQueueEntry);
    }
    else
    {
      auto retrievedQueueEntry = m_dataQueue.front ();
      m_dataQueue.pop ();

      NS_LOG_INFO(Simulator::Now ().GetSeconds () << "s: RoundRobinRetrieval - "
                  "Both Queues have packets - Packet retrieved from Switch " << m_switchId <<
                  " DATA buffer");

      return std::make_pair(true, retrievedQueueEntry);
    }
  }
  else if (m_ackQueue.size() > 0 && m_dataQueue.size() == 0)
  {
    auto retrievedQueueEntry = m_ackQueue.front ();
    m_ackQueue.pop ();

    NS_LOG_INFO(Simulator::Now ().GetSeconds () << "s: RoundRobinRetrieval - "
                "No DATA packets available - Packet retrieved from Switch " << m_switchId <<
                " ACK buffer");

    return std::make_pair(true, retrievedQueueEntry);
  }
  else if (m_ackQueue.size() == 0 && m_dataQueue.size() > 0)
  {
    auto retrievedQueueEntry = m_dataQueue.front ();
    m_dataQueue.pop ();

    NS_LOG_INFO(Simulator::Now ().GetSeconds () << "s: RoundRobinRetrieval - "
                "No ACK packets available - Packet retrieved from Switch " << m_switchId <<
                " DATA buffer");

    return std::make_pair(true, retrievedQueueEntry);
  }

  return std::make_pair(false, TransmitBuffer::QueueEntry());
}

std::pair<bool, TransmitBuffer::QueueEntry>
TransmitBuffer::AckPriorityRetrieval()
{
  if (m_ackQueue.size() > 0)
  {
    auto retrievedQueueEntry = m_ackQueue.front ();
    m_ackQueue.pop ();

    NS_LOG_INFO(Simulator::Now ().GetSeconds () << "s: AckPriorityRetrieval - Packet retrieved from "
                "Switch " << m_switchId << " ACK buffer");

    return std::make_pair(true, retrievedQueueEntry);
  }
  else if (m_dataQueue.size() > 0)
  {
    auto retrievedQueueEntry = m_dataQueue.front ();
    m_dataQueue.pop ();

    NS_LOG_INFO(Simulator::Now ().GetSeconds () << "s: AckPriorityRetrieval - Packet retrieved from "
                "Switch " << m_switchId << " data buffer");

    return std::make_pair(true, retrievedQueueEntry);
  }

  return std::make_pair(false, TransmitBuffer::QueueEntry());
}
