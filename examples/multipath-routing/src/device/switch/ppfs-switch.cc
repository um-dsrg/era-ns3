#include <algorithm>

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ppp-header.h"

#include "ppfs-switch.h"
#include "../../results-container.h"
#include "../../random-generator-manager.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PpfsSwitch");

/**
 ForwardingAction implementation
 */

PpfsSwitch::ForwardingAction::ForwardingAction (splitRatio_t splitRatio,
                                                Ptr<NetDevice> forwardingPort)
    : splitRatio (splitRatio), forwardingPort (forwardingPort)
{
}

bool
PpfsSwitch::ForwardingAction::operator< (const PpfsSwitch::ForwardingAction &other) const
{
  return splitRatio > other.splitRatio; // To sort in descending order
}

/**
 PpfsSwitch implementation
 */

PpfsSwitch::PpfsSwitch (id_t id, uint64_t switchBufferSize,
                        const std::string& txBufferRetrievalMethod, ResultsContainer &resContainer)
    : SwitchBase (id, switchBufferSize, txBufferRetrievalMethod, resContainer)
{
  m_uniformRandomVariable = RandomGeneratorManager::CreateUniformRandomVariable (0.0, 1.0);
}

void
PpfsSwitch::AddEntryToRoutingTable (uint32_t srcIp, uint32_t dstIp, portNum_t srcPort,
                                    portNum_t dstPort, FlowProtocol protocol,
                                    ns3::Ptr<ns3::NetDevice> forwardingPort,
                                    splitRatio_t splitRatio)
{
  NS_LOG_INFO ("Switch " << m_id << ": Adding entry to routing table");
  RtFlow rtFlow{srcIp, dstIp, srcPort, dstPort, protocol};

  if (m_routingTable.find (rtFlow) == m_routingTable.end ())
    { // New entry
      std::vector<ForwardingAction> forwardingActionList;
      forwardingActionList.emplace_back (ForwardingAction (splitRatio, forwardingPort));
      m_routingTable.emplace (rtFlow, forwardingActionList);
      NS_LOG_INFO ("Adding new flow entry.\nFlow: " << rtFlow << "\nSplit Ratio: " << splitRatio);
    }
  else
    { // Update existing entry
      auto &forwardingActionList{m_routingTable.at (rtFlow)};
      forwardingActionList.emplace_back (ForwardingAction (splitRatio, forwardingPort));
      NS_LOG_INFO ("Updating existing entry.\nFlow: " << rtFlow << "\nSplit Ratio: " << splitRatio);
    }
}

void
PpfsSwitch::SetPacketReception ()
{
  uint32_t numOfDevices = m_node->GetNDevices ();
  NS_ASSERT (numOfDevices > 0);

  for (uint32_t currentDevice = 0; currentDevice < numOfDevices; ++currentDevice)
    {
      m_node->RegisterProtocolHandler (MakeCallback (&PpfsSwitch::PacketReceived, this),
                                       /* all protocols */ 0, m_node->GetDevice (currentDevice),
                                       /* disable promiscuous mode */ false);
    }
}

void
PpfsSwitch::PacketReceived (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                            uint16_t protocol, const Address &src, const Address &dst,
                            NetDevice::PacketType packetType)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds () << "s: Switch " << m_id << " received a packet");

  if (m_receiveBuffer.AddPacket(packet) == false)
    return;

  auto parsedFlow = ExtractFlowFromPacket (packet, protocol);
  NS_LOG_INFO ("  " << parsedFlow);

  try
  {
    auto &forwardingActionList = m_routingTable.at (parsedFlow);
    auto randNum = m_uniformRandomVariable->GetValue ();
    auto forwardingNetDevice = ns3::Ptr<ns3::NetDevice>{nullptr};

    for (const auto &forwardingAction : forwardingActionList)
    {
      if (randNum <= forwardingAction.splitRatio)
      {
        forwardingNetDevice = forwardingAction.forwardingPort;
        break;
      }
    }

    NS_ABORT_MSG_IF (forwardingNetDevice == nullptr,
                     "Switch " << m_id << " failed to find the forwarding port");

    auto sendSuccess = forwardingNetDevice->Send (packet->Copy (), dst, protocol);
    NS_ABORT_MSG_IF (!sendSuccess, "Switch " << m_id << " failed to forward packet");
    NS_LOG_INFO (Simulator::Now ().GetSeconds () << "s: Switch " << m_id <<
                 " forwarded a packet at " << Simulator::Now ());
  }
  catch (const std::out_of_range &oor)
  {
    NS_ABORT_MSG ("Routing table Miss on Switch " << m_id << ".Flow: " << parsedFlow);
  }
}

void
PpfsSwitch::ReconcileSplitRatios ()
{
  NS_LOG_INFO ("Switch " << m_id << ": Reconciling routing table");

  for (auto &routingTableEntry : m_routingTable)
    {

      auto &forwardingActionList = routingTableEntry.second;

      NS_LOG_INFO ("Working on Flow\n--------------------\n"
                   << routingTableEntry.first
                   << "--------------------\n"
                      "Split ratios before reconciliation:\n"
                      "--------------------");

      for (const auto &forwardingAction : forwardingActionList)
        {
          NS_LOG_INFO ("Spilt Ratio: " << forwardingAction.splitRatio);
        }
      NS_LOG_INFO ("--------------------");

      // Delete entries with a 0 split ratio value
      auto predicate = [](const ForwardingAction &fa) { return (fa.splitRatio == 0); };
      forwardingActionList.erase (
          std::remove_if (forwardingActionList.begin (), forwardingActionList.end (), predicate),
          forwardingActionList.end ());

      NS_LOG_INFO ("--------------------\n"
                   "Split ratios after zero removal:\n"
                   "--------------------");

      for (const auto &forwardingAction : forwardingActionList)
        {
          NS_LOG_INFO ("Spilt Ratio: " << forwardingAction.splitRatio);
        }
      NS_LOG_INFO ("--------------------");

      if (forwardingActionList.empty ())
        {
          continue;
        }

      auto total = splitRatio_t{0.0};

      for (const auto &forwardingAction : forwardingActionList)
        {
          total += forwardingAction.splitRatio;
        }

      /**
         Sort the forwarding action list in descending order (largest to smallest) and
         normalise the split ratios such that they total to 1.
         */
      std::sort (forwardingActionList.begin (), forwardingActionList.end ());
      for (auto &forwardingAction : forwardingActionList)
        {
          forwardingAction.splitRatio = (forwardingAction.splitRatio / total);
        }

      // Convert to cumulative split ratios
      for (size_t i = 1; i < forwardingActionList.size (); ++i)
        {
          forwardingActionList[i].splitRatio += forwardingActionList[i - 1].splitRatio;
        }

      if (Abs (forwardingActionList.back ().splitRatio - 1.0) <= 1e-5)
        {
          forwardingActionList.back ().splitRatio = 1.0;
        }

      NS_LOG_INFO ("Split ratios after reconciliation:\n--------------------");
      for (const auto &forwardingAction : forwardingActionList)
        {
          NS_LOG_INFO ("Spilt Ratio: " << forwardingAction.splitRatio);
        }
      NS_LOG_INFO ("--------------------");

      NS_ABORT_MSG_IF (forwardingActionList.back ().splitRatio != 1.0,
                       "Switch: " << m_id << "\n"
                                  << "Flow: " << routingTableEntry.first << "\n"
                                  << "The final split ratio is not equal to 1. Split Ratio: "
                                  << forwardingActionList.back ().splitRatio);
    }
}
