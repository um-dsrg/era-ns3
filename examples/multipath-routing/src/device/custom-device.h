#ifndef custom_device_h
#define custom_device_h

#include "ns3/node.h"
#include "../definitions.h"

class CustomDevice {
public:
    id_t GetId () const;
    ns3::Ptr<ns3::Node> GetNode () const;

protected:
    CustomDevice (id_t id);
    virtual ~CustomDevice ();

    id_t m_id{0}; /**< The Device Id taken from the KSP Xml file. */
    ns3::Ptr<ns3::Node> m_node; /**< The Ns3 node linked with this device. */
};

#endif /* custom_device_h */
