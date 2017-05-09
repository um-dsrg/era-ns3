#ifndef PPFS_SWITCH_H
#define PPFS_SWITCH_H

#include <map>

#include "ns3/net-device.h"

class PpfsSwitch
{
public:
  PpfsSwitch ();
  PpfsSwitch (uint32_t id);
  void InsertNetDevice (uint32_t linkId, ns3::Ptr<ns3::NetDevice> device);
private:
  /*
   * Key -> Link Id, Value -> Pointer to the net device connected to that link.
   * This map will be used for transmission and when building the routing table.
   */
  std::map <uint32_t, ns3::Ptr<ns3::NetDevice>> m_linkNetDeviceTable;

  uint32_t m_id; /*!< Stores the node's id */
};

#endif /* PPFS_SWITCH_H */
