#include "transmit-queue.h"

bool
TransmitQueue::Enqueue (ns3::Ptr<ns3::Packet> packet)
{
  std::cout << "A packet is received" << std::endl;
  m_dataQueue.push (packet);
  return true;
}

ns3::Ptr<ns3::Packet>
TransmitQueue::Dequeue ()
{
  if (m_dataQueue.empty ())
    return 0;

  ns3::Ptr<ns3::Packet> packet = m_dataQueue.front ();
  m_dataQueue.pop ();
  return packet;
}

ns3::Ptr<ns3::Packet>
TransmitQueue::Remove ()
{
  std::cout << "Calling the remove function" << std::endl;
  return nullptr;
}

ns3::Ptr<const ns3::Packet>
TransmitQueue::Peek () const
{
  std::cout << "Calling the peek function" << std::endl;
  return nullptr;
}
