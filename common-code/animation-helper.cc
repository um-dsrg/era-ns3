#include "ns3/mobility-helper.h"
#include "ns3/animation-interface.h"

#include "animation-helper.h"

using namespace ns3;
using namespace tinyxml2;

void
AnimationHelper::SetNodeMobilityAndCoordinates (XMLNode *rootNode, NodeContainer &allNodes)
{
  // Installing the constant mobility model on all the nodes.
  MobilityHelper mobilityHelper;
  mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityHelper.Install (allNodes);

  // Configuring the node co-ordinates
  XMLElement *nodeConfiguration = rootNode->FirstChildElement ("NodeConfiguration");
  NS_ABORT_MSG_IF (nodeConfiguration == nullptr, "NodeConfiguration element not found");

  XMLElement *nodeElement = nodeConfiguration->FirstChildElement ("Node");

  while (nodeElement != nullptr)
    {
      int x (0);
      int y (0);

      nodeElement->QueryAttribute ("X", &x);
      nodeElement->QueryAttribute ("Y", &y);

      /*
       * Inverting the y-axis because NetAnim has an inverted y-axis starts
       * from 0 and draws onward to the bottom not the top.
       */
      y = -y;

      uint32_t nodeId (0);
      nodeElement->QueryAttribute ("Id", &nodeId);
      AnimationInterface::SetConstantPosition (allNodes.Get (nodeId), x, y);

      nodeElement = nodeElement->NextSiblingElement ("Node");
    }
}

void
AnimationHelper::SetupAnimation (const std::string &animPath, NodeContainer &terminalNodes,
                                 NodeContainer &switchNodes)
{
  m_animation = std::unique_ptr<AnimationInterface> (new AnimationInterface (animPath));
  m_animation->EnablePacketMetadata ();

  for (auto terminal = terminalNodes.Begin (); terminal != terminalNodes.End (); ++terminal)
    m_animation->UpdateNodeColor ((*terminal), 255, 0, 0); // Terminals are red

  for (auto switchNode = switchNodes.Begin (); switchNode != switchNodes.End (); ++switchNode)
    m_animation->UpdateNodeColor ((*switchNode), 0, 0, 255); // Switches are blue
}
