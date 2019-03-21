#include <map>
#include <sstream>
#include <tinyxml2.h>

#include "ns3/config.h"
#include "ns3/boolean.h"
#include "ns3/queue-size.h"
#include "ns3/command-line.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-helper.h"

#include "src/sdn-switch.h"
#include "src/routing-helper.h"
#include "src/topology-builder.h"
#include "src/results-container.h"
#include "src/random-generator-manager.h"
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
  bool perPacketDelayLog{false};

  uint32_t initRun{1};
  uint32_t seedValue{1};

  std::string stopTime{""};
  std::string inputFile{""};
  std::string outputFile{""};
  std::string flowMonitorOutputFile{""};

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
                    "Enable delay logging on a per-packet level. "
                    "By default this feature is disabled due to "
                    "the high memory requirements.",
                    perPacketDelayLog);
  cmdLine.Parse (argc, argv);

  NS_ABORT_MSG_IF ((usePpfsSwitches == false && useSdnSwitches == false),
                   "No switch type is defined");

  if (verbose)
    {
      LogComponentEnable ("RoutingHelper", LOG_LEVEL_ALL);
      LogComponentEnable ("TopologyBuilder", LOG_LEVEL_ALL);
      LogComponentEnable ("ResultsContainer", LOG_LEVEL_ALL);
      /* Switches */
      LogComponentEnable ("SdnSwitch", LOG_LEVEL_ALL);
      LogComponentEnable ("PpfsSwitch", LOG_LEVEL_ALL);
      LogComponentEnable ("SwitchBase", LOG_LEVEL_ALL);
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

  // Create the nodes and build the topology
  TopologyBuilderBase *topologyBuilder{nullptr};
  if (useSdnSwitches)
    {
      topologyBuilder = new TopologyBuilder<SdnSwitch>;
    }
  else if (usePpfsSwitches)
    {
      topologyBuilder = new TopologyBuilder<PpfsSwitch>;
    }

  topologyBuilder->CreateNodes (rootNode);
  auto transmitOnLink{topologyBuilder->BuildNetworkTopology (rootNode)};
  topologyBuilder->AssignIpToTerminals ();
  topologyBuilder->EnablePacketReceptionOnSwitches ();

  // Parse the flows and build the routing table
  auto flows{topologyBuilder->ParseFlows (rootNode)};
  RoutingHelperBase *routingHelper{nullptr};

  if (useSdnSwitches)
    {
      routingHelper = new RoutingHelper<SdnSwitch>;
    }
  else if (usePpfsSwitches)
    {
      routingHelper = new RoutingHelper<PpfsSwitch>;
    }
  routingHelper->BuildRoutingTable (flows, transmitOnLink);

  // Reconcile the routing tables. Useful only for PPFS switches
  topologyBuilder->ReconcileRoutingTables ();

  ResultsContainer resContainer (flows);

  AppContainer appContainer;
  appContainer.InstallApplicationsOnTerminals (flows, topologyBuilder->GetTerminals (),
                                               usePpfsSwitches, resContainer);

  if (enablePcap)
    {
      PointToPointHelper myHelper;
      myHelper.EnablePcapAll ("ppfs-pcap", false);
    }

  // Setup Flow Monitor
  FlowMonitorHelper flowMonHelper;
  flowMonHelper.InstallAll ();

  // Set the simulation stop time
  Simulator::Stop (Time (stopTime));

  // Set the buffer size
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/"
               "$ns3::DropTailQueue<Packet>/MaxSize",
               QueueSizeValue (QueueSize ("1000000p")));

  // if (!perPacketDelayLog)
  //   { // Compress the delay log every 500ms
  //     Simulator::Schedule (Time ("500ms"), &AppContainer::CompressDelayLog, &appContainer);
  //   }

  Simulator::Run ();
  Simulator::Stop ();

  resContainer.AddFlowResults ();
  resContainer.SaveFile (outputFile);

  /* TODO Update the below
  ResultManager resultManager;
  resultManager.AddGoodputResults (flows, appContainer.GetTransmitterApps (),
                                   appContainer.GetReceiverApps ());
  if (perPacketDelayLog)
    {
      resultManager.AddDelayResults (appContainer.GetTransmitterApps (),
                                     appContainer.GetReceiverApps ());
    }
  else
    {
      resultManager.AddDelayResults (appContainer);
    }

  if (useSdnSwitches) {
      auto queueElement = topologyBuilder->GetSwitchQueueLoggingElement(resultManager.m_xmlDoc);
      resultManager.AddQueueStatistics(queueElement);
  }

  resultManager.SaveFile (outputFile); */

  // Save the flow monitor result file
  flowMonHelper.SerializeToXmlFile (flowMonitorOutputFile, false, false);

  Simulator::Destroy ();

  delete routingHelper;
  delete topologyBuilder;

  return 0;
}
