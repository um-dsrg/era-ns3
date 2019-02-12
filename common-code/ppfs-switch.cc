#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ppfs-switch.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PpfsSwitch");

PpfsSwitch::ForwardingAction::ForwardingAction(splitRatio_t splitRatio, Ptr<NetDevice> forwardingPort) :
splitRatio(splitRatio), forwardingPort(forwardingPort) {
}

// TODO: May need to reverse this so that it's sorted biggest to smallest!
bool PpfsSwitch::ForwardingAction::operator<(const PpfsSwitch::ForwardingAction &other) const {
    return splitRatio < other.splitRatio;
}

PpfsSwitch::PpfsSwitch(id_t id) : SwitchBase(id) {
}

void PpfsSwitch::AddEntryToRoutingTable(uint32_t srcIp, uint32_t dstIp, portNum_t srcPort, portNum_t dstPort,
                                        FlowProtocol protocol, splitRatio_t splitRatio,
                                        ns3::Ptr<ns3::NetDevice> forwardingPort) {
    NS_LOG_INFO("Adding entry to the routing table");

    RtFlow rtFlow {srcIp, dstIp, srcPort, dstPort, protocol};

    if (m_routingTable.find(rtFlow) == m_routingTable.end()) { // New entry
        std::vector<ForwardingAction> forwardingActionList;
        forwardingActionList.emplace_back(ForwardingAction(splitRatio, forwardingPort));
        m_routingTable.emplace(rtFlow, forwardingActionList);
    } else { // Update existing entry
        auto& forwardingActionList {m_routingTable.at(rtFlow)};
        forwardingActionList.emplace_back(ForwardingAction(splitRatio, forwardingPort));
    }
}

void PpfsSwitch::SetPacketReception() {
    uint32_t numOfDevices = m_node->GetNDevices ();
    NS_ASSERT (numOfDevices > 0);
    for (uint32_t currentDevice = 0; currentDevice < numOfDevices; ++currentDevice) {
        m_node->RegisterProtocolHandler (MakeCallback (&PpfsSwitch::PacketReceived, this),
                                         /* all protocols */ 0, m_node->GetDevice (currentDevice),
                                         /* disable promiscuous mode */ false);
    }
}

void PpfsSwitch::PacketReceived(Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                                uint16_t protocol, const Address &src, const Address &dst,
                                NetDevice::PacketType packetType) {
    NS_LOG_INFO("Switch " << m_id << " received a packet at " << Simulator::Now().GetSeconds() << "s");
}
