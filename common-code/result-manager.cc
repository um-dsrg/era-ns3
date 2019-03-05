#include <chrono>
#include <sstream>
#include <iomanip>
#include <boost/numeric/conversion/cast.hpp>

#include "sdn-switch.h"
#include "ppfs-switch.h"
#include "receiver-app.h"
#include "result-manager.h"
#include "transmitter-app.h"
#include "single-path-receiver.h"
#include "single-path-transmitter.h"

using namespace tinyxml2;

NS_LOG_COMPONENT_DEFINE ("ResultManager");

ResultManager::ResultManager() {
    XMLNode* rootElement = m_xmlDoc.NewElement("Log");
    m_rootNode = m_xmlDoc.InsertFirstChild(rootElement);
    InsertTimeStamp();

    if (m_rootNode == nullptr) {
        throw std::runtime_error("Could not create element node");
    }
}

void ResultManager::AddGoodputResults(const ApplicationHelper::applicationContainer_t& receiverApplications) {
    NS_LOG_INFO("Building the goodput results");

    XMLElement* goodputElement = m_xmlDoc.NewElement("Goodput");

    for (const auto& receiverApplicationPair : receiverApplications) {
        auto flowId {receiverApplicationPair.first};

        Ptr<ApplicationBase> receiverApp;
        receiverApp = DynamicCast<ReceiverApp>(receiverApplicationPair.second);
        if (receiverApp == nullptr) {
            receiverApp = DynamicCast<SinglePathReceiver>(receiverApplicationPair.second);
        }

        XMLElement* flowElement = m_xmlDoc.NewElement("Flow");
        flowElement->SetAttribute("Id", flowId);
        flowElement->SetAttribute("MeanRxGoodPut", receiverApp->GetMeanRxGoodput());

        goodputElement->InsertEndChild(flowElement);
    }

    XMLComment* comment = m_xmlDoc.NewComment("Goodput values are in Mbps");
    goodputElement->InsertFirstChild(comment);

    m_rootNode->InsertEndChild(goodputElement);
}

void ResultManager::AddDelayResults(const ApplicationHelper::applicationContainer_t &transmitterApplications,
                                    const ApplicationHelper::applicationContainer_t &receiverApplications) {
    NS_LOG_INFO("Building the delay results");

    XMLElement* delayElement = m_xmlDoc.NewElement("Delay");

    for (const auto& receiverApplicationPair : receiverApplications) {
        auto flowId {receiverApplicationPair.first};
        NS_LOG_DEBUG("Calculating delay for flow " << flowId);

        XMLElement* flowElement = m_xmlDoc.NewElement("Flow");
        flowElement->SetAttribute("Id", flowId);

        Ptr<ApplicationBase> receiverApp;
        receiverApp = DynamicCast<ReceiverApp>(receiverApplicationPair.second);
        if (receiverApp == nullptr) {
            receiverApp = DynamicCast<SinglePathReceiver>(receiverApplicationPair.second);
        }

        Ptr<ApplicationBase> transmitterApp;
        transmitterApp = DynamicCast<TransmitterApp>(transmitterApplications.at(flowId));
        if (transmitterApp == nullptr) {
            transmitterApp = DynamicCast<SinglePathTransmitterApp>(transmitterApplications.at(flowId));
        }

        const auto& receiverDelayLog {receiverApp->GetDelayLog()};
        const auto& transmitterDelayLog {transmitterApp->GetDelayLog()};

        for (const auto& delayLogEntry : receiverDelayLog) {
            auto pktNumber {delayLogEntry.first};
            NS_LOG_DEBUG("Working on packet " << pktNumber);

            auto recvTime {delayLogEntry.second};
            auto transmittedTime {transmitterDelayLog.at(pktNumber)};
            auto delayInNs {(recvTime - transmittedTime).GetNanoSeconds()};

            XMLElement* packetElement = m_xmlDoc.NewElement("Packet");
            packetElement->SetAttribute("Number", boost::numeric_cast<unsigned int>(pktNumber));
            packetElement->SetAttribute("Delay", boost::numeric_cast<double>(delayInNs));
            flowElement->InsertEndChild(packetElement);
        }

        delayElement->InsertEndChild(flowElement);
    }

    XMLComment* comment = m_xmlDoc.NewComment("Delay values are in nanoseconds");
    delayElement->InsertFirstChild(comment);

    m_rootNode->InsertEndChild(delayElement);
}

void ResultManager::AddDelayResults(const ApplicationHelper& appHelper) {
    NS_LOG_INFO("Building the compressed delay results");

    XMLElement* delayElement = m_xmlDoc.NewElement("Delay");

    for (const auto& compressedDelayLog : appHelper.GetCompressedDelayLog()) {
        const auto& flowId = compressedDelayLog.first;
        const auto& delayLog = compressedDelayLog.second;

        XMLElement* flowElement {m_xmlDoc.NewElement("Flow")};
        flowElement->SetAttribute("Id", flowId);
        flowElement->SetAttribute("TotalRxPackets", static_cast<double>(delayLog.first));
        flowElement->SetAttribute("TotalDelayNs", delayLog.second);

        delayElement->InsertEndChild(flowElement);
    }

    XMLComment* comment = m_xmlDoc.NewComment("Delay values are in nanoseconds");
    delayElement->InsertFirstChild(comment);

    m_rootNode->InsertEndChild(delayElement);
}

void ResultManager::AddQueueStatistics(XMLElement* queueElement) {
    m_rootNode->InsertEndChild(queueElement);
}

void ResultManager::SaveFile(const std::string& path) {
    if (m_xmlDoc.SaveFile(path.c_str()) != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("Could not save the result file in " + path);
    }
}

void ResultManager::InsertTimeStamp () {
    using namespace std::chrono;
    time_point<system_clock> currentTime (system_clock::now ());
    std::time_t currentTimeFormatted = system_clock::to_time_t (currentTime);

    std::stringstream ss;
    ss << std::put_time (std::localtime (&currentTimeFormatted), "%a %d-%m-%Y %T");

    XMLElement* rootElement = m_rootNode->ToElement();
    rootElement->SetAttribute("Generated", ss.str().c_str());
}
