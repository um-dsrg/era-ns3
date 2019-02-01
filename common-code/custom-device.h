#ifndef CUSTOM_DEVICE_H
#define CUSTOM_DEVICE_H

#include "ns3/node.h"
#include "definitions.h"

class CustomDevice
{
public:
  id_t
  GetId ()
  {
    return m_id;
  }

  ns3::Ptr<ns3::Node>
  GetNode ()
  {
    return m_node;
  }

protected:
  CustomDevice () = default;
  CustomDevice (id_t id);
  virtual ~CustomDevice ();

  id_t m_id{0}; //!< The Device Id taken from the KSP Xml file.
  ns3::Ptr<ns3::Node> m_node; //!< The Ns3 node linked with this device.
};

#endif // CUSTOM_DEVICE_H
