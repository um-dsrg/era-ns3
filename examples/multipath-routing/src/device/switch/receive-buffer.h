#ifndef receive_buffer_h
#define receive_buffer_h

#include "ns3/packet.h"

#include "../../definitions.h"

class ReceiveBuffer
{
public:
  ReceiveBuffer(id_t switchId, uint64_t bufferSize);

  bool AddPacket(ns3::Ptr<const ns3::Packet> packet);
  void RemovePacket(ns3::Ptr<const ns3::Packet> packet);

private:
  const id_t m_switchId;

  /* Buffer related variables */
  uint64_t m_usedCapacity = 0;
  const uint64_t m_bufferSize;
  uint64_t m_numDroppedPackets = 0;
};

#endif /* receive_buffer_h */
