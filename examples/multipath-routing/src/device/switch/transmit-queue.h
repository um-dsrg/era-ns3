#ifndef transmit_queue_h
#define transmit_queue_h

#include <queue>
#include <functional>

#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/packet.h"

#include "../../definitions.h"

class TransmitQueue : public ns3::Queue<ns3::Packet>
{
public:
  TransmitQueue (const std::string &retrievalMethod, id_t switchId);

  bool Enqueue (ns3::Ptr<ns3::Packet> packet) override;
  ns3::Ptr<ns3::Packet> Dequeue () override;
  ns3::Ptr<ns3::Packet> Remove () override;
  ns3::Ptr<const ns3::Packet> Peek () const override;

private:
  PacketType GetPacketType (ns3::Ptr<const ns3::Packet> packet) const;

  ns3::Ptr<ns3::Packet> GetAckPacket ();
  ns3::Ptr<ns3::Packet> GetDataPacket ();

  /* Queue Retrieval Methods */
  ns3::Ptr<ns3::Packet> InOrderRetrieval ();
  ns3::Ptr<ns3::Packet> RoundRobinRetrieval ();
  ns3::Ptr<ns3::Packet> AckPriorityRetrieval ();

  /**
   * When True, transmit a packet from the ACK queue. This is used by the Round robin retrieval
   * method to track the last queue used.
   */
  bool m_rrTransmitAckPacket = false;

  id_t m_switchId;
  std::queue<ns3::Ptr<ns3::Packet>> m_ackQueue;
  std::queue<ns3::Ptr<ns3::Packet>> m_dataQueue;

  /* Retrieval functionality */
  std::function<ns3::Ptr<ns3::Packet> (void)> m_retrievalFunction;

  enum class RetrievalMethod { RoundRobin, AckPriority, InOrder };
  RetrievalMethod m_retrievalMethod;

  ns3::NS_LOG_TEMPLATE_DECLARE;
};

#endif /* transmit_queue_h */
