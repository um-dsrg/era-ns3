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
  struct QueueEntry
  {
    QueueEntry () = default;

    QueueEntry(uint16_t protocol, PacketType packetType, const ns3::Address dst,
               ns3::Ptr<const ns3::Packet> packet):
      protocol(protocol), packetType(packetType), dst(dst), packet(packet)
      {}

    uint16_t protocol = 0;
    PacketType packetType;
    const ns3::Address dst;
    ns3::Ptr<const ns3::Packet> packet = nullptr;
  };

  TransmitBuffer (const std::string& retrievalMethod, id_t switchId);

  void AddPacket (QueueEntry queueEntry);
  std::pair<bool, QueueEntry> RetrievePacket ();

private:
  /* Queue Retrieval Methods */
  std::pair<bool, QueueEntry> InOrderRetrieval();
  std::pair<bool, QueueEntry> RoundRobinRetrieval();
  std::pair<bool, QueueEntry> AckPriorityRetrieval();

  std::queue<QueueEntry> m_ackQueue; /**< Queue storing ACK packets */
  std::queue<QueueEntry> m_dataQueue; /**< Queue storing Data packets */

  /**
   * When True, transmit a packet from the ACK queue. This is used by the Round robin retrieval
   * method to track the last queue used.
   */
  bool m_rrTransmitAckPacket = false;

  const id_t m_switchId;
  RetrievalMethod m_retrievalMethod;
  std::function<std::pair<bool, QueueEntry>(void)> m_retrievalFunction;

  static const std::map<std::string, RetrievalMethod> RetreivalMethodMap;
};

#endif /* transmit_buffer_h */
