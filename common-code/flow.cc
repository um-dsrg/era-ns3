#include "flow.h"

/**
 * Path Implementation
 */

/**
 * The global port number counter starting from 49152 as it is the start of the range of
 * dynamic/private port numbers. This was done to avoid any conflicts with any other
 * standards.
 */
portNum_t Path::portNumCounter = 49152;

Path::Path ()
{
  srcPort = portNumCounter++;
  dstPort = portNumCounter++;
}

void
Path::AddLink (id_t linkId)
{
  links.push_back (linkId);
}

std::ostream &
operator<< (std::ostream &output, Path &path)
{
  output << "Path ID: " << path.id << "\n";
  output << "Source Port: " << path.srcPort << "\n";
  output << "Destination Port: " << path.dstPort << "\n";
  output << "Links \n";

  for (const auto &linkId : path.links)
    {
      output << "Link ID: " << linkId << "\n";
    }
}

/**
 * Flow Implementation
 */

void
Flow::AddPath (const Path &path)
{
  m_paths.push_back (path);
}

bool
Flow::operator< (const Flow &other) const
{
  /*
   * Used by the map to store the keys in order.
   * In this case it is sorted by Source Ip Address, then Destination Ip Address, and
   * finally Port Number.
   */
  if (srcAddress.Get () == other.srcAddress.Get ())
    return dstAddress.Get () < other.dstAddress.Get ();
  else
    return srcAddress.Get () < other.srcAddress.Get ();
}

std::ostream &
operator<< (std::ostream &output, Flow &flow)
{
  ns3::Ipv4Address address;
  output << "Flow ID: " << flow.id << "\n";
  output << "Source IP Addr: ";
  flow.srcAddress.Print (output);
  output << "\n";
  output << "Destination IP Addr: ";
  flow.dstAddress.Print (output);
  output << "\n";
  output << "Protocol: " << static_cast<char> (flow.protocol) << "\n";
  output << "----------------------";

  return output;
}
