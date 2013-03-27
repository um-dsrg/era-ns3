/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 University of Washington, 2012 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Allow ns-3 to ping a real host somewhere, using emulation mode and ping
// the simulated node from the host.
//
//   ------------------
//   | ns-3 simulation |
//   |                 |
//   |  -------        |
//   | | node  |       |
//   |  -------        |
//   | | fd-   |       |
//   | | net-  |       |
//   | | device|       |
//   |  -------        |
//   |   |             |
//   |   |             |
//   ----|-------------
//   |  ---       ---  |
//   | |   |     |   | |
//   | |TAP|     |ETH| |
//   | |   |     |   | |
//   |  ---       ---  |
//   |             |   |
//   |  host       |   |
//   --------------|----
//                 |
//                 |
//                 ---- (Internet) -------
//
// To use this example:
//  1) The ns-3 will create the TAP device for you in the host machine.
//     For this you need to provide the network address to allocate IP addresses
//     for the TAP/TU device and the ns-3 FdNetDevice.
//
//  2) Take into consideration that this experiment requires the host to be able to
//     forward the traffic generated by the simulation to the Internet.
//     So for Linux systems, make sure to configure:
//     # echo 1 > /proc/sys/net/ipv4/ip_forward
//
//  3) Once the experiment is running you can ping the FdNetDevice IP address from
//     the host machine.

#include "ns3/abort.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/fd-net-device-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TAPPingExample");

static void
PingRtt (std::string context, Time rtt)
{
  NS_LOG_UNCOND ("Received Response with RTT = " << rtt);
}

int
main (int argc, char *argv[])
{
  NS_LOG_INFO ("Ping Emulation Example with TAP");

  std::string deviceName ("tap0");
  std::string remote ("192.0.43.10"); // example.com
  std::string network ("1.2.3.4");
  std::string mask ("255.255.255.0");
  std::string pi ("no");

  //
  // Allow the user to override any of the defaults at run-time, via
  // command-line arguments
  //
  CommandLine cmd;
  cmd.AddValue ("deviceName", "Device name", deviceName);
  cmd.AddValue ("remote", "Remote IP address (dotted decimal only please)", remote);
  cmd.AddValue ("tapNetwork", "Network address to assign the TAP device IP address (dotted decimal only please)", network);
  cmd.AddValue ("tapMask", "Network mask for configure the TAP device (dotted decimal only please)", mask);
  cmd.AddValue ("modePi", "If 'yes' a PI header will be added to the traffic traversing the device(flag IFF_NOPI will be unset).", pi);
  cmd.Parse (argc, argv);

  NS_ABORT_MSG_IF (network == "1.2.3.4", "You must change the local IP address before running this example");

  Ipv4Address remoteIp (remote.c_str ());
  Ipv4Address tapNetwork (network.c_str ());
  Ipv4Mask tapMask (mask.c_str ());

  bool modePi = ( pi == "yes" ? true : false);

  //
  // Since we are using a real piece of hardware we need to use the realtime
  // simulator.
  //
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

  //
  // Since we are going to be talking to real-world machines, we need to enable
  // calculation of checksums in our protocols.
  //
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  //
  // In such a simple topology, the use of the helper API can be a hindrance
  // so we drop down into the low level API and do it manually.
  //
  // First we need a single node.
  //
  NS_LOG_INFO ("Create Node");
  Ptr<Node> node = CreateObject<Node> ();

  // Create an fd device, set a MAC address and point the device to the
  // Linux device name.  The device needs a transmit queueing discipline so
  // create a droptail queue and give it to the device.  Finally, "install"
  // the device into the node.
  //
  Ipv4AddressHelper addresses;
  addresses.SetBase (tapNetwork, tapMask);
  Ipv4Address tapIp = addresses.NewAddress ();

  NS_LOG_INFO ("Create Device");
  TapFdNetDeviceHelper helper;
  helper.SetDeviceName (deviceName);
  helper.SetModePi (modePi);
  helper.SetTapIpv4Address (tapIp);
  helper.SetTapIpv4Mask (tapMask);

  NetDeviceContainer devices = helper.Install (node);
  Ptr<NetDevice> device = devices.Get (0);

  //
  // Add a default internet stack to the node (ARP, IPv4, ICMP, UDP and TCP).
  //
  NS_LOG_INFO ("Add Internet Stack");
  InternetStackHelper internetStackHelper;
  internetStackHelper.Install (node);

  //
  // Add an address to the ns-3 device in the same network than one
  // assigned to the TAP.
  //
  NS_LOG_INFO ("Create IPv4 Interface");
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  uint32_t interface = ipv4->AddInterface (device);
  Ipv4Address devIp = addresses.NewAddress ();
  Ipv4InterfaceAddress address = Ipv4InterfaceAddress (devIp, tapMask);
  ipv4->AddAddress (interface, address);
  ipv4->SetMetric (interface, 1);
  ipv4->SetUp (interface);

  //
  // Add a route to the ns-3 device so it can reach the outside world though the
  // TAP.
  //
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4);
  staticRouting->SetDefaultRoute (tapIp, interface);

  //
  // Create the ping application.  This application knows how to send
  // ICMP echo requests.  Setting up the packet sink manually is a bit
  // of a hassle and since there is no law that says we cannot mix the
  // helper API with the low level API, let's just use the helper.
  //
  NS_LOG_INFO ("Create V4Ping Appliation");
  Ptr<V4Ping> app = CreateObject<V4Ping> ();
  app->SetAttribute ("Remote", Ipv4AddressValue (remoteIp));
  app->SetAttribute ("Verbose", BooleanValue (true) );
  node->AddApplication (app);
  app->SetStartTime (Seconds (1.0));
  app->SetStopTime (Seconds (21.0));

  //
  // Give the application a name.  This makes life much easier when constructing
  // config paths.
  //
  Names::Add ("app", app);

  //
  // Hook a trace to print something when the response comes back.
  //
  Config::Connect ("/Names/app/Rtt", MakeCallback (&PingRtt));

  //
  // Enable a promiscuous pcap trace to see what is coming and going on our device.
  //
  helper.EnablePcap ("fd-tap-ping", device, true);

  //
  // Now, do the actual emulation.
  //
  NS_LOG_INFO ("Run Emulation.");
  Simulator::Stop (Seconds (25.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
