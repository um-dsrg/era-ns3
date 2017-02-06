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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("OSPF-Network");

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

  return 0;
}
