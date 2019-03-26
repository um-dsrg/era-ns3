#ifndef terminal_h
#define terminal_h

#include <map>

#include "ns3/ipv4-address.h"
#include "../custom-device.h"

class Terminal : public CustomDevice
{
public:
  using terminalContainer_t = std::map<id_t, Terminal>;

  Terminal () = default;
  Terminal (id_t id);

  ns3::Ipv4Address GetIpAddress () const;
  void SetIpAddress ();

private:
  ns3::Ipv4Address m_ipAddress;
};

#endif /* terminal_h */
