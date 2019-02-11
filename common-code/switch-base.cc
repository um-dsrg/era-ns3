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

#include "switch-base.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SwitchBase");

SwitchBase::SwitchBase (id_t id) : CustomDevice (id) {
}

SwitchBase::~SwitchBase () {
}

/**
 @brief Insert a reference to the NetDevice and the link id that is connected with that device.

 In this function a net device and the link id connected to it are stored.

 @param linkId The link id.
 @param device A pointer to the net device.
 */
void SwitchBase::InsertNetDevice (LinkId_t linkId, Ptr<NetDevice> device) {
    auto ret = m_linkNetDeviceTable.insert ({linkId, device});
    NS_ABORT_MSG_IF (ret.second == false, "The Link ID " << linkId << " is already stored in node's " << m_id <<
                     " Link->NetDevice map");

    m_netDeviceLinkTable.insert ({device, linkId});
    m_switchQueueResults.insert ({linkId, QueueResults ()}); // Pre-populating the map with empty values
}

/**
 @brief Returns a pointer to the queue connected to the link id.

 Returns a pointer to the queue that is connected with that particular link id

 @param linkId The link id.
 @return The queue connected to that link id.
 */
Ptr<Queue<Packet>> SwitchBase::GetQueueFromLinkId (LinkId_t linkId) const {
    auto ret = m_linkNetDeviceTable.find (linkId);
    NS_ABORT_MSG_IF (ret == m_linkNetDeviceTable.end (),
                     "The port connecting link id: " << linkId << " was not found");

    Ptr<NetDevice> port = ret->second;
    Ptr<PointToPointNetDevice> p2pDevice = port->GetObject<PointToPointNetDevice> ();
    return p2pDevice->GetQueue ();
}

const std::map<LinkId_t, SwitchBase::QueueResults>& SwitchBase::GetQueueResults () const {
    return m_switchQueueResults;
}

const std::map<SwitchBase::LinkFlowId, LinkStatistic>& SwitchBase::GetLinkStatistics () const {
    return m_linkStatistics;
}

void SwitchBase::LogLinkStatistics (Ptr<NetDevice> port, FlowId_t flowId, uint32_t packetSize) {
    auto linkRet = m_netDeviceLinkTable.find (port);
    NS_ABORT_MSG_IF (linkRet == m_netDeviceLinkTable.end (), "The Link connected to the net device"
                     "was not found");

    LinkFlowId linkFlowId (linkRet->second, flowId);

    auto linkStatRet = m_linkStatistics.find (linkFlowId);

    if (linkStatRet == m_linkStatistics.end ()) { // Link statistics entry not found, create it
        LinkStatistic linkStatistic;
        linkStatistic.timeFirstTx = Simulator::Now ();
        linkStatistic.timeLastTx = Simulator::Now ();
        linkStatistic.packetsTransmitted++;
        linkStatistic.bytesTransmitted += packetSize;
        m_linkStatistics.insert ({linkFlowId, linkStatistic});
    } else { // Update the link statistics entry
        LinkStatistic &linkStatistic = linkStatRet->second;
        linkStatistic.timeLastTx = Simulator::Now ();
        linkStatistic.packetsTransmitted++;
        linkStatistic.bytesTransmitted += packetSize;
    }
}

void SwitchBase::LogQueueEntries (Ptr<NetDevice> port) {
    Ptr<PointToPointNetDevice> p2pDevice = port->GetObject<PointToPointNetDevice> ();
    Ptr<Queue<Packet>> queue = p2pDevice->GetQueue ();

    uint32_t numOfPackets (queue->GetNPackets ());
    uint32_t numOfBytes (queue->GetNBytes ());

    auto ret = m_netDeviceLinkTable.find (port);
    NS_ABORT_MSG_IF (ret == m_netDeviceLinkTable.end (), "LinkId not found from NetDevice");

    QueueResults &queueResults (m_switchQueueResults[ret->second /*link id*/]);

    if (numOfPackets > queueResults.peakNumOfPackets) {
        queueResults.peakNumOfPackets = numOfPackets;
    }

    if (numOfBytes > queueResults.peakNumOfBytes) {
        queueResults.peakNumOfBytes = numOfBytes;
    }
}
