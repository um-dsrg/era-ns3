#ifndef transmit_buffer_h
#define transmit_buffer_h

#include <map>
#include <queue>
#include <functional>

#include "ns3/packet.h"

#include "../../definitions.h"

enum class RetrievalMethod {RoundRobin, AckPriority, InOrder};

class TransmitBuffer
{
public:
  TransmitBuffer (const std::string& retrievalMethod, id_t switchId);

  void AddPacket (ns3::Ptr<ns3::Packet> packet, PacketType type);
  std::pair<bool, ns3::Ptr<ns3::Packet>> RetrievePacket ();

private:
  std::pair<bool, ns3::Ptr<ns3::Packet>> InOrderRetrieval();

  const id_t m_switchId;

  std::queue<ns3::Ptr<ns3::Packet>> m_ackQueue; /**< Queue storing ACK packets */
  std::queue<ns3::Ptr<ns3::Packet>> m_dataQueue; /**< Queue storing Data packets */

  RetrievalMethod m_retrievalMethod;
  std::function<std::pair<bool, ns3::Ptr<ns3::Packet>>(void)> m_retrievalFunction;

  static const std::map<std::string, RetrievalMethod> RetreivalMethodMap;
};

#endif /* transmit_buffer_h */
