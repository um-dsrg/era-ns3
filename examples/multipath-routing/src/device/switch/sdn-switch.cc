#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/abort.h"
#include "ns3/double.h"
#include "ns3/ppp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/point-to-point-net-device.h"

#include "sdn-switch.h"
#include "../../results-container.h"
#include "../../random-generator-manager.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SdnSwitch");

SdnSwitch::SdnSwitch (id_t id, uint64_t switchBufferSize) : SwitchBase (id, switchBufferSize)
{
}

void
SdnSwitch::AddEntryToRoutingTable (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort,
                                   portNum_t dstPort, FlowProtocol protocol,
                                   ns3::Ptr<ns3::NetDevice> forwardingPort, splitRatio_t splitRatio)
{
  RtFlow rtFlow{srcIp, dstIp, srcPort, dstPort, protocol};
  auto ret = m_routingTable.emplace (rtFlow, forwardingPort);
  NS_ABORT_MSG_IF (ret.second == false, "Unable to add routing table entry in Switch " << m_id);

  NS_LOG_INFO ("Add entry in SDN Switch " << m_id);
  NS_LOG_INFO (rtFlow);
}

void
SdnSwitch::SetPacketReception ()
{
  uint32_t numOfDevices = m_node->GetNDevices ();
  NS_ASSERT (numOfDevices > 0);
  for (uint32_t currentDevice = 0; currentDevice < numOfDevices; ++currentDevice)
    {
      m_node->RegisterProtocolHandler (MakeCallback (&SdnSwitch::PacketReceived, this),
                                       /*all protocols*/ 0, m_node->GetDevice (currentDevice),
                                       /*disable promiscuous mode*/ false);
    }
}

void
SdnSwitch::PacketReceived (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                           const Address &src, const Address &dst, NetDevice::PacketType packetType)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds () << "s: Switch " << m_id << " received a packet");

  if (m_receiveBuffer.AddPacket (packet) == false)
    return;

  RtFlow parsedFlow;

  parsedFlow = ExtractFlowFromPacket (packet, protocol);
  NS_LOG_INFO ("  " << parsedFlow);

  try
    {
      Ptr<NetDevice> forwardingNetDevice;
      forwardingNetDevice = m_routingTable.at (parsedFlow);
      TransmitPacket (forwardingNetDevice, packet, dst, protocol);
  } catch (const std::out_of_range &oor)
    {
      NS_ABORT_MSG ("Routing table Miss on Switch " << m_id << ". Flow: \n" << parsedFlow);
  }
}
