#include "mptcp-header.h"

using namespace ns3;

TypeId MptcpHeader::GetTypeId()
{
  static TypeId tid = TypeId("MptcpHeader").SetParent<Header>().AddConstructor<MptcpHeader>();
  return tid;
}

TypeId MptcpHeader::GetInstanceTypeId() const
{
  return GetTypeId();
}

uint32_t MptcpHeader::GetSerializedSize() const
{
  // This header contains a 64-bit unsigned integer that represents
  // the packet number.
  return 8;
}

void MptcpHeader::Serialize(Buffer::Iterator start) const
{
  start.WriteHtonU64(m_packetNumber);
}

uint32_t MptcpHeader::Deserialize(Buffer::Iterator start)
{
  m_packetNumber = start.ReadNtohU64();
  return 8; // The number of bytes consumed.
}

void MptcpHeader::Print(std::ostream &os) const
{
  os << "Packet number " << m_packetNumber;
}

void MptcpHeader::SetPacketNumber(uint64_t packetNumber)
{
  m_packetNumber = packetNumber;
}
