#include "custom-device.h"

using namespace ns3;

CustomDevice::CustomDevice (id_t id) : m_id (id) {
    m_node = CreateObject<Node> ();
}

CustomDevice::~CustomDevice () {
}

id_t CustomDevice::GetId () const {
    return m_id;
}

ns3::Ptr<ns3::Node> CustomDevice::GetNode () const {
    return m_node;
}
