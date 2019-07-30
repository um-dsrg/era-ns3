#ifndef transmit_buffer_h
#define transmit_buffer_h

#include <map>
#include <queue>

#include "ns3/packet.h"

#include "../../definitions.h"

enum class RetrievalMethod {RoundRobin, AckPriority, InOrder};

class TransmitBuffer
{
public:
  TransmitBuffer (const std::string& retrievalMethod);

  void AddPacket (ns3::Ptr<ns3::Packet> packet, PacketType type);
  ns3::Ptr<ns3::Packet> RetrievePacket ();

private:
  std::queue<ns3::Ptr<ns3::Packet>> m_ackQueue; /**< Queue storing ACK packets */
  std::queue<ns3::Ptr<ns3::Packet>> m_dataQueue; /**< Queue storing Data packets */

  RetrievalMethod m_retrievalMethod;

  static const std::map<std::string, RetrievalMethod> RetreivalMethodMap;
};

#endif /* transmit_buffer_h */
