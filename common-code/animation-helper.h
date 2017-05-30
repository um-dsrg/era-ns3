#ifndef ANIMATION_HELPER_H
#define ANIMATION_HELPER_H

#include <string>
#include <tinyxml2.h>
#include <memory>

#include "ns3/node-container.h"
#include "ns3/animation-interface.h"

class AnimationHelper
{
public:
  void SetNodeMobilityAndCoordinates (tinyxml2::XMLNode* rootNode, ns3::NodeContainer& allNodes);
  void SetupAnimation (const std::string& animPath, ns3::NodeContainer& terminalNodes,
                       ns3::NodeContainer& switchNodes);

private:
  std::unique_ptr<ns3::AnimationInterface> m_animation;
};

#endif /* ANIMATION_HELPER_H */
