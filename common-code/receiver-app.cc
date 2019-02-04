#include "ns3/log.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/udp-socket-factory.h"

#include "receiver-app.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ReceiverApp");

ReceiverApp::ReceiverApp(const Flow& flow)
{
}

ReceiverApp::~ReceiverApp()
{
  // TODO: Set all the sockets to 0 here.
}

void ReceiverApp::StartApplication()
{
  NS_LOG_UNCOND("Receiver started");
}

void ReceiverApp::StopApplication()
{
  NS_LOG_UNCOND("Receiver stopped");
}
