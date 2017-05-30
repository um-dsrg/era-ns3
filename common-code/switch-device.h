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

  struct QueueResults /*!< Stores statistics about the queue of each net device. */
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

  /**
   *  \brief function description
   *
   *  Detailed description
   *
   *  \param param
   *  \return return type
   */
  const std::map <LinkId_t, QueueResults>& GetQueueResults () const;
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

  void LogQueueEntries (ns3::Ptr<ns3::NetDevice> port);

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

  std::map <LinkId_t, QueueResults> m_switchQueueResults; /*!< Key -> LinkId, Value -> QueueStats */

  NodeId_t m_id;
  ns3::Ptr<ns3::Node> m_node;
};

#endif /* SWITCH_DEVICE_H */
