#include "ns3/abort.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/simulator.h"

#include "switch-device.h"

using namespace ns3;

void
SwitchDevice::InsertNetDevice(LinkId_t linkId, Ptr<NetDevice> device)
{
  auto ret = m_linkNetDeviceTable.insert({linkId, device});
  NS_ABORT_MSG_IF(ret.second == false,
                  "The Link ID " << linkId << " is already stored in node's "
                  << m_id << " Link->NetDevice map");

  m_netDeviceLinkTable.insert({device, linkId});

  m_switchQueueResults.insert({linkId, QueueResults()}); // Pre-populating the map with empty values
}

Ptr<Queue>
SwitchDevice::GetQueueFromLinkId(LinkId_t linkId) const
{
  auto ret = m_linkNetDeviceTable.find(linkId);
  NS_ABORT_MSG_IF(ret == m_linkNetDeviceTable.end(), "The port connecting link id: " << linkId
                  << " was not found");

  Ptr<NetDevice> port = ret->second;
  Ptr<PointToPointNetDevice> p2pDevice = port->GetObject<PointToPointNetDevice>();
  return p2pDevice->GetQueue();
}

const std::map <LinkId_t, SwitchDevice::QueueResults>&
SwitchDevice::GetQueueResults() const
{
  return m_switchQueueResults;
}

const std::map <SwitchDevice::LinkFlowId, LinkStatistic>&
SwitchDevice::GetLinkStatistics () const
{
  return m_linkStatistics;
}

SwitchDevice::SwitchDevice() {}

SwitchDevice::SwitchDevice(NodeId_t id, Ptr<Node> node) : m_id(id), m_node(node) {}

SwitchDevice::~SwitchDevice() {}

void
SwitchDevice::LogLinkStatistics (Ptr<NetDevice> port, FlowId_t flowId, uint32_t packetSize)
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

void
SwitchDevice::LogQueueEntries (Ptr<NetDevice> port)
{
  Ptr<PointToPointNetDevice> p2pDevice = port->GetObject<PointToPointNetDevice>();
  Ptr<Queue> queue = p2pDevice->GetQueue();

  uint32_t numOfPackets (queue->GetNPackets());
  uint32_t numOfBytes (queue->GetNBytes());

  auto ret = m_netDeviceLinkTable.find(port);
  NS_ABORT_MSG_IF(ret == m_netDeviceLinkTable.end() , "LinkId not found from NetDevice");

  QueueResults& queueResults (m_switchQueueResults[ret->second/*link id*/]);

  if (numOfPackets > queueResults.peakNumOfPackets) queueResults.peakNumOfPackets = numOfPackets;
  if (numOfBytes > queueResults.peakNumOfBytes) queueResults.peakNumOfBytes = numOfBytes;
}
