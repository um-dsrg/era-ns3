#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/simulator.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/ppp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/point-to-point-net-device.h"

#include "switch-base.h"
#include "../../results-container.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SwitchBase");

/********************************************/
/* RtFlow                                   */
/********************************************/

RtFlow::RtFlow (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort, portNum_t dstPort,
                FlowProtocol protocol)
    : srcIp{srcIp}, dstIp{dstIp}, srcPort{srcPort}, dstPort{dstPort}, protocol{protocol}
{
}

bool
RtFlow::operator< (const RtFlow &other) const
{
  if (srcIp == other.srcIp)
    {
      if (dstIp == other.dstIp)
        {
          if (srcPort == other.srcPort)
            {
              return dstPort < other.dstPort;
            }
          else
            {
              return srcPort < other.srcPort;
            }
        }
      else
        {
          return dstIp < other.dstIp;
        }
    }
  else
    {
      return srcIp < other.srcIp;
    }
}

std::ostream &
operator<< (std::ostream &os, const RtFlow &flow)
{
  os << "Protocol: " << static_cast<char> (flow.protocol) << " | "
     << "Source IP: " << flow.srcIp << " Destination IP: " << flow.dstIp
     << " | Source Port: " << flow.srcPort << " Destination Port: " << flow.dstPort;
  return os;
}

/********************************************/
/* SwitchBase                               */
/********************************************/

SwitchBase::SwitchBase (id_t id, uint64_t switchBufferSize,
                        const std::string& txBufferRetrievalMethod)
    : CustomDevice (id),
      m_receiveBuffer (id, switchBufferSize),
      m_txBufferRetrievalMethod(txBufferRetrievalMethod)
{
}

SwitchBase::~SwitchBase ()
{
  for (auto& netDevTxBufferPair : m_netDevToTxBuffer)
  {
    delete netDevTxBufferPair.second;
    netDevTxBufferPair.second = nullptr;
  }
}

uint64_t
SwitchBase::GetNumDroppedPackets() const
{
  return m_receiveBuffer.GetNumDroppedPackets();
}

void
SwitchBase::InstallTransmitBuffers()
{
  uint32_t numOfDevices = m_node->GetNDevices ();
  NS_ASSERT (numOfDevices > 0);

  for (uint32_t deviceIndex = 0; deviceIndex < numOfDevices; ++deviceIndex)
  {
    auto netDevice = m_node->GetDevice (deviceIndex);
    m_netDevBusy.emplace(netDevice, false); // Initially, all switch net devices are idle
    m_netDevToTxBuffer.emplace(netDevice, new TransmitBuffer(m_txBufferRetrievalMethod, m_id));

    // Store the relationship between the device index and the device
    NS_ABORT_MSG_IF(deviceIndex != netDevice->GetIfIndex(),
                    "The Device index and IfIndex do not match");
    m_indexToNetDev.emplace(std::to_string(deviceIndex), netDevice);
  }
}

void
SwitchBase::ReconcileSplitRatios ()
{
  return; // NOTE: This function is only used by the PPFS Switch
}

void
SwitchBase::EnablePacketTransmissionCompletionTrace ()
{
  uint32_t numOfDevices = m_node->GetNDevices ();
  NS_ASSERT (numOfDevices > 0);

  for (uint32_t deviceIndex = 0; deviceIndex < numOfDevices; ++deviceIndex)
    {
      auto netDevice = m_node->GetDevice (deviceIndex);
      netDevice->TraceConnect("PhyTxEnd", std::to_string(netDevice->GetIfIndex()),
                              MakeCallback(&SwitchBase::PacketTransmitted, this));
    }
}

void
SwitchBase::AddPacketToTransmitBuffer (Ptr<NetDevice> netDevice,
                                       TransmitBuffer::QueueEntry queueEntry)
{
  try
  {
    TransmitBuffer* transmitBuffer = m_netDevToTxBuffer.at(netDevice);
    transmitBuffer->AddPacket(queueEntry);
  }
  catch (const std::out_of_range &oor)
  {
    NS_ABORT_MSG ("Transmit Buffer not found on Switch " << m_id);
  }
}

void
SwitchBase::TransmitPacket(Ptr<NetDevice> netDevice)
{
  try
  {
    if (m_netDevBusy.at(netDevice) == true)
      return;

    TransmitBuffer* transmitBuffer = m_netDevToTxBuffer.at(netDevice);

    std::pair<bool, TransmitBuffer::QueueEntry> txBufferEntry = transmitBuffer->RetrievePacket();

    if (txBufferEntry.first == true)
    {
      auto queueEntry = &txBufferEntry.second;

      auto sendSuccess = netDevice->Send (queueEntry->packet->Copy(), queueEntry->dst,
                                          queueEntry->protocol);

      m_netDevBusy.at(netDevice) = true;

      NS_ABORT_MSG_IF (!sendSuccess, "Switch " << m_id << " failed to forward packet");
      NS_LOG_INFO (Simulator::Now ().GetSeconds () << "s: Switch " << m_id <<
                   " started transmitting a packet");
    }
    else
    {
      NS_LOG_INFO (Simulator::Now ().GetSeconds () << "s: Switch " << m_id <<
                   " transmit buffer empty");
    }
  }
  catch (const std::out_of_range &oor)
  {
    NS_ABORT_MSG ("Transmit Buffer not found on Switch " << m_id);
  }
}

void
SwitchBase::PacketTransmitted (std::string deviceIndex, ns3::Ptr<const ns3::Packet> packet)
{
  /*
    NOTE
    ----
    The packet in this function includes the Point to Point header size, whereas the function that
    is used by the switches does not include the header. Therefore, the packet size here is
    subtracted by the PPP header size to cater for this difference.
  */
  auto packetSize = packet->GetSize() - PppHeader().GetSerializedSize();
  NS_LOG_INFO (Simulator::Now ().GetSeconds ()
               << "s: Switch " << m_id << " finished transmission of a packet of size "
               << packetSize << " bytes");

  m_receiveBuffer.RemovePacket(packetSize);

  try
  {
    auto netDevice = m_indexToNetDev.at(deviceIndex);
    m_netDevBusy.at(netDevice) = false;
    Simulator::Schedule (Time("0"), &SwitchBase::TransmitPacket, this, netDevice);
  }
  catch (const std::out_of_range &oor)
  {
    NS_ABORT_MSG ("Net device with index " << deviceIndex << " not found on Switch " << m_id);
  }
}

std::pair<PacketType, RtFlow>
SwitchBase::ExtractFlowFromPacket (Ptr<const Packet> packet, uint16_t protocol)
{
  Ptr<Packet> receivedPacket = packet->Copy (); // Copy the packet for parsing purposes
  RtFlow flow;
  PacketType packetType {PacketType::Data};

  if (protocol == Ipv4L3Protocol::PROT_NUMBER)
    {
      Ipv4Header ipHeader;
      uint8_t ipProtocol (0);

      if (receivedPacket->PeekHeader (ipHeader))
        { // Extract source and destination IP
          ipProtocol = ipHeader.GetProtocol ();
          flow.srcIp = ipHeader.GetSource ().Get ();
          flow.dstIp = ipHeader.GetDestination ().Get ();
          receivedPacket->RemoveHeader (ipHeader); // Removing the IP header
        }

      if (ipProtocol == UdpL4Protocol::PROT_NUMBER)
        { // UDP Packet
          UdpHeader udpHeader;
          if (receivedPacket->PeekHeader (udpHeader))
            {
              flow.srcPort = udpHeader.GetSourcePort ();
              flow.dstPort = udpHeader.GetDestinationPort ();
              flow.protocol = FlowProtocol::Udp;
            }
        }
      else if (ipProtocol == TcpL4Protocol::PROT_NUMBER)
        { // TCP Packet
          TcpHeader tcpHeader;
          if (receivedPacket->PeekHeader (tcpHeader))
            {
              flow.srcPort = tcpHeader.GetSourcePort ();
              flow.dstPort = tcpHeader.GetDestinationPort ();
              flow.protocol = FlowProtocol::Tcp;
              receivedPacket->RemoveHeader(tcpHeader);
            }

            if (receivedPacket->GetSize() == 0)
              packetType = PacketType::Ack;
        }
      else if (ipProtocol == Icmpv4L4Protocol::PROT_NUMBER)
        { // ICMP Packet
          Icmpv4Header icmpHeader;
          if (receivedPacket->PeekHeader (icmpHeader))
            {
              flow.protocol = FlowProtocol::Icmp;
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
        {
          NS_ABORT_MSG ("Unknown packet type received. Packet Type "
                        << std::to_string (ipProtocol));
        }
    }
  else
    {
      NS_ABORT_MSG ("Non-IP Packet received. Protocol value " << protocol);
    }

  return std::make_pair(packetType, flow);
}
