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
// Standard C++ Includes
#include <iostream>
#include <vector>
#include <map>
#include <libgen.h>

// LEMON Includes
#include <lemon/smart_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include <lemon/graph_to_eps.h>

// ns3 Includes
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

#include "../multipath-mcf/classes/type-defs.h"
#include "../multipath-mcf/classes/log-manager.h"
#include "../multipath-mcf/classes/graph-utilities.h"
#include "../multipath-mcf/classes/commodity-utilities.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("OSPF-Network");

Ipv4Address
GetNodeIpAddress (Ptr<Node> node, uint32_t interface = 1, uint32_t addressIndex = 0)
{
  Ptr<Ipv4> nodeIp = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  return nodeIp->GetAddress (interface, addressIndex).GetLocal ();
}

int
main (int argc, char *argv[])
{
  NS_LOG_UNCOND ("OSPF Network simulator");

  CommandLine cmdLine;

  bool verbose = false;
  std::string lgfFile ("");
  std::string resultsDir ("");
  std::string resultsFileName ("");
  std::string logDir ("");
  std::string logFileName ("");
  std::string epsFile ("");
  std::string animFile ("");

  cmdLine.AddValue("verbose", "If true output log values", verbose);
  cmdLine.AddValue("lgfFile", "The full path to the LGF file", lgfFile);
  cmdLine.AddValue("resultsDir", "The directory where to store the results", resultsDir);
  cmdLine.AddValue("resultsFileName", "The name + extension of the results file name", resultsFileName);
  cmdLine.AddValue("logDir", "The directory where to store the logs", logDir);
  cmdLine.AddValue("logFileName", "The log's file name", logFileName);
  cmdLine.AddValue("animFile", "The path where to save the animation xml file. When blank it is disabled"
                   , animFile);
  cmdLine.AddValue("epsFile", "The path where to output the EPS file. If blank nothing will be output"
                   , epsFile);

  cmdLine.Parse (argc,argv);

  if (verbose) // Enable logging if verbose is set to true
    {
      LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    }

  // Check the validity of the command line arguments.
  NS_ABORT_MSG_IF(lgfFile.empty(), "LGF file location must be specified");
  NS_ABORT_MSG_IF(resultsDir.empty(), "Results directory must be specified");
  NS_ABORT_MSG_IF(resultsFileName.empty(), "Results file name must be specified");
  NS_ABORT_MSG_IF(logDir.empty(), "Log directory must be specified");
  NS_ABORT_MSG_IF(logFileName.empty(), "Log file name must be specified");

  LogManager logManager (logDir, logFileName);
  logManager.LogLgfFileLocation(lgfFile);
  logManager.LogResultsFile(resultsDir + resultsFileName);
  logManager.SaveLog();

  // FIXME: Add animation + eps conversion capabilities

  GraphUtilities graphUtilities (lgfFile); // Load and parse the LGF file
  if(!epsFile.empty())
    {
      graphUtilities.ExportGraphToEps(epsFile);
      logManager.LogEpsExport(epsFile);
    }

  uint32_t headerSize (30);
  CommodityUtilities commodityUtilities (graphUtilities, headerSize);
  commodityUtilities.LoadCommoditiesFromFile(lgfFile);

  // Creating the terminal and switch nodes
  NodeContainer nodes;
  nodes.Create(graphUtilities.GetNumberOfSwitches() + graphUtilities.GetNumberOfTerminals());

  // Installing internet on all the nodes
  InternetStackHelper internetStack;
  internetStack.InstallAll();

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("1.0.0.0", "255.0.0.0");

  PointToPointHelper p2pHelper;

  NetworkTopology_t& networkTopology = graphUtilities.GetNetworkTopology();
  // Loop through the network topology and setup the connections
  for (NetworkTopologyRevIt_t it = networkTopology.rbegin();
       it != networkTopology.rend();
       ++it)
    {
      // Configuring the PointToPoint channel Data rate (bits per second)
      p2pHelper.SetDeviceAttribute ("DataRate", DataRateValue (it->GetEdgeCapacity() * 1000000));
      // Configuring the PointToPoint channel delay
      p2pHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(it->GetEdgeDelay())));

      Id_t srcNodeId = it->GetSourceId();
      Id_t sinkNodeId = it->GetSinkId();

      NetDeviceContainer link = p2pHelper.Install (nodes.Get(srcNodeId), nodes.Get(sinkNodeId));
      ipv4.Assign(link);
      // Assigning new network address per node due to the fact that OSPF routes networks not
      // specific IP Addresses.
      ipv4.NewNetwork ();
    }

  // Building the routing table
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // Installing the OnOff application
  NS_LOG_INFO ("Installing Applications on terminals");

  std::string socketType = "ns3::UdpSocketFactory";

  double maxEndTime = 0.0;

  for (CommodityUtilities::CommodityIt_t commodity = commodityUtilities.CommoditiesBegin();
       commodity != commodityUtilities.CommoditiesEnd();
       ++commodity)
    {
      // NOTE: This code assumes that the terminal only has 1 IP Address. I.E only one link
      // is entering the terminal. For now this should suffice. If the need changes, this
      // code needs to change as well.

      // Binding the IP Address with the port number.
      // This address signifies the destination IP Address + port number
      // We need the ip address of the receiver.

      // Get the IP Address of the receiver node.
      Ipv4Address sinkIpAddress = GetNodeIpAddress (nodes.Get(commodity->GetSinkId()));
      InetSocketAddress sinkSocket (sinkIpAddress, commodity->GetPortNumber());

      uint32_t packetSize = commodity->GetPacketSize ();
      double dataRateInBps = commodity->GetDataRate () * 1000000;
      uint32_t maxBytes = packetSize * commodity->GetNumOfPackets ();

      // Configure the Transmitter on the Source Node /////////////////////////
      OnOffHelper onOff (socketType, sinkSocket);
      onOff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      onOff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      onOff.SetAttribute ("PacketSize", UintegerValue (packetSize));
      onOff.SetAttribute ("DataRate", DataRateValue(dataRateInBps)); // Data rate is set in Bps
      onOff.SetAttribute("MaxBytes", UintegerValue (maxBytes));

      ApplicationContainer srcApps = onOff.Install (nodes.Get (commodity->GetSourceId()));
      srcApps.Start (Seconds (commodity->GetTransmissionStartTime()));
      srcApps.Stop (Seconds (commodity->GetTransmissionEndTime()));

      // Configure the Receiver on the Sink Node //////////////////////////////
      PacketSinkHelper packetSinkHelper (socketType, sinkSocket);
      ApplicationContainer packetSinkApp = packetSinkHelper.Install (nodes.Get (commodity->GetSinkId()));
      packetSinkApp.Start (Seconds (commodity->GetTransmissionStartTime()));
      packetSinkApp.Stop (Seconds (commodity->GetTransmissionEndTime()));

      if (commodity->GetTransmissionEndTime() > maxEndTime)
        maxEndTime = commodity->GetTransmissionEndTime();

      // Log entry for OnOff application
      NS_LOG_INFO("OnOff installed on node " << commodity->GetSourceId()
                  << "\n  Dst Ip: " << sinkSocket.GetIpv4().Get()
                  << " Dst Port: " << sinkSocket.GetPort() << "\n  "
                  << "Packet size: " << packetSize
                  << "bytes" << " @ " << commodity->GetDataRate () << "Mbps\n  "
                  << "Maximum Bytes to transfer " << maxBytes << "\n  "
                  << "No. of Packets " << commodity->GetNumOfPackets ());

      // Log entry for the packet sink helper
      NS_LOG_INFO("PacketSink installed on node " << commodity->GetSinkId()
                  << "\n  "
                  << "Dst Ip: " << sinkSocket.GetIpv4().Get()
                  << " Port Number: " << sinkSocket.GetPort());
    }

  // Enable Flow Monitor
  FlowMonitorHelper flowMonitorHelper;
  // Installing flow monitor on all the nodes.
  Ptr<FlowMonitor> flowMonitor = flowMonitorHelper.InstallAll();

  maxEndTime += 10; // Giving a 10 seconds buffer.
  Simulator::Stop(Seconds (maxEndTime));
  Simulator::Run();

  Simulator::Destroy();
  return 0;
}
