#include "ns3/log.h"
#include "ns3/simulator.h"

#include "receive-buffer.h"

NS_LOG_COMPONENT_DEFINE ("ReceiveBuffer");

using namespace ns3;

ReceiveBuffer::ReceiveBuffer(id_t switchId, uint64_t bufferSize) :
  m_switchId(switchId), m_bufferSize(bufferSize)
{}

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

    NS_LOG_INFO(Simulator::Now().GetSeconds() << "s: Switch: " << m_switchId << " packet ADDED to "
                "receive buffer. Packet size: " << packetSize << " Used capacity: " <<
                m_usedCapacity);
    return true;
  }
  else
  {
    m_numDroppedPackets++;
    NS_LOG_INFO(Simulator::Now().GetSeconds() << "s: Switch: " << m_switchId << " packet DROPPED "
                "at receive buffer. Packet size: " << packetSize << " Used capacity: " <<
                m_usedCapacity << " Num dropped packets: " << m_numDroppedPackets);
    return false;
  }
}

void
ReceiveBuffer::RemovePacket(Ptr<const Packet> packet)
{
  auto packetSize {packet->GetSize()};
  m_usedCapacity -= packetSize;

  NS_ABORT_MSG_IF(m_usedCapacity < 0, "Switch " << m_switchId << " negative used capacity. " <<
                  "Value: " << m_usedCapacity);

  NS_LOG_INFO(Simulator::Now().GetSeconds() << "s: Switch: " << m_switchId << " packet REMOVED " <<
              "from receive buffer. Packet size: " << packetSize << " Used capacity: " <<
              m_usedCapacity);
}
