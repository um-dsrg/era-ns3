#include "flow.h"

/**
 * Path Implementation
 */

/**
 * The global port number counter starting from 49152 as it is the start of the range of
 * dynamic/private port numbers. This was done to avoid any conflicts with any other
 * standards.
 */
portNum_t Path::portNumCounter = 49152;

Path::Path (bool autoAssignPortNumbers) {
    if (autoAssignPortNumbers) {
        srcPort = portNumCounter++;
        dstPort = portNumCounter++;
    }
}

void Path::AddLink (Link const *link) {
    m_links.push_back (link);
}

const std::vector<Link const *> & Path::GetLinks () const {
    return m_links;
}

std::ostream & operator<< (std::ostream &output, const Path &path) {
    output << "Path ID: " << path.id << "\n";
    output << "Data Rate: " << path.dataRate << "\n";
    output << "Source Port: " << path.srcPort << "\n";
    output << "Destination Port: " << path.dstPort << "\n";
    output << "Links \n";
    
    for (const auto link : path.m_links) {
        output << "Link ID: " << link->id << "\n";
    }
    
    return output;
}

/**
 * Flow Implementation
 */

Flow::Flow() : m_ackShortestPath(/* disable auto port number generation */ false) {
}

void Flow::AddDataPath(const Path& path) {
    m_dataPaths.push_back(path);
}

const std::vector<Path>& Flow::GetDataPaths() const {
    return m_dataPaths;
}

void Flow::AddAckPath(const Path &path) {
    m_ackPaths.push_back(path);
}

const std::vector<Path>& Flow::GetAckPaths() const {
    return m_ackPaths;
}

void Flow::AddAckShortestPath(const Path &path) {
    m_ackShortestPath = path;
}

const Path& Flow::GetAckShortestPath() const {
    return m_ackShortestPath;
}

bool Flow::operator< (const Flow &other) const {
    /*
    * Used by the map to store the keys in order.
    * In this case it is sorted by Source Ip Address, then Destination Ip Address.
    */
    if (srcNode->GetIpAddress().Get() == other.srcNode->GetIpAddress().Get()) {
    return dstNode->GetIpAddress().Get() < other.dstNode->GetIpAddress().Get();
    } else {
    return srcNode->GetIpAddress().Get() < other.srcNode->GetIpAddress().Get();
    }
}

std::ostream & operator<< (std::ostream &output, const Flow &flow) {
    ns3::Ipv4Address address;
    output << "Flow ID: " << flow.id << "\n";
    output << "Data Rate: " << flow.dataRate << "\n";
    output << "Packet Size: " << flow.packetSize << "bytes\n";
    output << "Source IP Addr: " << flow.srcNode->GetIpAddress().Get() << "\n";
    output << "Destination IP Addr: " << flow.dstNode->GetIpAddress().Get() << "\n";
    output << "Protocol: " << static_cast<char> (flow.protocol) << "\n";
    output << "----------------------\n";

    if (!flow.m_dataPaths.empty()) {
        output << "Data Paths\n---\n";
        for (const auto& dataPath : flow.m_dataPaths) {
            output << dataPath;
            output << "---\n";
        }
    }

    if (!flow.m_ackPaths.empty()) {
        output << "Ack Paths\n---\n";
        for (const auto& ackPath : flow.m_ackPaths) {
            output << ackPath;
            output << "---\n";
        }
    }
    return output;
}
