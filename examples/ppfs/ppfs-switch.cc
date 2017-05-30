#include "ns3/abort.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/log.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/double.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/queue.h"

#include "ppfs-switch.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PpfsSwitch");

PpfsSwitch::PpfsSwitch () {}

PpfsSwitch::PpfsSwitch (uint32_t id, Ptr<Node> node) : SwitchDevice(id, node) {}

void
PpfsSwitch::InsertEntryInRoutingTable(uint32_t srcIpAddr, uint32_t dstIpAddr, uint16_t portNumber,
                                      char protocol, uint32_t flowId, uint32_t linkId,
                                      double flowRatio)
{
  FlowMatch currentFlow (srcIpAddr, dstIpAddr, portNumber, protocol);

  auto ret = m_linkNetDeviceTable.find(linkId);
  NS_ABORT_MSG_IF(ret == m_linkNetDeviceTable.end(), "Link " << linkId << " was not found in "<<
                  "the routing table of Switch with Id " << m_id);
  Ptr<NetDevice> forwardDevice = ret->second;

  NS_LOG_INFO("---------------------------------------------------------------");
  NS_LOG_INFO("Inserting entry in Switch " << m_id << " routing table.\n" << currentFlow);

  auto routingTableEntry = m_routingTable.find(currentFlow);
  if (routingTableEntry == m_routingTable.end()) // Route does not exist in table
    {
      FlowDetails flowDetails (flowId);
      flowDetails.forwardingActions.push_back(ForwardingAction(forwardDevice, flowRatio));
      m_routingTable.insert({currentFlow, flowDetails});

      NS_LOG_INFO(flowDetails.forwardingActions.back()); // Logging the entry
    }
  else // Route already exists in table
    {
      std::vector<ForwardingAction>& forwardActions = routingTableEntry->second.forwardingActions;
      ForwardingAction& lastAction = forwardActions.back();
      double currentSplitRatio = lastAction.splitRatio + flowRatio;
      forwardActions.push_back(ForwardingAction(forwardDevice, currentSplitRatio));

      for (auto& action : forwardActions) NS_LOG_INFO(action);
    }
}

void
PpfsSwitch::SetPacketHandlingMechanism()
{
  uint32_t numOfDevices = m_node->GetNDevices();
  NS_ASSERT(numOfDevices > 0);
  for (uint32_t currentDevice = 0; currentDevice < numOfDevices; ++currentDevice)
    {
      m_node->RegisterProtocolHandler(MakeCallback(&PpfsSwitch::ReceiveFromDevice, this), 0,
                                      m_node->GetDevice(currentDevice), false);
      // False flag means that promiscuous mode is disabled
    }
}

void
PpfsSwitch::ForwardPacket(ns3::Ptr<const ns3::Packet> packet, uint16_t protocol, const Address& dst)
{
  FlowMatch flow (ParsePacket(packet, protocol));

  auto ret = m_routingTable.find(flow);
  NS_ABORT_MSG_IF(ret == m_routingTable.end(), "Routing Table Miss\n" << flow);

  NS_LOG_INFO("  Switch " << m_id << ": Forwarding packet at " << Simulator::Now().GetSeconds()
              << "s");

  Ptr<NetDevice> forwardingPort (GetPort(ret->second.forwardingActions));

  uint32_t packetSizeInclP2pHdr (packet->GetSize()+2);
  LogLinkStatistics(forwardingPort, ret->second.flowId, packetSizeInclP2pHdr);
  bool sendSuccessful = forwardingPort->Send(packet->Copy(), dst, protocol);
  if (sendSuccessful == false) NS_LOG_INFO("WARNING: Packet Transmission failed");

  LogQueueEntries(forwardingPort); // Log the net device's queue details
}

void
PpfsSwitch::SetRandomNumberGenerator (uint32_t seed, uint32_t run)
{
  SeedManager::SetSeed (seed);
  SeedManager::SetRun (run);

  m_uniformRandomVariable = CreateObject<UniformRandomVariable>();

  m_uniformRandomVariable->SetAttribute("Min", DoubleValue(0.0));
  m_uniformRandomVariable->SetAttribute ("Max", DoubleValue (1.0));
}

void
PpfsSwitch::ReceiveFromDevice(Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                              uint16_t protocol, const Address &src, const Address &dst,
                              NetDevice::PacketType packetType)
{
  NS_LOG_INFO("  Switch " << m_id << ": Received a packet at "
              << Simulator::Now().GetSeconds() << "s");

  ForwardPacket(packet, protocol, dst); // Forward the packet
}

PpfsSwitch::FlowMatch
PpfsSwitch::ParsePacket (ns3::Ptr<const ns3::Packet> packet, uint16_t protocol)
{
  Ptr<Packet> recvPacket = packet->Copy (); // Copy the packet for parsing purposes
  FlowMatch flow;

  if (protocol == Ipv4L3Protocol::PROT_NUMBER) // Packet is IP
    {
      Ipv4Header ipHeader;
      uint8_t ipProtocol (0);

      if (recvPacket->PeekHeader(ipHeader)) // Parsing IP Header
        {
          ipProtocol = ipHeader.GetProtocol();
          flow.srcIpAddr = ipHeader.GetSource().Get();
          flow.dstIpAddr = ipHeader.GetDestination().Get();
          recvPacket->RemoveHeader(ipHeader); // Removing the IP header
        }

      if (ipProtocol == UdpL4Protocol::PROT_NUMBER) // UDP Packet
        {
          UdpHeader udpHeader;
          if (recvPacket->PeekHeader(udpHeader))
            {
              flow.portNumber = udpHeader.GetDestinationPort();
              flow.protocol = FlowMatch::Protocol::Udp;
            }
        }
      else if (ipProtocol == TcpL4Protocol::PROT_NUMBER) // TCP Packet
        {
          TcpHeader tcpHeader;
          if (recvPacket->PeekHeader(tcpHeader))
            {
              flow.portNumber = tcpHeader.GetDestinationPort();
              flow.protocol = FlowMatch::Protocol::Tcp;
            }
        }
      else
        NS_ABORT_MSG("Unknown packet type received. Packet Type " << ipProtocol);
    }
  else
    NS_ABORT_MSG("Non-IP Packet received. Protocol value " << protocol);

  return flow;
}

ns3::Ptr<ns3::NetDevice>
PpfsSwitch::GetPort (const std::vector<PpfsSwitch::ForwardingAction>& forwardActions)
{
  double randomNumber (GenerateRandomNumber()); // Generate random number
  double lhs = 0.0;
  double rhs = 0.0;

  typedef  std::vector<PpfsSwitch::ForwardingAction>::size_type sizeType;

  // Loop through the routing table until a match is found
  for (sizeType index = 0; index < forwardActions.size(); ++index)
    { // A match is found based on the equation (lhs <= value < rhs)
      lhs = (index == 0) ? 0.0 : forwardActions[index - 1].splitRatio;
      rhs = forwardActions[index].splitRatio;

      if ((lhs <= randomNumber) && (randomNumber < rhs))
        return forwardActions[index].port; // Match found

      /*
       * If we are at the end and of the forwarding table and no match has been found yet the packet
       * should be forwarded at the last forwarding slot. When such instance occur a log entry will
       * be output to ease debugging.
       */
      if (index == (forwardActions.size()-1))
        {
          NS_LOG_INFO("The random number " << randomNumber << " did not fit any slot in" <<
                      " the forwarding table.");
          return forwardActions[index].port;
        }
    }

  NS_FATAL_ERROR("Forwarding port number for packet not found.");
}

void
PpfsSwitch::LogLinkStatistics (ns3::Ptr<ns3::NetDevice> port, uint32_t flowId, uint32_t packetSize)
{
  auto linkRet = m_netDeviceLinkTable.find(port);
  NS_ABORT_MSG_IF(linkRet == m_netDeviceLinkTable.end(), "The Link connected to the net device"
                  "was not found");

  LinkFlowId linkFlowId (linkRet->second, flowId);

  auto linkStatRet = m_linkStatistics.find(linkFlowId);

  if (linkStatRet == m_linkStatistics.end()) // Link statistics entry not found, create it
    {
      LinkStatistic linkStatistic;
      linkStatistic.timeFirstTx = Simulator::Now();
      linkStatistic.timeLastTx = Simulator::Now();
      linkStatistic.packetsTransmitted++;
      linkStatistic.bytesTransmitted += packetSize;
      m_linkStatistics.insert({linkFlowId, linkStatistic});
    }
  else // Update the link statistics entry
    {
      LinkStatistic& linkStatistic = linkStatRet->second;
      linkStatistic.timeLastTx = Simulator::Now();
      linkStatistic.packetsTransmitted++;
      linkStatistic.bytesTransmitted += packetSize;
    }
}

double
PpfsSwitch::GenerateRandomNumber ()
{
  return m_uniformRandomVariable->GetValue();
}
