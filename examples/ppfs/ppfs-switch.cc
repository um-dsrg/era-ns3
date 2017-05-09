#include "ns3/abort.h"
#include "ppfs-switch.h"

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
