/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
#include <tinyxml2.h>
#include <map>

// ns3 Includes
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

#include "ppfs-switch.h"
#include "topology-builder.h"
#include "routing-helper.h"

using namespace ns3;
using namespace tinyxml2;

NS_LOG_COMPONENT_DEFINE ("PPFS");

int
main (int argc, char *argv[])
{
  NS_LOG_UNCOND ("Per Packet Flow Splitting Simulator");

  // Parsing the XML file.
  XMLDocument xmlLogFile;
  XMLError error = xmlLogFile.LoadFile("/home/noel/Development/source-code/ns3/log.xml");
  NS_ABORT_MSG_IF(error != XML_SUCCESS, "Could not load LOG FILE");
  XMLNode* rootNode = xmlLogFile.LastChild();
  NS_ABORT_MSG_IF(rootNode == nullptr, "No root node node found");

  std::map<uint32_t, PpfsSwitch> switchMap; /*!< Key -> Node ID. Value -> Switch object */
  std::map <uint32_t, LinkInformation> linkInformation; /*!< Key -> Link ID, Value -> Link Info */
  NodeContainer allNodes; /*!< Node container storing all the nodes */
  NodeContainer terminalNodes; /*!< Node container storing a reference to the terminal nodes */
  NodeContainer switchNodes; /*!< Node container storing a reference to the switch nodes */
  TopologyBuilder topologyBuilder (rootNode, switchMap, allNodes, terminalNodes, switchNodes);

  topologyBuilder.CreateNodes ();
  topologyBuilder.ParseNodeConfiguration();
  topologyBuilder.BuildNetworkTopology (linkInformation);
  topologyBuilder.AssignIpToTerminals ();

  RoutingHelper routingHelper;
  routingHelper.PopulateRoutingTables(switchMap, linkInformation, allNodes, rootNode);
  routingHelper.SetReceiveFunctionForSwitches(switchNodes);

  // Simple test application!
  ns3::Ptr<Ipv4> nodeIp = allNodes.Get(8)->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  InetSocketAddress sinkSocket (nodeIp->GetAddress(1, 0).GetBroadcast(), 9);
  OnOffHelper onOff ("ns3::UdpSocketFactory", sinkSocket);
  onOff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onOff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  onOff.SetAttribute ("PacketSize", UintegerValue (1470));
  onOff.SetAttribute ("DataRate", DataRateValue(5000000)); // Data rate is set to 5Mbps
  onOff.SetAttribute("MaxBytes", UintegerValue (1470)); // 1 packet

  ApplicationContainer srcApp = onOff.Install(allNodes.Get(0));
  srcApp.Start(Seconds (1));
  srcApp.Stop(Seconds (10));

  // Configuring receiver
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkSocket);
  ApplicationContainer sinkApp = sinkHelper.Install(allNodes.Get(8));
  sinkApp.Start(Seconds (0));
  sinkApp.Stop(Seconds (10));

  Simulator::Run ();
  Simulator::Destroy ();
  // How to retrieve IP address of node
  //ns3::Ptr<Ipv4> nodeIp = allNodes.Get(0)->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  //std::cout << nodeIp->GetAddress (1, 0).GetLocal ().Get() << std::endl;

  return 0;
}
