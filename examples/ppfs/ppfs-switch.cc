#include "ns3/abort.h"
#include "ppfs-switch.h"

using namespace ns3;

PpfsSwitch::PpfsSwitch ()
{}

PpfsSwitch::PpfsSwitch (uint32_t id) : m_id(id)
{}

void
PpfsSwitch::InsertNetDevice(uint32_t linkId, ns3::Ptr<ns3::NetDevice> device)
{
  auto ret = m_linkNetDeviceTable.insert({linkId, device});
  NS_ABORT_MSG_IF(ret.second == false,
                  "The Link ID " << linkId << " is already stored in node's "
                  << m_id << " Link->NetDevice map");
}

void
PpfsSwitch::InsertEntryInRoutingTable(uint32_t srcIpAddr, uint32_t dstIpAddr, uint16_t portNumber,
                                      char protocol, uint32_t linkId, double flowRatio)
{
  FlowMatch currentFlow (srcIpAddr, dstIpAddr, portNumber, protocol);

  auto ret = m_linkNetDeviceTable.find(linkId);
  NS_ABORT_MSG_IF(ret == m_linkNetDeviceTable.end(), "Link " << linkId << " was not found in "<<
                  "the routing table of Switch with Id " << m_id);
  Ptr<NetDevice> forwardDevice = ret->second;

  auto routingTableEntry = m_routingTable.find(currentFlow);
  if (routingTableEntry == m_routingTable.end()) // Route does not exist in table
    {
      std::vector<ForwardingAction> forwardAction;
      forwardAction.push_back(ForwardingAction(forwardDevice, flowRatio));
      m_routingTable.insert({currentFlow, forwardAction});
    }
  else // Route already exists in table
    {
      std::vector<ForwardingAction>& forwardActions = routingTableEntry->second;
      ForwardingAction& lastAction = forwardActions.back();
      double currentSplitRatio = lastAction.splitRatio + flowRatio;
      forwardActions.push_back(ForwardingAction(forwardDevice, currentSplitRatio));
    }
}
