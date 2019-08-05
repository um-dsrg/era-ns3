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
        std::cout << "Hello" << std::endl;
        break;
      case RetrievalMethod::AckPriority:
        std::cout << "Hello" << std::endl;
        break;
      case RetrievalMethod::InOrder:
        m_retrievalFunction = std::bind(&TransmitBuffer::InOrderRetrieval, this);
        break;
    }

  } catch (const std::out_of_range& e)
  {
    NS_ABORT_MSG("Retrieval method does not exist. Method: " << retrievalMethod);
  }
}

void
TransmitBuffer::AddPacket(TransmitBuffer::QueueEntry queueEntry)
{
  if ((m_retrievalMethod == RetrievalMethod::InOrder) ||
      (queueEntry.packetType == PacketType::Data))
  {
    m_dataQueue.emplace(queueEntry);
    NS_LOG_INFO(Simulator::Now ().GetSeconds () << "s: Add DATA packet to Switch " << m_switchId <<
                " transmit buffer");
  }
  else if (queueEntry.packetType == PacketType::Ack)
  {
    m_ackQueue.emplace(queueEntry);
    NS_LOG_INFO(Simulator::Now ().GetSeconds () << "s: Add ACK packet to Switch " << m_switchId <<
                " transmit buffer");
  }
  else
  {
    NS_ABORT_MSG("Adding packet to transmit buffer failed.");
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
