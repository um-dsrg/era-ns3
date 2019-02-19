#include <map>
#include <sstream>
#include <tinyxml2.h>

#include "ns3/command-line.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-helper.h"

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
    bool enablePcap{false};

    uint32_t initRun{1};
    uint32_t seedValue{1};

    std::string stopTime{""};
    std::string inputFile{""};
    std::string outputFile{""};
    std::string flowMonitorOutpuFile{""};

    // Set the command line parameters
    CommandLine cmdLine;
    cmdLine.AddValue("verbose", "When set, output the log values", verbose);
    cmdLine.AddValue("input", "The path to the input file", inputFile);
    cmdLine.AddValue("output", "The path where to store the result file", outputFile);
    cmdLine.AddValue("flowMonitorOutput", "The path where to store the generated flow monitor file", flowMonitorOutpuFile);
    cmdLine.AddValue("stopTime",
                     "When set, the value in this string will represent the time"
                     "at which the simulation will stop. Time can be in the format of "
                     "XXh/min/s/ms/...",
                     stopTime);
    cmdLine.AddValue("run", "The initial run value. Default of 1.", initRun);
    cmdLine.AddValue("seed", "The seed used by the random number generator. Default of 1.", seedValue);
    cmdLine.AddValue("enablePcap", "Enable PCAP tracing on all devices", enablePcap);
    // TODO Add whether to use the PPFS switch or not
    cmdLine.Parse (argc, argv);

    if (verbose) {
        LogComponentEnable("SdnSwitch", LOG_LEVEL_ALL);
        LogComponentEnable("SwitchBase", LOG_LEVEL_ALL);
        LogComponentEnable("ReceiverApp", LOG_LEVEL_ALL);
        LogComponentEnable("ResultManager", LOG_LEVEL_ALL);
        LogComponentEnable("TransmitterApp", LOG_LEVEL_ALL);
        LogComponentEnable("ApplicationBase", LOG_LEVEL_ALL);
        LogComponentEnable("TopologyBuilder", LOG_LEVEL_ALL);
        LogComponentEnable("ApplicationHelper", LOG_LEVEL_ALL);
        LogComponentEnable("ApplicationMonitor", LOG_LEVEL_ALL);
        LogComponentEnable("SinglePathReceiverApp", LOG_LEVEL_ALL);
        LogComponentEnable("SinglePathTransmitterApp", LOG_LEVEL_ALL);
    }

    // Set the seed and run values
    RandomGeneratorManager::SetSeed(seedValue);
    RandomGeneratorManager::SetRun(initRun);

    // Parse the XML file
    XMLDocument xmlInputFile;
    XMLError error = xmlInputFile.LoadFile(inputFile.c_str());
    NS_ABORT_MSG_IF (error != XML_SUCCESS, "Could not load LOG FILE");
    XMLNode *rootNode = xmlInputFile.LastChild();
    NS_ABORT_MSG_IF (rootNode == nullptr, "No root node node found");

    // Create the nodes and build the topology
    TopologyBuilder<SdnSwitch> topologyBuilder;
    topologyBuilder.CreateNodes(rootNode);
    auto transmitOnLink{topologyBuilder.BuildNetworkTopology(rootNode)};
    topologyBuilder.AssignIpToTerminals();
    topologyBuilder.EnablePacketReceptionOnSwitches();

    // Parse the flows and build the routing table
    auto flows{topologyBuilder.ParseFlows(rootNode)};
    // TODO Remove the class definition and replace it with a template function only
    RoutingHelper<SdnSwitch> routingHelper;
    routingHelper.BuildRoutingTable(flows, transmitOnLink);

    ApplicationHelper appHelper;
    appHelper.InstallApplicationsOnTerminals(flows, topologyBuilder.GetTerminals());

    if(enablePcap) {
        PointToPointHelper myHelper;
        myHelper.EnablePcapAll("ppfs-pcap", false);
    }

    // Setup Flow Monitor
    FlowMonitorHelper flowMonHelper;
    flowMonHelper.InstallAll();

    Simulator::Run();
    Simulator::Stop();

    ResultManager resultManager;
    resultManager.AddGoodputResults(appHelper.GetReceiverApps());
    resultManager.AddDelayResults(appHelper.GetTransmitterApps(), appHelper.GetReceiverApps());
    resultManager.SaveFile(outputFile.c_str());

    // Save the flow monitor result file
    flowMonHelper.SerializeToXmlFile(flowMonitorOutpuFile, false, false);

    Simulator::Destroy();

    return 0;
}
