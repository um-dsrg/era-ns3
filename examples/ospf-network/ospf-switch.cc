#include "ns3/log.h"
#include "ns3/abort.h"

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
OspfSwitch::LogPacketTransmission (std::string context, ns3::Ptr<const ns3::Packet> packet)
{
  std::cout << "Switch " << m_id << " received a packet" << std::endl;

  std::cout << "Packet Size is: " << packet->GetSize() << std::endl;

  // TODO: Log Queue Entries here -> We just need the net device. Use the map linkNetDeviceTable as it has the
  // link id as key and the net device as value.
  // TODO: Log Link Statistics here
  // For link statistics we need, the port, flowId and the packetsize.
  // Port we have access to.
  // Packet size we have access to.
  // Need to get the flow id somehow.
}
