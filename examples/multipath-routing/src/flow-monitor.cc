#include <map>
#include <tinyxml2.h>
#include <boost/numeric/conversion/cast.hpp>

#include "ns3/ipv4-address.h"

#include "flow-monitor.h"

using ns3::Ipv4Address;

struct FlowDetails
{
  id_t flowId{0};
  id_t pathId{0};
};

std::pair<FlowDetails, bool>
GetMatchingFlowDetails (const Ipv4Address &srcAddress, const Ipv4Address &dstAddress,
                        portNum_t srcPort, portNum_t dstPort, const Flow::flowContainer_t &flows)
{
  FlowDetails flowDetails;
  auto flowFound{false};

  for (const auto &flowPair : flows)
    {
      const auto &flow{flowPair.second};

      auto srcIpAddress = Ipv4Address{flow.srcNode->GetIpAddress ()};
      auto dstIpAddress = Ipv4Address{flow.dstNode->GetIpAddress ()};

      // Check if there is a data path match
      if (srcIpAddress.IsEqual (srcAddress) && dstIpAddress.IsEqual (dstAddress))
        {
          for (const auto &dataPath : flow.GetDataPaths ()) // Search through the data paths
            {
              if (dataPath.srcPort == srcPort && dataPath.dstPort == dstPort)
                {
                  flowDetails.flowId = flowPair.first;
                  flowDetails.pathId = dataPath.id;
                  flowFound = true;
                  break;
                }
            }
        }

      // Check if there is an ack path match
      if (srcIpAddress.IsEqual (dstAddress) && dstIpAddress.IsEqual (srcAddress))
        {
          for (const auto &ackPath : flow.GetAckPaths ())
            {
              if (ackPath.srcPort == srcPort && ackPath.dstPort == dstPort)
                {
                  flowDetails.flowId = flowPair.first;
                  flowDetails.pathId = ackPath.id;
                  flowFound = true;
                  break;
                }
            }
        }
    }

  return std::make_pair (flowDetails, flowFound);
}

void
SaveFlowMonitorResultFile (ns3::FlowMonitorHelper &flowMonHelper,
                           const Flow::flowContainer_t &flows,
                           const std::string &flowMonitorOutputLocation)
{
  using boost::numeric_cast;
  using tinyxml2::XMLDocument;
  using tinyxml2::XMLError;
  using tinyxml2::XMLNode;

  try
    {
      XMLDocument flowMonDocument;

      XMLError parseResult =
          flowMonDocument.Parse (flowMonHelper.SerializeToXmlString (2, false, false).c_str ());

      if (parseResult != tinyxml2::XML_SUCCESS)
        {
          throw std::runtime_error ("Tinyxml was unable to parse the flow monitor results");
        }

      XMLNode *rootNode = flowMonDocument.FirstChild ();

      /**< Key: Flow Monitor Flow Id | Value: Pair (flow id, path id) */
      std::map<id_t, FlowDetails> flowMonMap;

      /* Build the Flow Id Map & update the Ipv4FlowClassifier element */

      auto flowClassifierElement = rootNode->FirstChildElement ("Ipv4FlowClassifier");
      auto flowElement = flowClassifierElement->FirstChildElement ("Flow");

      while (flowElement != nullptr)
        {
          auto flowId = id_t{0};
          auto tmpPort = int{0};

          flowElement->QueryAttribute ("flowId", &flowId);

          // Get flow's Ip address
          auto srcIpAddr = Ipv4Address{flowElement->Attribute ("sourceAddress")};
          auto dstIpAddr = Ipv4Address{flowElement->Attribute ("destinationAddress")};

          // Get flow's port numbers
          flowElement->QueryAttribute ("sourcePort", &tmpPort);
          auto srcPort = portNum_t{numeric_cast<portNum_t> (tmpPort)};

          flowElement->QueryAttribute ("destinationPort", &tmpPort);
          auto dstPort = portNum_t{numeric_cast<portNum_t> (tmpPort)};

          FlowDetails matchingFlowDetails;
          auto flowMatchFound = bool{false};

          std::tie (matchingFlowDetails, flowMatchFound) =
              GetMatchingFlowDetails (srcIpAddr, dstIpAddr, srcPort, dstPort, flows);

          if (!flowMatchFound)
            {
              throw std::runtime_error ("No match for Flow " + std::to_string (flowId) + " found");
            }

          // Update the flow element values
          flowElement->SetAttribute ("flowId", matchingFlowDetails.flowId);
          flowElement->SetAttribute ("pathId", matchingFlowDetails.pathId);

          // Save the mapping to update the FlowStats element
          flowMonMap.emplace (flowId, matchingFlowDetails);

          flowElement = flowElement->NextSiblingElement ("Flow");
        }

      /* Update the FlowStats element with the correct flow id */

      auto flowStatsElement = rootNode->FirstChildElement ("FlowStats");
      flowElement = flowStatsElement->FirstChildElement ("Flow");

      while (flowElement != nullptr)
        {
          auto fmFlowId = id_t{0};
          flowElement->QueryAttribute ("flowId", &fmFlowId);

          const auto &flowDetails = flowMonMap.at (fmFlowId);
          flowElement->SetAttribute ("flowId", flowDetails.flowId);
          flowElement->SetAttribute ("pathId", flowDetails.pathId);

          flowElement = flowElement->NextSiblingElement ("Flow");
        }

      // Save the XML file
      if (flowMonDocument.SaveFile (flowMonitorOutputLocation.c_str ()) != tinyxml2::XML_SUCCESS)
        {
          throw std::runtime_error ("Could not save the result file in " +
                                    flowMonitorOutputLocation);
        }
  } catch (const std::runtime_error &e)
    {
      std::cerr << e.what ()
                << "\nAn unmodified version of the Flow Monitor XML result file will be saved."
                << std::endl;
      flowMonHelper.SerializeToXmlFile (flowMonitorOutputLocation, false, false);
  }
}
