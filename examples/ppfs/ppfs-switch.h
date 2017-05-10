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
  void InsertEntryInRoutingTable (uint32_t srcIpAddr, uint32_t dstIpAddr, uint16_t portNumber,
                                  char protocol, uint32_t linkId, double flowRatio);
private:
  struct FlowMatch
  {
    uint32_t srcIpAddr;
    uint32_t dstIpAddr;
    uint16_t portNumber; // Destination Port Number
    enum Protocol { Tcp = 'T', Udp = 'U' };
    Protocol protocol;

    FlowMatch (uint32_t srcIpAddr, uint32_t dstIpAddr,
               uint16_t port, char protocol) : srcIpAddr (srcIpAddr), dstIpAddr (dstIpAddr),
                                                   portNumber (port)
    {
      if (protocol == 'T')
        this->protocol = Protocol::Tcp;
      if (protocol == 'U')
        this->protocol = Protocol::Udp;
    }

    bool operator<(const FlowMatch &other) const
    {
      /*
       * Used by the map to store the keys in order.
       * In this case it is sorted by Source Ip Address, then Destination Ip Address, and
       * finally Port Number.
       */
      if (srcIpAddr == other.srcIpAddr)
        {
          if (dstIpAddr == other.dstIpAddr)
            return portNumber < other.portNumber;
          else
            return dstIpAddr < other.dstIpAddr;
        }
      else
        return srcIpAddr < other.srcIpAddr;
    }
  };

  struct ForwardingAction
  {
    ns3::Ptr<ns3::NetDevice> port;
    double splitRatio;

    ForwardingAction (ns3::Ptr<ns3::NetDevice> port, double splitRatio) : port (port),
                                                                          splitRatio (splitRatio)
    {}
  };
  /*
   * Key -> Link Id, Value -> Pointer to the net device connected to that link.
   * This map will be used for transmission and when building the routing table.
   */
  std::map <uint32_t, ns3::Ptr<ns3::NetDevice>> m_linkNetDeviceTable;
  // Key -> FlowMatch, Value -> Vector of Forwarding actions
  std::map <FlowMatch, std::vector<ForwardingAction>> m_routingTable;
  uint32_t m_id; /*!< Stores the node's id */
};

#endif /* PPFS_SWITCH_H */
