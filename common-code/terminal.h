#ifndef TERMINAL_H
#define TERMINAL_H

#include "ns3/ipv4-address.h"
#include "custom-device.h"

class Terminal : public CustomDevice
{
public:
  Terminal () = default;
  Terminal (id_t id);

  void RetrieveIpAddress ();

private:
  ns3::Ipv4Address m_ipAddress;
};

#endif // TERMINAL_H
