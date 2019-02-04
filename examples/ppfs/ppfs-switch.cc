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

#include "../../common-code/random-generator-manager.h"

#include "ppfs-switch.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PpfsSwitch");

PpfsSwitch::PpfsSwitch (id_t id) : SwitchBase (id)
{
}

void
PpfsSwitch::AddEntryToRoutingTable (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort,
                                    portNum_t dstPort, FlowProtocol protocol,
                                    Ptr<NetDevice> forwardingPort)
{
  // FIXME May make this log more informative.
  NS_LOG_INFO ("Adding routing table entry in PPFS Switch " << m_id);

  auto ret =
      m_routingTable.emplace (RtFlow (srcIp, dstIp, srcPort, dstPort, protocol), forwardingPort);
  NS_ABORT_MSG_IF (ret.second == false, "Unable to add routing table entry in Switch " << m_id);
}

void PpfsSwitch::SetPacketReception()
{
  uint32_t numOfDevices = m_node->GetNDevices ();
  NS_ASSERT (numOfDevices > 0);
  for (uint32_t currentDevice = 0; currentDevice < numOfDevices; ++currentDevice)
  {
    m_node->RegisterProtocolHandler (MakeCallback (&PpfsSwitch::PacketReceived, this),
                                     /*all protocols*/ 0, m_node->GetDevice (currentDevice),
                                     /*disable promiscuous mode*/ false);
  }
}

void PpfsSwitch::PacketReceived(ns3::Ptr<ns3::NetDevice> incomingPort, ns3::Ptr<const ns3::Packet> packet, uint16_t protocol, const ns3::Address &src, const ns3::Address &dst, ns3::NetDevice::PacketType packetType)
{
  NS_LOG_UNCOND("A packet has been received by node " << m_id);
}

bool
PpfsSwitch::RtFlow::operator< (const RtFlow &other) const
{
  // FIXME Implement this
  return 1 < 5;
}

// const uint32_t
// PpfsSwitch::GetSeed () const
// {
//   return m_seed;
// }

// const uint32_t
// PpfsSwitch::GetRun () const
// {
//   return m_run;
// }

// void
// PpfsSwitch::InsertEntryInRoutingTable (uint32_t srcIpAddr, uint32_t dstIpAddr, uint16_t portNumber,
//                                        char protocol, FlowId_t flowId, LinkId_t linkId,
//                                        double flowRatio)
// {
//   Flow currentFlow (flowId, srcIpAddr, dstIpAddr, portNumber, protocol);

//   auto ret = m_linkNetDeviceTable.find (linkId);
//   NS_ABORT_MSG_IF (ret == m_linkNetDeviceTable.end (),
//                    "Link " << linkId << " was not found in "
//                            << "the routing table of Switch with Id " << m_id);
//   Ptr<NetDevice> forwardDevice = ret->second;

//   NS_LOG_INFO ("---------------------------------------------------------------");
//   NS_LOG_INFO ("Inserting entry in Switch " << m_id << " routing table.\n" << currentFlow);

//   auto routingTableEntry = m_routingTable.find (currentFlow);
//   if (routingTableEntry == m_routingTable.end ()) // Route does not exist in table
//     {
//       std::vector<ForwardingAction> forwardingActions;
//       forwardingActions.push_back (ForwardingAction (forwardDevice, flowRatio));
//       m_routingTable.insert ({currentFlow, forwardingActions});

//       NS_LOG_INFO (forwardingActions.back ()); // Logging the entry
//     }
//   else // Route already exists in table
//     {
//       std::vector<ForwardingAction> &forwardActions = routingTableEntry->second;
//       ForwardingAction &lastAction = forwardActions.back ();
//       double currentSplitRatio = lastAction.splitRatio + flowRatio;
//       forwardActions.push_back (ForwardingAction (forwardDevice, currentSplitRatio));

//       for (auto &action : forwardActions)
//         NS_LOG_INFO (action);
//     }
// }

// void
// PpfsSwitch::SetPacketHandlingMechanism ()
// {
//   uint32_t numOfDevices = m_node->GetNDevices ();
//   NS_ASSERT (numOfDevices > 0);
//   for (uint32_t currentDevice = 0; currentDevice < numOfDevices; ++currentDevice)
//     {
//       m_node->RegisterProtocolHandler (MakeCallback (&PpfsSwitch::ReceiveFromDevice, this),
//                                        /*all protocols*/ 0, m_node->GetDevice (currentDevice),
//                                        /*disable promiscuous mode*/ false);
//     }
// }

// void
// PpfsSwitch::ForwardPacket (ns3::Ptr<const ns3::Packet> packet, uint16_t protocol,
//                            const Address &dst)
// {
//   Flow flow (
//       ParsePacket (packet, protocol, false /*This simulation does not handle ICMP packets*/));

//   auto ret = m_routingTable.find (flow);
//   NS_ABORT_MSG_IF (ret == m_routingTable.end (), "Routing Table Miss\n" << flow);

//   NS_LOG_INFO ("  Switch " << m_id << ": Forwarding packet at " << Simulator::Now ().GetSeconds ()
//                            << "s");

//   Ptr<NetDevice> forwardingPort (GetPort (ret->second));
//   uint32_t packetSizeInclP2pHdr (packet->GetSize () + 2);

//   // To get the flow id use the one from the routing table *ret*, because the variable *flow* does
//   // not have a valid flow id set.
//   LogLinkStatistics (forwardingPort, ret->first.id, packetSizeInclP2pHdr);
//   bool sendSuccessful = forwardingPort->Send (packet->Copy (), dst, protocol);
//   if (sendSuccessful == false)
//     NS_LOG_INFO ("WARNING: Packet Transmission failed");

//   LogQueueEntries (forwardingPort); // Log the net device's queue details
// }

// void
// PpfsSwitch::SetRandomNumberGenerator ()
// {
//   /*
//    * The Seed and run are stored before generating the random variable because the run variable will
//    * be automatically incremented once a UniformRandomVariable is created.
//    */
//   m_seed = RandomGeneratorManager::GetSeed ();
//   m_run = RandomGeneratorManager::GetRun ();

//   m_uniformRandomVariable = RandomGeneratorManager::CreateUniformRandomVariable (0.0, 1.0);
// }

// void
// PpfsSwitch::ReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
//                                uint16_t protocol, const Address &src, const Address &dst,
//                                NetDevice::PacketType packetType)
// {
//   NS_LOG_INFO ("  Switch " << m_id << ": Received a packet (" << packet->GetSize () << "bytes) at "
//                            << Simulator::Now ().GetSeconds () << "s");

//   ForwardPacket (packet, protocol, dst); // Forward the packet
// }

// ns3::Ptr<ns3::NetDevice>
// PpfsSwitch::GetPort (const std::vector<PpfsSwitch::ForwardingAction> &forwardActions)
// {
//   double randomNumber (GenerateRandomNumber ()); // Generate random number
//   double lhs = 0.0;
//   double rhs = 0.0;

//   typedef std::vector<PpfsSwitch::ForwardingAction>::size_type sizeType;

//   // Loop through the routing table until a match is found
//   for (sizeType index = 0; index < forwardActions.size (); ++index)
//     { // A match is found based on the equation (lhs <= value < rhs)
//       lhs = (index == 0) ? 0.0 : forwardActions[index - 1].splitRatio;
//       rhs = forwardActions[index].splitRatio;

//       if ((lhs <= randomNumber) && (randomNumber < rhs))
//         return forwardActions[index].port; // Match found

//       /*
//        * If we are at the end and of the forwarding table and no match has been found yet the packet
//        * should be forwarded at the last forwarding slot. When such instance occur a log entry will
//        * be output to ease debugging.
//        */
//       if (index == (forwardActions.size () - 1))
//         {
//           NS_LOG_INFO ("The random number " << randomNumber << " did not fit any slot in"
//                                             << " the forwarding table.");
//           return forwardActions[index].port;
//         }
//     }

//   NS_FATAL_ERROR ("Forwarding port number for packet not found.");
//   return 0; // This return statement will never be hit
// }

// double
// PpfsSwitch::GenerateRandomNumber ()
// {
//   return m_uniformRandomVariable->GetValue ();
// }
