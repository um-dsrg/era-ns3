#ifndef SWITCH_BASE_H
#define SWITCH_BASE_H

#include <map>

#include "ns3/net-device.h"
#include "ns3/queue.h"

#include "definitions.h"
#include "custom-device.h"

class SwitchBase : public CustomDevice {
public:
    void InsertNetDevice (LinkId_t linkId, ns3::Ptr<ns3::NetDevice> device);

    // Queue Statistics /////////////////////////////////////////////////////////
    struct QueueResults {// Stores statistics about the queue of each net device

        uint32_t peakNumOfPackets;
        uint32_t peakNumOfBytes;

        QueueResults () : peakNumOfPackets (0), peakNumOfBytes (0) {
        }
    };
  ns3::Ptr<ns3::Queue<ns3::Packet>> GetQueueFromLinkId (LinkId_t linkId) const;

  const std::map<LinkId_t, QueueResults> &GetQueueResults () const;

  // Link Statistics //////////////////////////////////////////////////////////
  struct LinkFlowId {
    LinkId_t linkId;
    FlowId_t flowId;
    LinkFlowId () : linkId (0), flowId (0) {
    }
    LinkFlowId (LinkId_t linkId, FlowId_t flowId) : linkId (linkId), flowId (flowId) {
    }

    bool
    operator< (const LinkFlowId &other) const {
      // Used by the map to store the keys in order.
      if (linkId == other.linkId)
        return flowId < other.flowId;
      else
        return linkId < other.linkId;
    }
  };

  const std::map<LinkFlowId, LinkStatistic> &GetLinkStatistics () const;

protected:
  /**
   *  \brief Constructor is protected because this class cannot be instantiated directly
   */
  SwitchBase () = default;
  SwitchBase (id_t id);
  virtual ~SwitchBase ();

  virtual void SetPacketReception () = 0;

  // Queue Statistics /////////////////////////////////////////////////////////
  void LogQueueEntries (ns3::Ptr<ns3::NetDevice> port);
  std::map<LinkId_t, QueueResults> m_switchQueueResults; /*!< Key -> LinkId, Value -> QueueStats */

  // Link Statistics //////////////////////////////////////////////////////////
  void LogLinkStatistics (ns3::Ptr<ns3::NetDevice> port, FlowId_t flowId, uint32_t packetSize);
  std::map<LinkFlowId, LinkStatistic> m_linkStatistics; /*!< Key -> Link+Flow, Value->LinkStats */

  /*
   * Key -> Link Id, Value -> Pointer to the net device connected to that link.
   * This map will be used for transmission and when building the routing table.
   */
  std::map<LinkId_t, ns3::Ptr<ns3::NetDevice>> m_linkNetDeviceTable;
  /*
   * Key -> Pointer to the net device, Value -> The link Id that is connected to
   * this net device. (This is the inverse of m_linkNetDeviceTable)
   */
  std::map<ns3::Ptr<ns3::NetDevice>, LinkId_t> m_netDeviceLinkTable;
};

#endif /* SWITCH_BASE_H */
