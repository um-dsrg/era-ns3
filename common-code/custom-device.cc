#include "custom-device.h"

using namespace ns3;

CustomDevice::CustomDevice (id_t id) : m_id (id)
{
  m_node = CreateObject<Node> ();
}

CustomDevice::~CustomDevice ()
{
}
