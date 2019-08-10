#ifndef transmit_queue_h
#define transmit_queue_h

#include <queue>

#include "ns3/queue.h"
#include "ns3/packet.h"

class TransmitQueue : public ns3::Queue<ns3::Packet>
{
public:
  bool Enqueue (ns3::Ptr<ns3::Packet> packet);
  ns3::Ptr<ns3::Packet> Dequeue ();
  ns3::Ptr<ns3::Packet> Remove ();
  ns3::Ptr<const ns3::Packet> Peek () const;

private:
  std::queue<ns3::Ptr<ns3::Packet>> m_dataQueue;
};

#endif /* transmit_queue_h */
