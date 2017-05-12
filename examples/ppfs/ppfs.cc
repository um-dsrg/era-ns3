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
#include "application-helper.h"
#include "animation-helper.h"
#include "result-manager.h"

using namespace ns3;
using namespace tinyxml2;

NS_LOG_COMPONENT_DEFINE ("PPFS");

int
main (int argc, char *argv[])
{
  NS_LOG_UNCOND ("Per Packet Flow Splitting Simulator");

  // TODO: Add cmdLine parameters!

  // We need to enable logging here!
  LogComponentEnable ("RoutingHelper", LOG_LEVEL_INFO);
  LogComponentEnable("PpfsSwitch", LOG_LEVEL_INFO);
  LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);

  // Parsing the XML file.
  XMLDocument xmlLogFile;
  XMLError error = xmlLogFile.LoadFile("/home/noel/Development/source-code/ns3/log.xml");
  NS_ABORT_MSG_IF(error != XML_SUCCESS, "Could not load LOG FILE");
  XMLNode* rootNode = xmlLogFile.LastChild();
  NS_ABORT_MSG_IF(rootNode == nullptr, "No root node node found");

  NodeContainer allNodes; /*!< Node container storing all the nodes */
  NodeContainer terminalNodes; /*!< Node container storing a reference to the terminal nodes */
  NodeContainer switchNodes; /*!< Node container storing a reference to the switch nodes */

  std::map<uint32_t, PpfsSwitch> switchMap; /*!< Key -> Node ID. Value -> Switch object */
  std::map <uint32_t, LinkInformation> linkInformation; /*!< Key -> Link ID, Value -> Link Info */

  TopologyBuilder topologyBuilder (rootNode, switchMap, allNodes, terminalNodes, switchNodes);
  topologyBuilder.CreateNodes ();
  topologyBuilder.ParseNodeConfiguration();
  topologyBuilder.BuildNetworkTopology (linkInformation);
  topologyBuilder.AssignIpToTerminals ();

  RoutingHelper routingHelper (switchMap);
  routingHelper.PopulateRoutingTables(linkInformation, allNodes, rootNode);
  routingHelper.SetReceiveFunctionForSwitches(switchNodes);

  AnimationHelper animHelper;
  animHelper.SetNodeMobilityAndCoordinates(rootNode, allNodes);
  animHelper.SetupAnimation("/home/noel/Development/source-code/ns3/animation.xml", terminalNodes,
                            switchNodes);

  ApplicationHelper applicationHelper;
  uint32_t stopTime = applicationHelper.InstallApplicationOnTerminals(allNodes, rootNode);

  ResultManager resultManager;
  resultManager.SetupFlowMonitor(allNodes, stopTime);

  Simulator::Stop(Seconds(stopTime));
  Simulator::Run ();

  resultManager.GenerateFlowMonitorXmlLog();
  resultManager.UpdateFlowIds(rootNode, allNodes);
  resultManager.AddQueueStatistics(switchMap);
  resultManager.SaveXmlResultFile("/home/noel/Development/source-code/ns3/results.xml");

  Simulator::Destroy ();

  return 0;
}
