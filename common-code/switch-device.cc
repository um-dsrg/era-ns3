#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/icmpv4-l4-protocol.h"

#include "switch-device.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SwitchDevice");

void
SwitchDevice::InsertNetDevice (LinkId_t linkId, Ptr<NetDevice> device)
{
  auto ret = m_linkNetDeviceTable.insert ({linkId, device});
  NS_ABORT_MSG_IF (ret.second == false, "The Link ID " << linkId << " is already stored in node's "
                                                       << m_id << " Link->NetDevice map");

  m_netDeviceLinkTable.insert ({device, linkId});

  m_switchQueueResults.insert (
      {linkId, QueueResults ()}); // Pre-populating the map with empty values
}

Ptr<Queue<Packet>>
SwitchDevice::GetQueueFromLinkId (LinkId_t linkId) const
{
  auto ret = m_linkNetDeviceTable.find (linkId);
  NS_ABORT_MSG_IF (ret == m_linkNetDeviceTable.end (),
                   "The port connecting link id: " << linkId << " was not found");

  Ptr<NetDevice> port = ret->second;
  Ptr<PointToPointNetDevice> p2pDevice = port->GetObject<PointToPointNetDevice> ();
  return p2pDevice->GetQueue ();
}

const std::map<LinkId_t, SwitchDevice::QueueResults> &
SwitchDevice::GetQueueResults () const
{
  return m_switchQueueResults;
}

const std::map<SwitchDevice::LinkFlowId, LinkStatistic> &
SwitchDevice::GetLinkStatistics () const
{
  return m_linkStatistics;
}

SwitchDevice::SwitchDevice ()
{
}

SwitchDevice::SwitchDevice (NodeId_t id, Ptr<Node> node) : m_id (id), m_node (node)
{
}

SwitchDevice::~SwitchDevice ()
{
}

void
SwitchDevice::LogLinkStatistics (Ptr<NetDevice> port, FlowId_t flowId, uint32_t packetSize)
{
  auto linkRet = m_netDeviceLinkTable.find (port);
  NS_ABORT_MSG_IF (linkRet == m_netDeviceLinkTable.end (), "The Link connected to the net device"
                                                           "was not found");

  LinkFlowId linkFlowId (linkRet->second, flowId);

  auto linkStatRet = m_linkStatistics.find (linkFlowId);

  if (linkStatRet == m_linkStatistics.end ()) // Link statistics entry not found, create it
    {
      LinkStatistic linkStatistic;
      linkStatistic.timeFirstTx = Simulator::Now ();
      linkStatistic.timeLastTx = Simulator::Now ();
      linkStatistic.packetsTransmitted++;
      linkStatistic.bytesTransmitted += packetSize;
      m_linkStatistics.insert ({linkFlowId, linkStatistic});
    }
  else // Update the link statistics entry
    {
      LinkStatistic &linkStatistic = linkStatRet->second;
      linkStatistic.timeLastTx = Simulator::Now ();
      linkStatistic.packetsTransmitted++;
      linkStatistic.bytesTransmitted += packetSize;
    }
}

Flow
SwitchDevice::ParsePacket (Ptr<const Packet> packet, uint16_t protocol, bool allowIcmpPackets)
{
  Ptr<Packet> recvPacket = packet->Copy (); // Copy the packet for parsing purposes
  Flow flow;

  if (protocol == Ipv4L3Protocol::PROT_NUMBER) // Packet is IP
    {
      Ipv4Header ipHeader;
      uint8_t ipProtocol (0);

      if (recvPacket->PeekHeader (ipHeader)) // Parsing IP Header
        {
          ipProtocol = ipHeader.GetProtocol ();
          flow.srcIpAddr = ipHeader.GetSource ().Get ();
          flow.dstIpAddr = ipHeader.GetDestination ().Get ();
          recvPacket->RemoveHeader (ipHeader); // Removing the IP header
        }

      if (ipProtocol == UdpL4Protocol::PROT_NUMBER) // UDP Packet
        {
          UdpHeader udpHeader;
          if (recvPacket->PeekHeader (udpHeader))
            {
              flow.dstPortNumber = udpHeader.GetDestinationPort ();
              flow.SetProtocol (Flow::Protocol::Udp);
            }
        }
      else if (ipProtocol == TcpL4Protocol::PROT_NUMBER) // TCP Packet
        {
          TcpHeader tcpHeader;
          if (recvPacket->PeekHeader (tcpHeader))
            {
              flow.dstPortNumber = tcpHeader.GetDestinationPort ();
              flow.SetProtocol (Flow::Protocol::Tcp);
            }
        }
      else if (ipProtocol == Icmpv4L4Protocol::PROT_NUMBER && allowIcmpPackets) // ICMP Packet
        {
          Icmpv4Header icmpHeader;
          if (recvPacket->PeekHeader (icmpHeader))
            {
              flow.SetProtocol (Flow::Protocol::Icmp);
              switch (icmpHeader.GetType ())
                {
                case Icmpv4Header::Type_e::ICMPV4_ECHO:
                  NS_LOG_INFO ("ICMP Echo message received at Switch " << m_id);
                  break;
                case Icmpv4Header::Type_e::ICMPV4_ECHO_REPLY:
                  NS_LOG_INFO ("ICMP Echo reply message received at Switch " << m_id);
                  break;
                case Icmpv4Header::Type_e::ICMPV4_DEST_UNREACH:
                  NS_LOG_INFO ("ICMP Destination Unreachable message received at Switch " << m_id);
                  break;
                case Icmpv4Header::Type_e::ICMPV4_TIME_EXCEEDED:
                  NS_LOG_INFO ("ICMP Time exceeded message received at Switch " << m_id);
                  break;
                default:
                  NS_ABORT_MSG ("ICMP unidentified message received at Switch " << m_id);
                  break;
                }
            }
        }
      else
        NS_ABORT_MSG ("Unknown packet type received. Packet Type " << std::to_string (ipProtocol));
    }
  else
    NS_ABORT_MSG ("Non-IP Packet received. Protocol value " << protocol);

  return flow;
}

void
SwitchDevice::LogQueueEntries (Ptr<NetDevice> port)
{
  Ptr<PointToPointNetDevice> p2pDevice = port->GetObject<PointToPointNetDevice> ();
  Ptr<Queue<Packet>> queue = p2pDevice->GetQueue ();

  uint32_t numOfPackets (queue->GetNPackets ());
  uint32_t numOfBytes (queue->GetNBytes ());

  auto ret = m_netDeviceLinkTable.find (port);
  NS_ABORT_MSG_IF (ret == m_netDeviceLinkTable.end (), "LinkId not found from NetDevice");

  QueueResults &queueResults (m_switchQueueResults[ret->second /*link id*/]);

  if (numOfPackets > queueResults.peakNumOfPackets)
    queueResults.peakNumOfPackets = numOfPackets;
  if (numOfBytes > queueResults.peakNumOfBytes)
    queueResults.peakNumOfBytes = numOfBytes;
}
