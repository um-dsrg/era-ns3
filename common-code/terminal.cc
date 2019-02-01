#include "ns3/ipv4.h"

#include "terminal.h"

using namespace ns3;

Terminal::Terminal (id_t id) : CustomDevice (id)
{
}

void
Terminal::RetrieveIpAddress ()
{
  m_ipAddress = m_node->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
}
