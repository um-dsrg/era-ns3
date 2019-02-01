#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <cstdint>

// Variable definitions ///////////////////////////////////////////////////////
typedef uint32_t id_t;
typedef double delay_t;
typedef double dataRate_t;
// TODO Remove the below typedefs
typedef uint32_t LinkId_t;
typedef uint32_t NodeId_t;
typedef uint32_t FlowId_t;

// Enum definitions /////////////////////////////////////////////////////////
enum class NodeType : char { Switch = 'S', Terminal = 'T' };

// Structure definitions /////////////////////////////////////////////////////////

// Flow Match definitions ///////////////////////////////////////////////////
#include <iostream>
#include "ns3/ipv4-address.h"
struct Flow
{
  FlowId_t id;
  uint32_t srcIpAddr;
  uint32_t dstIpAddr;
  uint16_t dstPortNumber; // Destination Port Number
  enum Protocol { Tcp = 'T', Udp = 'U', Icmp = 'I', Ack = 'A', Undefined = 'X' };

  // Default Constructor
  Flow () : id (0), srcIpAddr (0), dstIpAddr (0), dstPortNumber (0), protocol (Protocol::Undefined)
  {
  }
  Flow (FlowId_t id, uint32_t srcIpAddr, uint32_t dstIpAddr, uint16_t port, char protocol)
      : id (id),
        srcIpAddr (srcIpAddr),
        dstIpAddr (dstIpAddr),
        dstPortNumber (port),
        protocol (Protocol::Undefined)
  {
    SetProtocol (protocol);
  }

  void
  SetProtocol (Protocol &protocol)
  {
    this->protocol = protocol;
  }

  void
  SetProtocol (const char &protocol)
  {
    if (protocol == 'T')
      this->protocol = Protocol::Tcp;
    if (protocol == 'U')
      this->protocol = Protocol::Udp;
    if (protocol == 'A')
      this->protocol = Protocol::Ack;
  }

  Protocol
  GetProtocol ()
  {
    return protocol;
  }

  bool
  operator< (const Flow &other) const
  {
    /*
     * Used by the map to store the keys in order.
     * In this case it is sorted by Source Ip Address, then Destination Ip Address, and
     * finally Port Number.
     */
    if (srcIpAddr == other.srcIpAddr)
      {
        if (dstIpAddr == other.dstIpAddr)
          return dstPortNumber < other.dstPortNumber;
        else
          return dstIpAddr < other.dstIpAddr;
      }
    else
      return srcIpAddr < other.srcIpAddr;
  }

  friend std::ostream &
  operator<< (std::ostream &output, Flow &value)
  {
    ns3::Ipv4Address address;
    output << "Flow ID: " << value.id << "\n";
    // Source Ip Address
    output << "Source IP Addr: " << value.srcIpAddr << " (";
    address.Set (value.srcIpAddr);
    address.Print (output);
    output << ")\n";
    // Destination Ip Address
    output << "Destination IP Addr: " << value.dstIpAddr << " (";
    address.Set (value.dstIpAddr);
    address.Print (output);
    output << ")\n";
    // Port Number
    output << "Destination Port Number: " << value.dstPortNumber << "\n";
    output << "Protocol: " << (char) value.protocol << "\n";
    output << "----------------------";
    return output;
  }

private:
  Protocol protocol;
};

#include "ns3/nstime.h"
struct LinkStatistic /*!< Stores the link statistics */
{
  ns3::Time timeFirstTx;
  ns3::Time timeLastTx;
  uint32_t packetsTransmitted;
  uint32_t bytesTransmitted;

  LinkStatistic () : timeFirstTx (0), timeLastTx (0), packetsTransmitted (0), bytesTransmitted (0)
  {
  }
};

#endif /* DEFINITIONS_H */
