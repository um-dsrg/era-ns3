// -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*-
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//

// NS3 - Layer 4 Protocol base class
// George F. Riley, Georgia Tech, Spring 2007

#ifndef IP_L4_PROTOCOL_H
#define IP_L4_PROTOCOL_H

#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"
#include "ns3/ipv6-interface.h"

namespace ns3 {

class Packet;
class Ipv4Address;
class Ipv4Interface;
class Ipv4Route;
class Ipv6Route;

/**
 * \brief L4 Protocol abstract base class 
 *
 * This is an abstract base class for layer four protocols which use IP as
 * the network layer.
 */
class IpL4Protocol : public Object
{
public:
  enum RxStatus {
    RX_OK,
    RX_CSUM_FAILED,
    RX_ENDPOINT_CLOSED,
    RX_ENDPOINT_UNREACH
  };

  static TypeId GetTypeId (void);

  virtual ~IpL4Protocol ();

  /**
   * \returns the protocol number of this protocol.
   */
  virtual int GetProtocolNumber (void) const = 0;

  /**
   * \param p packet to forward up
   * \param header IPv4 Header information
   * \param incomingInterface the Ipv4Interface on which the packet arrived
   * 
   * Called from lower-level layers to send the packet up
   * in the stack. 
   */
  virtual enum RxStatus Receive (Ptr<Packet> p,
                                 Ipv4Header const &header,
                                 Ptr<Ipv4Interface> incomingInterface) = 0;

  /**
   * \param p packet to forward up
   * \param header IPv6 Header information
   * \param incomingInterface the Ipv6Interface on which the packet arrived
   *
   * Called from lower-level layers to send the packet up
   * in the stack.
   */
  virtual enum RxStatus Receive (Ptr<Packet> p,
                                 Ipv6Header const &header,
                                 Ptr<Ipv6Interface> incomingInterface) = 0;

  /**
   * \param icmpSource the source address of the icmp message
   * \param icmpTtl the ttl of the icmp message
   * \param icmpType the 'type' field of the icmp message
   * \param icmpCode the 'code' field of the icmp message
   * \param icmpInfo extra information dependent on the icmp message
   *        generated by Icmpv4L4Protocol
   * \param payloadSource the source address of the packet which triggered
   *        the icmp message
   * \param payloadDestination the destination address of the packet which
   *        triggered the icmp message.
   * \param payload the first 8 bytes of the udp header of the packet
   *        which triggered the icmp message.
   */
  virtual void ReceiveIcmp (Ipv4Address icmpSource, uint8_t icmpTtl,
                            uint8_t icmpType, uint8_t icmpCode, uint32_t icmpInfo,
                            Ipv4Address payloadSource, Ipv4Address payloadDestination,
                            const uint8_t payload[8]);


  /**
   * \param icmpSource the source address of the icmp message
   * \param icmpTtl the ttl of the icmp message
   * \param icmpType the 'type' field of the icmp message
   * \param icmpCode the 'code' field of the icmp message
   * \param icmpInfo extra information dependent on the icmp message
   *        generated by Icmpv6L4Protocol
   * \param payloadSource the source address of the packet which triggered
   *        the icmp message
   * \param payloadDestination the destination address of the packet which
   *        triggered the icmp message.
   * \param payload the first 8 bytes of the udp header of the packet
   *        which triggered the icmp message.
   */
  virtual void ReceiveIcmp (Ipv6Address icmpSource, uint8_t icmpTtl,
                            uint8_t icmpType, uint8_t icmpCode, uint32_t icmpInfo,
                            Ipv6Address payloadSource, Ipv6Address payloadDestination,
                            const uint8_t payload[8]);

  typedef Callback<void,Ptr<Packet>, Ipv4Address, Ipv4Address, uint8_t, Ptr<Ipv4Route> > DownTargetCallback;
  typedef Callback<void,Ptr<Packet>, Ipv6Address, Ipv6Address, uint8_t, Ptr<Ipv6Route> > DownTargetCallback6;

  /**
   * This method allows a caller to set the current down target callback
   * set for this L4 protocol
   *
   * \param cb current Callback for the L4 protocol
   */
  virtual void SetDownTarget (DownTargetCallback cb) = 0;

  /**
   * This method allows a caller to set the current down target callback
   * set for this L4 protocol
   *
   * \param cb current Callback for the L4 protocol
   */
  virtual void SetDownTarget6 (DownTargetCallback6 cb) = 0;

  /**
   * This method allows a caller to get the current down target callback
   * set for this L4 protocol, for
   *
   * \return current Callback for the L4 protocol
   */
  virtual DownTargetCallback GetDownTarget (void) const = 0;

  /**
   * This method allows a caller to get the current down target callback
   * set for this L4 protocol, for
   *
   * \return current Callback for the L4 protocol
   */
  virtual DownTargetCallback6 GetDownTarget6 (void) const = 0;
};

} // Namespace ns3

#endif
