#ifndef PPFS_SWITCH_H
#define PPFS_SWITCH_H

#include <map>

#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/queue.h"

#include "../common-code/switch-base.h"

class PpfsSwitch : public SwitchBase
{
public:
  PpfsSwitch () = default;
  PpfsSwitch (id_t id);

  void AddEntryToRoutingTable (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort, portNum_t dstPort,
                               FlowProtocol protocol, ns3::Ptr<ns3::NetDevice> forwardingPort);

  void SetPacketReception ();

  // const uint32_t GetSeed () const;
  // const uint32_t GetRun () const;

  // void InsertEntryInRoutingTable (uint32_t srcIpAddr, uint32_t dstIpAddr, uint16_t portNumber,
  //                                 char protocol, FlowId_t flowId, LinkId_t linkId,
  //                                 double flowRatio);
  // void SetPacketHandlingMechanism ();
  // void ForwardPacket (ns3::Ptr<const ns3::Packet> packet, uint16_t protocol,
  //                     const ns3::Address &dst);
  // void SetRandomNumberGenerator ();

private:

  void PacketReceived(ns3::Ptr<ns3::NetDevice> incomingPort, ns3::Ptr<const ns3::Packet> packet,
                      uint16_t protocol, const ns3::Address &src, const ns3::Address &dst,
                      ns3::NetDevice::PacketType packetType);

  struct RtFlow
  {
    RtFlow () = default;

    RtFlow (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort, portNum_t dstPort,
            FlowProtocol protocol)
        : srcIp{srcIp}, dstIp{dstIp}, srcPort{srcPort}, dstPort{dstPort}, protocol{protocol}
    {
    }

    bool operator< (const RtFlow &other) const;

    uint32_t srcIp{0};
    uint32_t dstIp{0};
    portNum_t srcPort{0};
    portNum_t dstPort{0};
    FlowProtocol protocol {FlowProtocol::Undefined};
  };
  
  friend std::ostream& operator<<(std::ostream& os, const RtFlow& flow);

  RtFlow ExtractFlowFromPacket (ns3::Ptr<const ns3::Packet> packet, uint16_t protocol);

  std::map<RtFlow, ns3::Ptr<ns3::NetDevice>> m_routingTable;

  // void ReceiveFromDevice (ns3::Ptr<ns3::NetDevice> incomingPort, ns3::Ptr<const ns3::Packet> packet,
  //                         uint16_t protocol, ns3::Address const &src, ns3::Address const &dst,
  //                         ns3::NetDevice::PacketType packetType);

  // struct ForwardingAction
  // {
  //   ns3::Ptr<ns3::NetDevice> port;
  //   double splitRatio;

  //   ForwardingAction (ns3::Ptr<ns3::NetDevice> port, double splitRatio)
  //       : port (port), splitRatio (splitRatio)
  //   {
  //   }

  //   friend std::ostream &
  //   operator<< (std::ostream &output, ForwardingAction &value)
  //   {
  //     output << "Split Ratio " << value.splitRatio << "\n";
  //     output << "----------------------";
  //     return output;
  //   }
  // };

  // ns3::Ptr<ns3::NetDevice> GetPort (const std::vector<ForwardingAction> &forwardActions);
  // double GenerateRandomNumber ();

  //  * Routing Table
  //  *
  //  * Key -> Flow, Value -> Vector of Forwarding actions.
  //  * This map stores the routing table which is used to map a flow to
  //  * a set of forwarding actions.

  // std::map<Flow, std::vector<ForwardingAction>> m_routingTable;
  // ns3::Ptr<ns3::UniformRandomVariable> m_uniformRandomVariable; //!< Used for flow splitting

  // uint32_t m_seed;
  // uint32_t m_run;
};

#endif /* PPFS_SWITCH_H */
