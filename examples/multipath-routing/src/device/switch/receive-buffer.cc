#include "ns3/log.h"
#include "ns3/simulator.h"

#include "receive-buffer.h"

NS_LOG_COMPONENT_DEFINE ("ReceiveBuffer");

using namespace ns3;

ReceiveBuffer::ReceiveBuffer(id_t switchId, uint64_t bufferSize) :
  m_switchId(switchId), m_bufferSize(bufferSize)
{}

uint64_t
ReceiveBuffer::GetNumDroppedPackets() const
{
  return m_numDroppedPackets;
}

bool
ReceiveBuffer::AddPacket(Ptr<const Packet> packet)
{
  auto packetSize {packet->GetSize()};

  if ((packetSize + m_usedCapacity) <= m_bufferSize)
  {
    m_usedCapacity += packetSize;

    NS_ABORT_MSG_IF(m_usedCapacity > m_bufferSize, "Switch " << m_switchId << " used capacity " <<
                    "exceeded buffer size. Used capacity: " << m_usedCapacity <<
                    " Buffer size: " << m_bufferSize);

    NS_LOG_INFO(Simulator::Now().GetSeconds() << "s - Switch " << m_switchId << ": Packet ADDED to "
                "receive buffer. Packet size: " << packetSize << " Used capacity: " <<
                m_usedCapacity << " Buffer capacity: " << m_bufferSize);
    return true;
  }
  else
  {
    m_numDroppedPackets++;
    NS_LOG_INFO(Simulator::Now().GetSeconds() << "s - Switch " << m_switchId << ": Packet DROPPED "
                "at receive buffer. Packet size: " << packetSize << " Used capacity: " <<
                m_usedCapacity << " Num dropped packets: " << m_numDroppedPackets);
    return false;
  }
}

void
ReceiveBuffer::RemovePacket(uint32_t packetSize)
{
  NS_ABORT_MSG_IF(packetSize > m_usedCapacity, "Switch " << m_switchId << " negative used capacity. " <<
                  "Packet Size: " << packetSize << " Used Capacity: " << m_usedCapacity <<
                  " Buffer capacity: " << m_bufferSize);

  m_usedCapacity -= packetSize;

  NS_LOG_INFO(Simulator::Now().GetSeconds() << "s - Switch " << m_switchId << ": Packet REMOVED " <<
              "from receive buffer. Packet size: " << packetSize << " Used capacity: " <<
              m_usedCapacity << " Buffer capacity: " << m_bufferSize);
}
