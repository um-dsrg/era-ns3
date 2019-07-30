#include "ns3/log.h"
#include "ns3/simulator.h"

#include "transmit-buffer.h"

NS_LOG_COMPONENT_DEFINE ("TransmitBuffer");

using namespace ns3;

const std::map<std::string, RetrievalMethod> TransmitBuffer::RetreivalMethodMap =
  {{"RoundRobin", RetrievalMethod::RoundRobin},
   {"AckPriority", RetrievalMethod::AckPriority},
   {"InOrder", RetrievalMethod::InOrder}};

TransmitBuffer::TransmitBuffer(const std::string& retrievalMethod)
{
  try
  {
    m_retrievalMethod = TransmitBuffer::RetreivalMethodMap.at(retrievalMethod);
  } catch (const std::out_of_range& e)
  {
    NS_ABORT_MSG("Retrieval method does not exist. Method: " << retrievalMethod);
  }
}

void // FIXME: Add logging + notes and comments
TransmitBuffer::AddPacket (Ptr<Packet> packet, PacketType type)
{
  NS_LOG_INFO(Simulator::Now ().GetSeconds () << "s: Test Log message");

  if ((m_retrievalMethod == RetrievalMethod::InOrder) || (type == PacketType::Data))
  {
    m_dataQueue.emplace(packet->Copy());
  }
  else if (type == PacketType::Ack)
  {
    m_ackQueue.emplace(packet->Copy());
  }
  else
  {
    NS_ABORT_MSG("Adding packet to transmit buffer failed.");
  }
}

Ptr<Packet>
TransmitBuffer::RetrievePacket ()
{
  // FIXME: Implement this function
  return nullptr;
}
