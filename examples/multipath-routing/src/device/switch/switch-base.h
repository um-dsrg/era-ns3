#ifndef switch_base_h
#define switch_base_h

#include <map>

#include "ns3/packet.h"

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
  virtual ~SwitchBase ();

protected:
  SwitchBase (id_t id);

  virtual void SetPacketReception () = 0;
  virtual void AddEntryToRoutingTable (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort,
                                       portNum_t dstPort, FlowProtocol protocol,
                                       ns3::Ptr<ns3::NetDevice> forwardingPort,
                                       splitRatio_t splitRatio) = 0;
  virtual void ReconcileSplitRatios ();

  RtFlow ExtractFlowFromPacket (ns3::Ptr<const ns3::Packet> packet, uint16_t protocol);
};

#endif /* switch_base_h */
