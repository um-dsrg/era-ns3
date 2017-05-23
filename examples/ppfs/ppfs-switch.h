#ifndef PPFS_SWITCH_H
#define PPFS_SWITCH_H

#include <map>

#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/queue.h"

class PpfsSwitch
{
public:
  PpfsSwitch ();
  PpfsSwitch (uint32_t id);
  void InsertNetDevice (uint32_t linkId, ns3::Ptr<ns3::NetDevice> device);
  void InsertEntryInRoutingTable (uint32_t srcIpAddr, uint32_t dstIpAddr, uint16_t portNumber,
                                  char protocol, uint32_t flowId, uint32_t linkId,
                                  double flowRatio);
  void ForwardPacket (ns3::Ptr<const ns3::Packet> packet, uint16_t protocol,
                      const ns3::Address &dst);

  ns3::Ptr<ns3::Queue> GetQueueFromLinkId (uint32_t linkId) const;

  // Stores statistics about the queue of each net device.
  struct QueueResults
  {
    uint32_t peakNumOfPackets;
    uint32_t peakNumOfBytes;

    QueueResults () : peakNumOfPackets (0), peakNumOfBytes (0) {}
  };
  // Key -> Link ID
  const std::map <uint32_t, QueueResults>& GetQueueResults () const;

  struct LinkFlowId
  {
    uint32_t linkId;
    uint32_t flowId;
    LinkFlowId () : linkId (0), flowId (0) {}
    LinkFlowId (uint32_t linkId, uint32_t flowId) : linkId (linkId), flowId (flowId) {}

    bool operator<(const LinkFlowId &other) const
    {
      // Used by the map to store the keys in order.
      if (linkId == other.linkId)
        return flowId < other.flowId;
      else
        return linkId < other.linkId;
    }
  };

  // TODO: Put this into a common header file.
  struct LinkStatistic
  {
    ns3::Time timeFirstTx;
    ns3::Time timeLastTx;
    uint32_t packetsTransmitted;
    uint32_t bytesTransmitted;

    LinkStatistic () : timeFirstTx (0), timeLastTx (0), packetsTransmitted (0), bytesTransmitted (0)
    {}
  };
  const std::map <LinkFlowId, LinkStatistic>& GetLinkStatistics () const;

  // TODO: Insert function to set up the random number generator
private:
  struct FlowMatch
  {
    uint32_t srcIpAddr;
    uint32_t dstIpAddr;
    uint16_t portNumber; // Destination Port Number
    enum Protocol { Tcp = 'T', Udp = 'U', Undefined = 'X' };
    Protocol protocol;

    // Default Constructor
    FlowMatch () : srcIpAddr(0), dstIpAddr(0),
                   portNumber(0), protocol(Protocol::Undefined)
    {}
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

    friend std::ostream& operator<< (std::ostream& output, FlowMatch& value)
    {
      ns3::Ipv4Address address;

      // Source Ip Address
      output << "Source IP Addr: " << value.srcIpAddr << " (";
      address.Set(value.srcIpAddr);
      address.Print(output);
      output << ")\n";
      // Destination Ip Address
      output << "Destination IP Addr: " << value.dstIpAddr << " (";
      address.Set(value.dstIpAddr);
      address.Print(output);
      output << ")\n";
      // Port Number
      output << "Port Number: " << value.portNumber << "\n";
      output << "Protocol: " << (char) value.protocol << "\n";
      output << "----------------------";
      return output;
    }
  };

  struct ForwardingAction
  {
    ns3::Ptr<ns3::NetDevice> port;
    double splitRatio;

    ForwardingAction (ns3::Ptr<ns3::NetDevice> port, double splitRatio) : port (port),
                                                                          splitRatio (splitRatio)
    {}

    friend std::ostream& operator<< (std::ostream& output, ForwardingAction& value)
    {
      output << "Split Ratio " << value.splitRatio << "\n";
      output << "----------------------";
      return output;
    }
  };

  FlowMatch ParsePacket (ns3::Ptr<const ns3::Packet> packet, uint16_t protocol);
  ns3::Ptr<ns3::NetDevice> GetPort (const std::vector<ForwardingAction>& forwardActions);
  void LogLinkStatistics (ns3::Ptr<ns3::NetDevice> port, uint32_t flowId, uint32_t packetSize);
  void LogQueueEntries (ns3::Ptr<ns3::NetDevice> port);

  void SetRandomNumberGenerator ();
  double GenerateRandomNumber ();

  /*
   * Key -> Link Id, Value -> Pointer to the net device connected to that link.
   * This map will be used for transmission and when building the routing table.
   */
  std::map <uint32_t, ns3::Ptr<ns3::NetDevice>> m_linkNetDeviceTable;
  /*
   * Key -> Pointer to the net device, Value -> The link Id that is connected to
   * this net device. (This is the inverse of m_linkNetDeviceTable)
   */
  std::map <ns3::Ptr<ns3::NetDevice>, uint32_t> m_netDeviceLinkTable;
  /*
   * Key -> FlowMatch, Value -> Vector of Forwarding actions.
   * This map stores the routing table which is used to map a flow to
   * a set of forwarding actions.
   */
  struct FlowDetails
  {
    uint32_t flowId;
    std::vector<ForwardingAction> forwardingActions;

    FlowDetails () : flowId (0) {}
    FlowDetails (uint32_t flowId) : flowId (flowId) {}
  };
  std::map <FlowMatch, FlowDetails> m_routingTable;

  // Key -> link id + flow id, Value -> Statistics for that pair
  std::map <LinkFlowId, LinkStatistic> m_linkStatistics;
  std::map <uint32_t, QueueResults> m_switchQueueResults; /*!< Key -> LinkId, Value -> QueueStats */

  ns3::Ptr<ns3::UniformRandomVariable> m_uniformRandomVariable; /*!< Used for flow splitting */
  uint32_t m_id; /*!< Stores the node's id */
};

#endif /* PPFS_SWITCH_H */
