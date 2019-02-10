#include "topology-builder.h"

NS_LOG_COMPONENT_DEFINE ("TopologyBuilder");

/**
 Shuffles the XML Link elements in place.

 @param linkElements[in,out] Vector of link elements.
 */
void ShuffleLinkElements (std::vector<XMLElement *> &linkElements) {
    // TODO: Test that this is working
    std::random_shuffle (linkElements.begin (), linkElements.end ());
}
