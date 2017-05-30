#ifndef SWITCH_DEVICE_H
#define SWITCH_DEVICE_H

#include <map>

#include "ns3/net-device.h"
#include "ns3/queue.h"

#include "definitions.h"

class SwitchDevice
{
public:
  /**
   *  \brief Insert a reference to the NetDevice and the link id that is connected with that device
   *
   *  In this function a net device and the link id connected to it are stored.
   *
   *  \param linkId The link's id
   *  \param device A pointer to the net device
   */
  void InsertNetDevice (LinkId_t linkId, ns3::Ptr<ns3::NetDevice> device);

  // Queue Statistics /////////////////////////////////////////////////////////
  struct QueueResults // Stores statistics about the queue of each net device
  {
    uint32_t peakNumOfPackets;
    uint32_t peakNumOfBytes;

    QueueResults () : peakNumOfPackets (0), peakNumOfBytes (0) {}
  };

  /**
   *  \brief Will return a pointer to the queue connected to the link id
   *
   *  Returns a pointer to the queue that is connected with that particular link id
   *
   *  \param linkId The link id
   *  \return Queue The queue connected to that link id
   */
  ns3::Ptr<ns3::Queue> GetQueueFromLinkId (LinkId_t linkId) const;

  const std::map <LinkId_t, QueueResults>& GetQueueResults () const;

  // Link Statistics //////////////////////////////////////////////////////////
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

  const std::map <LinkFlowId, LinkStatistic>& GetLinkStatistics () const;
protected:
  /**
   *  \brief Constructor is protected because this class cannot be instantiated directly
   */
  SwitchDevice();
  /**
   *  \brief Constructor is protected because this class cannot be instantiated directly
   *  \param id The node's id
   *  \param node An ns3 pointer to the node that this switch represents
   */
  SwitchDevice(NodeId_t id, ns3::Ptr<ns3::Node> node);
  virtual ~SwitchDevice();

  virtual void SetPacketHandlingMechanism() = 0;

  // Queue Statistics /////////////////////////////////////////////////////////
  void LogQueueEntries (ns3::Ptr<ns3::NetDevice> port);
  std::map <LinkId_t, QueueResults> m_switchQueueResults; /*!< Key -> LinkId, Value -> QueueStats */

  // Link Statistics //////////////////////////////////////////////////////////
  // Key -> link id + flow id, Value -> Statistics for that pair
  std::map <LinkFlowId, LinkStatistic> m_linkStatistics;

  /*
   * Key -> Link Id, Value -> Pointer to the net device connected to that link.
   * This map will be used for transmission and when building the routing table.
   */
  std::map <LinkId_t, ns3::Ptr<ns3::NetDevice>> m_linkNetDeviceTable;
  /*
   * Key -> Pointer to the net device, Value -> The link Id that is connected to
   * this net device. (This is the inverse of m_linkNetDeviceTable)
   */
  std::map <ns3::Ptr<ns3::NetDevice>, LinkId_t> m_netDeviceLinkTable;

  NodeId_t m_id; /*!< The id associated with this device */
  ns3::Ptr<ns3::Node> m_node; /*!< Pointer to the ns3 node associated with this device */
};

#endif /* SWITCH_DEVICE_H */
