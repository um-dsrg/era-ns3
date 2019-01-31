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
#include <sstream>
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
#include "../../common-code/application-monitor.h"
#include "../../common-code/animation-helper.h"
#include "../../common-code/result-manager.h"
#include "../../common-code/random-generator-manager.h"

#include "ppfs-switch.h"

using namespace ns3;
using namespace tinyxml2;

int
main (int argc, char *argv[])
{
  bool verbose{false};
  std::string xmlLogFilePath{""};
  std::string xmlResultFilePath{""};
  std::string xmlAnimationFile{""};
  uint32_t queuePacketSize{100};
  uint32_t seed{1};
  uint32_t initRun{1};
  bool enableHistograms{false};
  bool enableFlowProbes{false};
  bool enablePcapTracing{false};
  uint64_t appBytesQuota{0};
  bool ignoreOptimalDataRates{false};
  bool logGoodputEveryPacket{false};
  std::string stopTime{""};
  uint32_t nPacketsPerFlow{0};

  CommandLine cmdLine;
  cmdLine.AddValue ("verbose", "If true display log values", verbose);
  cmdLine.AddValue ("seed", "The seed used by the random number generator. Default of 1.", seed);
  cmdLine.AddValue ("run", "The initial run value. Default of 1.", initRun);
  cmdLine.AddValue ("log", "The full path to the XML log file", xmlLogFilePath);
  cmdLine.AddValue ("result", "The full path of the result file", xmlResultFilePath);
  cmdLine.AddValue ("animation",
                    "The full path where to store the animation xml file."
                    "If left blank animation will be disabled.",
                    xmlAnimationFile);
  cmdLine.AddValue ("queuePacketSize",
                    "The maximum number of packets a queue can store."
                    "The value is 100",
                    queuePacketSize);
  cmdLine.AddValue ("enableHistograms",
                    "If set enable FlowMonitor's delay and jitter"
                    " histograms. By default they are disabled",
                    enableHistograms);
  cmdLine.AddValue ("enableFlowProbes",
                    "If set enable FlowMonitor's flow probes."
                    "By default they are disabled",
                    enableFlowProbes);
  cmdLine.AddValue ("enablePcapTracing",
                    "If set enable Pcap Tracing."
                    " By default this is disabled",
                    enablePcapTracing);
  cmdLine.AddValue ("appBytesQuota",
                    "The number of bytes each application must receive"
                    "before simulation can terminate. Default of 0.",
                    appBytesQuota);
  cmdLine.AddValue ("ignoreOptimalDataRates",
                    "When set, will ignore the optimal data rates"
                    "and every flow will transmit at its original requested data rate."
                    "Default false.",
                    ignoreOptimalDataRates);
  cmdLine.AddValue ("logGoodputEveryPacket",
                    "When set, log the goodput for each packet an"
                    "application receives and store it an an XML file. Default false.",
                    logGoodputEveryPacket);
  cmdLine.AddValue ("stopTime",
                    "When set, the value in this string will represent the time"
                    "at which the simulation will stop. Time can be in the format of "
                    "XXh/min/s/ms/...",
                    stopTime);
  cmdLine.AddValue ("nPacketsPerFlow",
                    "The number of packets each flow will transmit."
                    "Default 0.",
                    nPacketsPerFlow);

  cmdLine.Parse (argc, argv);

  if (verbose)
    {
      LogComponentEnable ("TopologyBuilder", LOG_LEVEL_INFO);
      LogComponentEnable ("PpfsSwitch", LOG_LEVEL_INFO);
      LogComponentEnable ("SwitchDevice", LOG_LEVEL_INFO);
      LogComponentEnable ("ApplicationMonitor", LOG_LEVEL_INFO);
      LogComponentEnable ("ApplicationHelper", LOG_LEVEL_INFO);
      LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
      LogComponentEnable ("ResultManager", LOG_LEVEL_INFO);
    }

  // Setting the seed and run values
  RandomGeneratorManager::SetSeed (seed);
  RandomGeneratorManager::SetRun (initRun);

  // Parsing the XML file.
  XMLDocument xmlLogFile;
  XMLError error = xmlLogFile.LoadFile (xmlLogFilePath.c_str ());
  NS_ABORT_MSG_IF (error != XML_SUCCESS, "Could not load LOG FILE");
  XMLNode *rootNode = xmlLogFile.LastChild ();
  NS_ABORT_MSG_IF (rootNode == nullptr, "No root node node found");

  TopologyBuilder<PpfsSwitch> topologyBuilder;
  topologyBuilder.CreateNodes (rootNode);

  // NodeContainer allNodes; /*!< Node container storing all the nodes */
  // NodeContainer terminalNodes; /*!< Node container storing a reference to the terminal nodes */
  // NodeContainer switchNodes; /*!< Node container storing a reference to the switch nodes */
  // NetDeviceContainer terminalDevices; /*!< Container storing all the terminal's net devices */

  // std::map<NodeId_t, PpfsSwitch> switchMap; /*!< Key -> Node ID. Value -> Switch object */
  // std::map<LinkId_t, LinkInformation> linkInformation; /*!< Key -> Link ID, Value -> Link Info */
  // /*
  //  * This map will be used for statistics purposes when we need to store the link id that a
  //  * terminal's net device is connected to. This will be used when the simulation is running
  //  * to calculate the link statistics.
  //  */
  // std::map<Ptr<NetDevice>, LinkId_t> terminalToLinkId; /*!< Key -> Net Device, Value -> Link Id */

  // TopologyBuilder<PpfsSwitch> topologyBuilder (rootNode, switchMap, terminalToLinkId, allNodes,
  //                                              terminalNodes, switchNodes, terminalDevices, false);
  // topologyBuilder.CreateNodes ();
  // topologyBuilder.ParseNodeConfiguration ();
  // topologyBuilder.BuildNetworkTopology (linkInformation);
  // topologyBuilder.SetSwitchRandomNumberGenerator ();
  // topologyBuilder.AssignIpToNodes ();

  // RoutingHelper<PpfsSwitch> routingHelper (switchMap);
  // routingHelper.PopulateRoutingTables (linkInformation, allNodes, rootNode);
  // routingHelper.SetSwitchesPacketHandler ();

  // std::unique_ptr<AnimationHelper> animHelper;

  // if (!xmlAnimationFile.empty ())
  //   {
  //     animHelper = std::unique_ptr<AnimationHelper> (new AnimationHelper ());
  //     animHelper->SetNodeMobilityAndCoordinates (rootNode, allNodes);
  //     animHelper->SetupAnimation (xmlAnimationFile, terminalNodes, switchNodes);
  //   }

  // ApplicationMonitor applicationMonitor (appBytesQuota, logGoodputEveryPacket);
  // ApplicationHelper applicationHelper (ignoreOptimalDataRates);
  // applicationHelper.InstallApplicationOnTerminals (applicationMonitor, allNodes, nPacketsPerFlow,
  //                                                  rootNode);

  // ResultManager resultManager;
  // resultManager.SetupFlowMonitor (allNodes);
  // resultManager.TraceTerminalTransmissions (terminalDevices, terminalToLinkId);

  // Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/MaxSize",
  //              QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queuePacketSize)));

  // if (enablePcapTracing) // Enable PCAP tracing if the command line parameter was set
  //   {
  //     PointToPointHelper myHelper;
  //     myHelper.EnablePcapAll ("ppfs-pcap", false);
  //   }

  // if (!stopTime.empty ())
  //   {
  //     NS_LOG_UNCOND ("Simulation to stop at " << stopTime);
  //     Simulator::Stop (Time (stopTime));
  //   }

  // Simulator::Run ();
  // Simulator::Stop ();

  // resultManager.GenerateFlowMonitorXmlLog (enableHistograms, enableFlowProbes);
  // resultManager.AddApplicationMonitorResults (applicationMonitor);
  // resultManager.UpdateFlowIds (rootNode, allNodes);
  // resultManager.AddQueueStatistics (switchMap);
  // resultManager.AddLinkStatistics (switchMap);
  // resultManager.AddSwitchDetails (switchMap);
  // resultManager.SaveXmlResultFile (xmlResultFilePath.c_str ());
  // // Storing the goodput for each flow per packet (only if this feature is enabled)
  // resultManager.SavePerPacketGoodPutResults (xmlResultFilePath, applicationMonitor);

  // Simulator::Destroy ();

  return 0;
}