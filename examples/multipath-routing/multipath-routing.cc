#include <map>
#include <sstream>
#include <tinyxml2.h>

#include "ns3/config.h"
#include "ns3/boolean.h"
#include "ns3/queue-size.h"
#include "ns3/command-line.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-helper.h"

#include "src/flow.h"
#include "src/definitions.h"
#include "src/flow-monitor.h"
#include "src/routing-helper.h"
#include "src/topology-builder.h"
#include "src/results-container.h"
#include "src/random-generator-manager.h"
#include "src/device/switch/sdn-switch.h"
#include "src/application/app-container.h"

using namespace ns3;
using namespace tinyxml2;

int
main (int argc, char *argv[])
{

  bool useSack{false};
  bool verbose{false};
  bool enablePcap{false};
  bool useSdnSwitches{false};
  bool usePpfsSwitches{false};
  bool logPacketResults{false};
  bool perPacketDelayLog{false};
  bool logBufferSizeWithTime{false};

  uint32_t initRun{1};
  uint32_t seedValue{1};

  uint64_t switchBufferSize{100'000};

  std::string stopTime{""};
  std::string inputFile{""};
  std::string outputFile{""};
  std::string flowMonitorOutputFile{""};
  std::string transmitBufferRetrievalMethod {"InOrder"};

  // Set the command line parameters
  CommandLine cmdLine;
  cmdLine.AddValue ("verbose", "When set, output the log values", verbose);
  cmdLine.AddValue ("input", "The path to the input file", inputFile);
  cmdLine.AddValue ("output", "The path where to store the result file", outputFile);
  cmdLine.AddValue ("flowMonitorOutput", "The path where to store the generated flow monitor file",
                    flowMonitorOutputFile);
  cmdLine.AddValue ("stopTime",
                    "When set, the value in this string will represent the time"
                    "at which the simulation will stop. Time can be in the format of "
                    "XXh/min/s/ms/...",
                    stopTime);
  cmdLine.AddValue ("run", "The initial run value. Default of 1.", initRun);
  cmdLine.AddValue ("seed", "The seed used by the random number generator. Default of 1.",
                    seedValue);
  cmdLine.AddValue ("enablePcap", "Enable PCAP tracing on all devices", enablePcap);
  cmdLine.AddValue ("useSack", "Enable TCP Sack on all sockets", useSack);
  cmdLine.AddValue ("usePpfsSwitches", "Enable the use of PPFS switches", usePpfsSwitches);
  cmdLine.AddValue ("useSdnSwitches", "Enable the use of SDN switches", useSdnSwitches);
  cmdLine.AddValue ("perPacketDelayLog",
                    "Enable delay logging on a per-packet level. By default this feature is "
                    "disabled due to the high memory requirements.",
                    perPacketDelayLog);
  cmdLine.AddValue ("switchBufferSize",
                    "The buffer size in bytes each switch is equipped with. The default switch "
                    "buffer size is equal to 100,000bytes",
                    switchBufferSize);
  cmdLine.AddValue ("logPacketResults",
                    "When set, log all the packet results for each flow. Enabling this feature "
                    "might result in very large result files.",
                    logPacketResults);
  cmdLine.AddValue ("logBufferSizeWithTime",
                    "When set, log the time the MSTCP receiver buffer size changes.",
                    logBufferSizeWithTime);
  cmdLine.AddValue("txBufferRetrieval",
                   "The method to use for the Transmit Buffer retrieval. Available options are:"
                   "RoundRobin | AckPriority | InOrder", transmitBufferRetrievalMethod);
  cmdLine.Parse (argc, argv);

  if (verbose)
    {
      LogComponentEnable ("Flow", LOG_LEVEL_ALL);
      LogComponentEnable ("RoutingHelper", LOG_LEVEL_ALL);
      LogComponentEnable ("TopologyBuilder", LOG_LEVEL_ALL);
      LogComponentEnable ("ResultsContainer", LOG_LEVEL_ALL);
      /* Switches */
      LogComponentEnable ("SdnSwitch", LOG_LEVEL_ALL);
      LogComponentEnable ("PpfsSwitch", LOG_LEVEL_ALL);
      LogComponentEnable ("SwitchBase", LOG_LEVEL_ALL);
      LogComponentEnable ("ReceiveBuffer", LOG_LEVEL_ALL);
      LogComponentEnable ("TransmitBuffer", LOG_LEVEL_ALL);
      LogComponentEnable ("SwitchContainer", LOG_LEVEL_ALL);
      /* Applications */
      LogComponentEnable ("AppContainer", LOG_LEVEL_ALL);
      LogComponentEnable ("ApplicationBase", LOG_LEVEL_ALL);
      LogComponentEnable ("UnipathReceiver", LOG_LEVEL_ALL);
      LogComponentEnable ("MultipathReceiver", LOG_LEVEL_ALL);
      LogComponentEnable ("UnipathTransmitter", LOG_LEVEL_ALL);
      LogComponentEnable ("MultipathTransmitter", LOG_LEVEL_ALL);
    }

  // Set the seed and run values
  RandomGeneratorManager::SetSeed (seedValue);
  RandomGeneratorManager::SetRun (initRun);

  // Parse the XML file
  XMLDocument xmlInputFile;
  XMLError error = xmlInputFile.LoadFile (inputFile.c_str ());
  NS_ABORT_MSG_IF (error != XML_SUCCESS, "Could not load input file. File path: " << inputFile);
  XMLNode *rootNode = xmlInputFile.LastChild ();
  NS_ABORT_MSG_IF (rootNode == nullptr, "No root node node found");

  // Configure Selective Acknowledgements
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (useSack));

  // Set the switch type
  SwitchType switchType;
  if (usePpfsSwitches)
    switchType = SwitchType::PpfsSwitch;
  else if (useSdnSwitches)
    switchType = SwitchType::SdnSwitch;
  else
    NS_ABORT_MSG ("No switch type is defined");

  SwitchContainer switchContainer (switchBufferSize, transmitBufferRetrievalMethod);
  Terminal::terminalContainer_t terminalContainer;
  Link::linkContainer_t linkContainer;

  // Create an instance of the result container
  ResultsContainer resContainer (logPacketResults, logBufferSizeWithTime);

  // Create the nodes and build the topology
  TopologyBuilder topologyBuilder (switchType, switchContainer, terminalContainer, resContainer);

  topologyBuilder.CreateNodes (rootNode);
  auto transmitOnLink = topologyBuilder.BuildNetworkTopology (rootNode, linkContainer);
  topologyBuilder.AssignIpToTerminals ();

  // Parse the flows and build the routing table
  auto flows = ParseFlows (rootNode, terminalContainer, linkContainer, switchType);

  // Build the switch routing table
  BuildRoutingTable (flows, transmitOnLink);

  // Setup the switches for packet handling
  switchContainer.SetupSwitches();

  // Setup the results container
  resContainer.SetupFlowResults (flows);
  resContainer.SetupSwitchResults (switchContainer);

  AppContainer appContainer;
  appContainer.InstallApplicationsOnTerminals (flows, usePpfsSwitches, resContainer);

  if (enablePcap)
    {
      PointToPointHelper myHelper;
      myHelper.EnablePcapAll ("multipath-routing-pcap", false);
    }

  // Setup Flow Monitor
  FlowMonitorHelper flowMonHelper;
  flowMonHelper.InstallAll ();

  // Set the simulation stop time
  Simulator::Stop (Time (stopTime));

  std::cout << "Network Simulation in progress..." << std::endl;

  Simulator::Run ();
  Simulator::Stop ();

  std::cout << "Network Simulation Complete." << std::endl;

  // Add the flow and switch results to the XML file and save it
  resContainer.AddFlowResults ();
  resContainer.AddSwitchResults (switchContainer);

  resContainer.AddSimulationParameters (inputFile, outputFile, flowMonitorOutputFile, stopTime,
                                        enablePcap, useSack, usePpfsSwitches, useSdnSwitches,
                                        perPacketDelayLog, switchBufferSize, logPacketResults);
  resContainer.SaveFile (outputFile);

  // Save the flow monitor result file and try to update the flow ids to match ours
  SaveFlowMonitorResultFile (flowMonHelper, flows, flowMonitorOutputFile);

  Simulator::Destroy ();

  return 0;
}
