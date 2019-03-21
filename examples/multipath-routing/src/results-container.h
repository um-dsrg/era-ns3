#ifndef results_container_h
#define results_container_h

#include <string>
#include <tinyxml2.h>

#include "flow.h"

class ResultsContainer
{
public:
  ResultsContainer ();

  // void AddGoodputResults (const Flow::FlowContainer &flows,
  //                         const AppContainer::applicationContainer_t &transmitterApplications,
  //                         const AppContainer::applicationContainer_t &receiverApplications);
  // void AddDelayResults (const AppContainer::applicationContainer_t &transmitterApplications,
  //                       const AppContainer::applicationContainer_t &receiverApplications);
  // void AddDelayResults (const AppContainer &appHelper);
  void AddQueueStatistics (tinyxml2::XMLElement *queueElement);
  void SaveFile (const std::string &path);

  // todo Put back to private
  tinyxml2::XMLDocument m_xmlDoc;

private:
  void InsertTimeStamp ();
  tinyxml2::XMLNode *m_rootNode;
};
#endif /* results_container_h */
