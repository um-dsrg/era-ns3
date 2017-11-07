#ifndef APPLICATION_HELPER_H
#define APPLICATION_HELPER_H

#include <tinyxml2.h>
#include <map>

#include <ns3/node-container.h>
#include <ns3/ipv4.h>

#include "definitions.h"
#include "application-monitor.h"

class ApplicationHelper
{
public:
  ApplicationHelper (bool useUpdatedDataRates);

  void InstallApplicationOnTerminals (ApplicationMonitor& applicationMonitor,
                                      ns3::NodeContainer& allNodes,
                                      tinyxml2::XMLNode* rootNode);
private:
  /**
   * @brief      Parses the FlowDataRateModifications section in the XML result file
   *
   * The function will parse the FlowDataRateModifications section and store the flow
   * details in the m_updatedFlows map.
   *
   * @param      rootNode  The root node of the XML Result file
   */
  void ParseDataRateModifications (tinyxml2::XMLNode* rootNode);
  /**
   * @brief      Depending on how m_useUpdatedDataRates is set, returns the flow's
   *             data rate.
   *
   * @param[in]  flowId            The flow identifier
   * @param[in]  originalDataRate  The original data rate as given by the optimal solution
   *
   * @return     The flow data rate.
   */
  double GetFlowDataRate (FlowId_t flowId, double originalDataRate);

  inline uint32_t CalculateHeaderSize (char protocol);
  inline ns3::Ipv4Address GetIpAddress (NodeId_t nodeId, ns3::NodeContainer& nodes);

  /**
   * @brief      Stores the requested and received data rate of a particular flow
   */
  struct FlowDataRate
  {
    FlowDataRate () : requestedDataRate (0.0), receivedDataRate (0.0)
    {}

    double requestedDataRate; //!< The Data Rate the flow requested
    double receivedDataRate; //!< The Data Rate the optimal solution allocated this flow
  };

  std::map<FlowId_t, FlowDataRate> m_updatedFlows; //!< Stores the updated flow's data rates
  /**
   * True:  The applications will use the updated data rates from the optimal solution.
   * False: The applications will transmit their requested data rates irrelevant of
   *        what the optimal solution assigned to them.
   */
  bool m_useUpdatedDataRates;
};

#endif /* APPLICATION_HELPER_H */
