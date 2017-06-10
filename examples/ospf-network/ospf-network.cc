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

#include "../../common-code/topology-builder.h"
#include "../../common-code/routing-helper.h"
#include "../../common-code/application-helper.h"
#include "../../common-code/animation-helper.h"
#include "../../common-code/result-manager.h"

#include "ospf-switch.h"

using namespace ns3;
using namespace tinyxml2;

NS_LOG_COMPONENT_DEFINE ("ospf-network");

int
main (int argc, char *argv[])
{
  NS_LOG_UNCOND ("OSPF network simulator");

  bool verbose (false);
  std::string xmlLogFilePath ("");
  std::string xmlResultFilePath ("");
  std::string xmlAnimationFile ("");
  uint32_t queuePacketSize (100);
  bool enableHistograms (false);
  bool enableFlowProbes (false);

  CommandLine cmdLine;
  cmdLine.AddValue("verbose", "If true display log values", verbose);
  cmdLine.AddValue("log", "The full path to the XML log file", xmlLogFilePath);
  cmdLine.AddValue("result", "The full path of the result file", xmlResultFilePath);
  cmdLine.AddValue("animation", "The full path where to store the animation xml file."
                   "If left blank animation will be disabled.", xmlAnimationFile);
  cmdLine.AddValue("queuePacketSize", "The maximum number of packets a queue can store."
                   "The value is 100", queuePacketSize);
  cmdLine.AddValue("enableHistograms", "If set enable FlowMonitor's delay and jitter histograms."
                   "By default they are disabled", enableHistograms);
  cmdLine.AddValue("enableFlowProbes", "If set enable FlowMonitor's flow probes."
                   "By default they are disabled", enableFlowProbes);

  cmdLine.Parse(argc, argv);

  if (verbose)
    {
      LogComponentEnable ("OspfSwitch", LOG_LEVEL_INFO);
      LogComponentEnable ("SwitchDevice", LOG_LEVEL_INFO);
      LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
      LogComponentEnable ("ResultManager", LOG_LEVEL_INFO);
    }

  // TODO: verify that the result and log file parameters are passed correctly

  // Parsing the XML file.
  XMLDocument xmlLogFile;
  XMLError error = xmlLogFile.LoadFile(xmlLogFilePath.c_str());
  NS_ABORT_MSG_IF(error != XML_SUCCESS, "Could not load LOG FILE");
  XMLNode* rootNode = xmlLogFile.LastChild();
  NS_ABORT_MSG_IF(rootNode == nullptr, "No root node node found");

  NodeContainer allNodes; /*!< Node container storing all the nodes */
  NodeContainer terminalNodes; /*!< Node container storing a reference to the terminal nodes */
  NodeContainer switchNodes; /*!< Node container storing a reference to the switch nodes */
  NetDeviceContainer terminalDevices; /*!< Container storing all the terminal's net devices */

  std::map<NodeId_t, OspfSwitch> switchMap; /*!< Key -> Node ID. Value -> Switch object */
  std::map <LinkId_t, LinkInformation> linkInformation; /*!< Key -> Link ID, Value -> Link Info */
  /*
   * This map will be used for statistics purposes when we need to store the link id that a
   * terminal's net device is connected to. This will be used when the simulation is running
   * to calculate the link statistics.
   */
  std::map <Ptr<NetDevice>, LinkId_t> terminalToLinkId; /*!< Key -> Net Device, Value -> Link Id */

  TopologyBuilder<OspfSwitch> topologyBuilder (rootNode, switchMap, terminalToLinkId, allNodes,
                                               terminalNodes, switchNodes, terminalDevices);
  topologyBuilder.CreateNodes ();
  topologyBuilder.ParseNodeConfiguration();
  topologyBuilder.BuildNetworkTopology (linkInformation);
  topologyBuilder.AssignIpToNodes(true, true);

  RoutingHelper<OspfSwitch> routingHelper (switchMap);
  routingHelper.PopulateRoutingTables(allNodes, rootNode);
  routingHelper.SetSwitchesPacketHandler();

  std::unique_ptr<AnimationHelper> animHelper;

  if (!xmlAnimationFile.empty())
    {
      animHelper = std::unique_ptr<AnimationHelper> (new AnimationHelper());
      animHelper->SetNodeMobilityAndCoordinates(rootNode, allNodes);
      animHelper->SetupAnimation(xmlAnimationFile, terminalNodes, switchNodes);
    }

  ApplicationHelper applicationHelper;
  uint32_t stopTime = applicationHelper.InstallApplicationOnTerminals(allNodes, rootNode);

  ResultManager resultManager;
  resultManager.SetupFlowMonitor(allNodes, stopTime);
  resultManager.TraceTerminalTransmissions(terminalDevices, terminalToLinkId);

  Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/MaxPackets",
               UintegerValue (queuePacketSize));

  // Building the routing table
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Simulator::Stop(Seconds(stopTime));
  Simulator::Run ();

  resultManager.GenerateFlowMonitorXmlLog(enableHistograms, enableFlowProbes);
  resultManager.UpdateFlowIds(rootNode, allNodes);
  resultManager.AddQueueStatistics(switchMap);
  resultManager.AddLinkStatistics(switchMap);
  resultManager.SaveXmlResultFile(xmlResultFilePath.c_str());

  Simulator::Destroy ();

  return 0;
}
