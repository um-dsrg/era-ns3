#include "topology-builder.h"

NS_LOG_COMPONENT_DEFINE ("TopologyBuilder");

/**
 * \brief      Shuffles the XML Link Elements in place.
 *
 * \param      linkElements  The link elements
 */
void
ShuffleLinkElements (std::vector<XMLElement *> &linkElements)
{
  // TODO Test that this is working
  std::random_shuffle (linkElements.begin (), linkElements.end ());
}
