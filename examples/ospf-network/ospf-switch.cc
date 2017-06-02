#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ppp-header.h"

#include "ospf-switch.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("OspfSwitch");

OspfSwitch::OspfSwitch ()
{
}

OspfSwitch::OspfSwitch (uint32_t id, Ptr<Node> node) :
    SwitchDevice (id, node)
{
}

void
OspfSwitch::SetPacketHandlingMechanism ()
{
  for (auto & linkNetDevice : m_linkNetDeviceTable)
    {
      linkNetDevice.second->TraceConnect ("PhyTxBegin", std::to_string (linkNetDevice.first),
                                          MakeCallback (&OspfSwitch::LogPacketTransmission, this));
    }
}

void
OspfSwitch::InsertEntryInRoutingTable(uint32_t srcIpAddr, uint32_t dstIpAddr, uint16_t portNumber,
                                      char protocol, uint32_t flowId)
{
  FlowMatch flow (srcIpAddr, dstIpAddr, portNumber, protocol);

  auto ret = m_routingTable.find (flow);

  NS_ABORT_MSG_IF (ret != m_routingTable.end (), "Trying to insert duplicate entry in OSPF switch");

  m_routingTable.insert ({flow, flowId});
}

void
OspfSwitch::LogPacketTransmission (std::string context, ns3::Ptr<const ns3::Packet> packet)
{
  NS_LOG_INFO("  Switch " << m_id << ": Received a packet (" << packet->GetSize() << "bytes) at "
              << Simulator::Now().GetSeconds() << "s");

  LinkId_t linkId (std::stoul(context));
  auto linkNetDevice = m_linkNetDeviceTable.find(linkId);
  NS_ABORT_MSG_IF(linkNetDevice == m_linkNetDeviceTable.end(), "The net device with link id "
                  << linkId << " was not found");

  // Remove the packet's Point To Point (PPP) header before passing it to the parse packet function.
  Ptr<Packet> recvPacket = packet->Copy (); // Copy the packet for parsing purposes
  PppHeader pppHeader;

  NS_ABORT_MSG_IF((recvPacket->PeekHeader(pppHeader) == 0), "The PPP Header was not found");
  recvPacket->RemoveHeader(pppHeader);

  FlowMatch flow (ParsePacket (recvPacket, Ipv4L3Protocol::PROT_NUMBER));

  auto tableMatch = m_routingTable.find(flow);
  NS_ABORT_MSG_IF(tableMatch == m_routingTable.end(), "Routing Table Miss for flow\n" << flow);

  LogQueueEntries(linkNetDevice->second);
  LogLinkStatistics(linkNetDevice->second, tableMatch->second, packet->GetSize());
}
