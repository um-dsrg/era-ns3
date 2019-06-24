#ifndef buffer_h
#define buffer_h

#include <queue>

#include "ns3/packet.h"

#include "../../definitions.h"
#include "../../results-container.h"

class Buffer
{
public:
  /**
   * @brief Construct a new Buffer::Buffer object
   *
   * @param switchId      The Switch Id
   * @param bufferSize    The Buffer Size
   * @param resContainer  Reference to the Results container instance
   */
  Buffer (const id_t switchId, uint64_t bufferSize, ResultsContainer &resContainer);
  /**
   * @brief Add a packet to the buffer
   *
   * @param packet The packet to save in the buffer
   * @param protocol The packet's protocol
   * @return true If packet successfully added to the buffer
   * @return false If buffer is full and packet dropped
   */
  bool AddToBuffer (ns3::Ptr<const ns3::Packet> packet, const uint16_t protocol);
  // FIXME: Add function documentation here
  std::pair<bool, ns3::Ptr<ns3::Packet>> RetrieveFromBuffer ();

private:
  /**
   * @brief Parse a packet to determine whether it is an Acknowledgement packet
   * or not.
   *
   * @param packet The packet to parse
   * @param protocol The packet L3 protocol
   * @return true If the packet is an Acknowledgement
   * @return false Otherwise
   */
  bool AckPacket (ns3::Ptr<const ns3::Packet> packet, const uint16_t protocol);

  const id_t m_switchId; /**< The switch id using this buffer */

  const uint64_t m_bufferSize; /**< The number of bytes allowed to be stored in this buffer */
  uint64_t m_usedCapacity; /**< The number of bytes currently stored in the buffer */

  std::queue<ns3::Ptr<ns3::Packet>> m_ackPktsQueue; /**< The queue storing ACK packets */
  std::queue<ns3::Ptr<ns3::Packet>> m_dataPktsQueue; /**< The queue storing Data packets */

  ResultsContainer &m_resContainer;
};

#endif /* buffer_h */
