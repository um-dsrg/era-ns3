#include "transmit-buffer.h"

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

void
TransmitBuffer::AddPacket (ns3::Ptr<ns3::Packet> packet, PacketType type)
{
  // FIXME: Implement this function
}

ns3::Ptr<ns3::Packet>
TransmitBuffer::RetrievePacket ()
{
  // FIXME: Implement this function
  return nullptr;
}
