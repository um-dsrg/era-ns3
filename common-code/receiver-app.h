#ifndef receiver_app_h
#define receiver_app_h

#include "ns3/applications-module.h"

#include "flow.h"

class ReceiverApp : public ns3::Application
{
public:
  ReceiverApp(const Flow& flow);
  virtual ~ReceiverApp();

private:
  virtual void StartApplication ();
  virtual void StopApplication ();
};

#endif /* receiver_app_h */
