#ifndef switch_base_h
#define switch_base_h

#include <map>
#include <unordered_map>

#include "ns3/packet.h"

#include "receive-buffer.h"
#include "../custom-device.h"
#include "../../definitions.h"

/**
 * @brief Represents the Flow entry in the Routing Table
 */
struct RtFlow
{
  uint32_t srcIp{0}; /**< Source IP Address. */
  uint32_t dstIp{0}; /**< Destination IP Address. */
  portNum_t srcPort{0}; /**< Source Port. */
  portNum_t dstPort{0}; /**< Destination Port. */
  FlowProtocol protocol{FlowProtocol::Undefined}; /**< Flow Protocol. */

  RtFlow () = default;
  RtFlow (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort, portNum_t dstPort,
          FlowProtocol protocol);

  bool operator< (const RtFlow &other) const;
  friend std::ostream &operator<< (std::ostream &os, const RtFlow &flow);
};

class SwitchBase : public CustomDevice
{
public:
  SwitchBase (id_t id, uint64_t switchBufferSize);
  virtual ~SwitchBase ();

  uint64_t GetNumDroppedPackets () const;

  virtual void ReconcileSplitRatios ();
  void EnablePacketTransmissionCompletionTrace ();

  virtual void SetPacketReception () = 0;
  virtual void AddEntryToRoutingTable (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort,
                                       portNum_t dstPort, FlowProtocol protocol,
                                       ns3::Ptr<ns3::NetDevice> forwardingPort,
                                       splitRatio_t splitRatio) = 0;

protected:
  void TransmitPacket (ns3::Ptr<ns3::NetDevice> forwardingNetDevice);

  void PacketTransmitted (std::string deviceIndex, ns3::Ptr<const ns3::Packet> packet);
  RtFlow ExtractFlowFromPacket (ns3::Ptr<const ns3::Packet> packet, uint16_t protocol);

  virtual void PacketReceived (ns3::Ptr<ns3::NetDevice> incomingPort,
                               ns3::Ptr<const ns3::Packet> packet, uint16_t protocol,
                               const ns3::Address &src, const ns3::Address &dst,
                               ns3::NetDevice::PacketType ndPacketType) = 0;

  ReceiveBuffer m_receiveBuffer;
};

#endif /* switch_base_h */
