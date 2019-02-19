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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

#include "../../common-code/sdn-switch.h"
#include "../../common-code/routing-helper.h"
#include "../../common-code/result-manager.h"
#include "../../common-code/topology-builder.h"
#include "../../common-code/application-helper.h"
#include "../../common-code/random-generator-manager.h"

using namespace ns3;
using namespace tinyxml2;

int main (int argc, char *argv[]) {
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

    if (verbose) {
        LogComponentEnable ("TopologyBuilder", LOG_LEVEL_ALL);
        LogComponentEnable ("SdnSwitch", LOG_LEVEL_ALL);
        LogComponentEnable ("SwitchBase", LOG_LEVEL_ALL);
        LogComponentEnable ("ApplicationMonitor", LOG_LEVEL_ALL);
        LogComponentEnable ("ApplicationHelper", LOG_LEVEL_ALL);
        LogComponentEnable ("ApplicationBase", LOG_LEVEL_ALL);
        LogComponentEnable ("ReceiverApp", LOG_LEVEL_ALL);
        LogComponentEnable ("SinglePathReceiverApp", LOG_LEVEL_ALL);
        LogComponentEnable ("TransmitterApp", LOG_LEVEL_ALL);
        LogComponentEnable ("SinglePathTransmitterApp", LOG_LEVEL_ALL);
        LogComponentEnable ("ResultManager", LOG_LEVEL_ALL);
    }

    // Setting the seed and run values
    RandomGeneratorManager::SetSeed (seed);
    RandomGeneratorManager::SetRun (initRun);

    // Parsing the XML file.
    XMLDocument xmlLogFile;
    //  XMLError error = xmlLogFile.LoadFile (xmlLogFilePath.c_str ());
    XMLError error = xmlLogFile.LoadFile("/Users/noel/Documents/Results/ns3/result_tcp.xml");
    NS_ABORT_MSG_IF (error != XML_SUCCESS, "Could not load LOG FILE");
    XMLNode *rootNode = xmlLogFile.LastChild ();
    NS_ABORT_MSG_IF (rootNode == nullptr, "No root node node found");

    // Create the nodes and build the topology
    TopologyBuilder<SdnSwitch> topologyBuilder;
    topologyBuilder.CreateNodes (rootNode);
    auto transmitOnLink{topologyBuilder.BuildNetworkTopology (rootNode)};
    topologyBuilder.AssignIpToTerminals ();
    topologyBuilder.EnablePacketReceptionOnSwitches();

    // Parse the flows and build the routing table
    auto flows{topologyBuilder.ParseFlows (rootNode)};
    RoutingHelper<SdnSwitch> routingHelper;
    routingHelper.BuildRoutingTable (flows, transmitOnLink);

    ApplicationHelper appHelper;
    appHelper.InstallApplicationsOnTerminals(flows, topologyBuilder.GetTerminals());

    PointToPointHelper myHelper;
    myHelper.EnablePcapAll ("ppfs-pcap", false);

    Simulator::Run();
    Simulator::Stop();

    ResultManager resultManager;
    resultManager.AddGoodputResults(appHelper.GetReceiverApps());
    resultManager.AddDelayResults(appHelper.GetTransmitterApps(), appHelper.GetReceiverApps());
    resultManager.SaveFile("/Users/noel/Documents/Results/ns3/result_ns3.xml");

    Simulator::Destroy();

    return 0;
}
