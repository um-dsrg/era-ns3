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
  NodeContainer allNodes;
  TopologyBuilder topologyBuilder (rootNode, switchMap, allNodes);

  topologyBuilder.CreateNodes ();
  topologyBuilder.BuildNetworkTopology ();
  return 0;
}
