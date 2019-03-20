#ifndef result_manager_h
#define result_manager_h

#include <string>
#include <tinyxml2.h>

#include "flow.h"
#include "application/app-container.h"

class ResultManager
{
public:
  ResultManager ();

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
#endif /* result_manager_h */
