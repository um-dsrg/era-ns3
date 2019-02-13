#include "ppfs-switch.h"
#include "topology-builder.h"

NS_LOG_COMPONENT_DEFINE ("TopologyBuilder");

template <>
typename TopologyBuilder<PpfsSwitch>::PathPortMap
TopologyBuilder<PpfsSwitch>::AddDataPaths(Flow &flow, XMLElement *flowElement) {

    TopologyBuilder<PpfsSwitch>::PathPortMap pathPortMap;

    auto pathsElement = flowElement->FirstChildElement ("Paths");
    auto pathElement = pathsElement->FirstChildElement ("Path");

    Path dummyPath(true);

    while (pathElement != nullptr) {
        Path path(false);
        path.srcPort = dummyPath.srcPort;
        path.dstPort = dummyPath.dstPort;

        pathElement->QueryAttribute ("Id", &path.id);

        double dataRate;
        pathElement->QueryAttribute("DataRate", &dataRate);
        path.dataRate = ns3::DataRate(std::string{std::to_string(dataRate) + "Mbps"});

        auto linkElement = pathElement->FirstChildElement ("Link");
        while (linkElement != nullptr) {
            id_t linkId;
            linkElement->QueryAttribute ("Id", &linkId);
            path.AddLink (&m_links.at (linkId));
            linkElement = linkElement->NextSiblingElement ("Link");
        }

        auto ret = pathPortMap.emplace(path.id, std::make_pair(path.srcPort, path.dstPort));
        NS_ABORT_MSG_IF(ret.second == false, "Trying to insert a duplicate path " << path.id);

        flow.AddDataPath(path);
        pathElement = pathElement->NextSiblingElement ("Path");
    }

    return pathPortMap;
}

template <>
void TopologyBuilder<PpfsSwitch>::AddAckPaths(Flow& flow, XMLElement* flowElement,
                                              const TopologyBuilder<PpfsSwitch>::PathPortMap& pathPortMap) {

    auto ackShortestPathElement = flowElement->FirstChildElement("AckShortestPath");

    /**
     All data paths have the same source and destination port numbers; therefore,
     any path will do for this purpose.
     */
    Path dataPath = flow.GetDataPaths().front();
    Path ackPath(false);

    /* Swapping the port numbers for the ACK flows */
    ackPath.srcPort = dataPath.dstPort;
    ackPath.dstPort = dataPath.srcPort;

    auto linkElement = ackShortestPathElement->FirstChildElement("Link");
    while (linkElement != nullptr) {
        id_t linkId;
        linkElement->QueryAttribute ("Id", &linkId);
        ackPath.AddLink (&m_links.at (linkId));
        linkElement = linkElement->NextSiblingElement ("Link");
    }

    flow.AddAckShortestPath(ackPath);
}

/**
 Shuffles the XML Link elements in place.

 @param linkElements[in,out] Vector of link elements.
 */
void ShuffleLinkElements (std::vector<XMLElement *> &linkElements) {
    std::random_shuffle (linkElements.begin (), linkElements.end ());
}
