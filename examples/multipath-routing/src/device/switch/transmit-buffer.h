#ifndef transmit_buffer_h
#define transmit_buffer_h

#include <queue>

#include "ns3/packet.h"

#include "../../definitions.h"

class TransmitBuffer
{
public:
  TransmitBuffer ();

  void AddPacket (ns3::Ptr<ns3::Packet> packet, PacketType type);
  ns3::Ptr<ns3::Packet> RetrievePacket ();

private:
  std::queue<ns3::Ptr<ns3::Packet>> m_ackQueue; /**< Queue storing ACK packets */
  std::queue<ns3::Ptr<ns3::Packet>> m_dataQueue; /**< Queue storing Data packets */
};

#endif /* transmit_buffer_h */
