#ifndef application_base_h
#define application_base_h

#include <map>

#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/application.h"

#include "../flow.h"
#include "../definitions.h"
#include "../results-container.h"

class ApplicationBase : public ns3::Application
{
protected:
  ApplicationBase (id_t id);
  virtual ~ApplicationBase ();

  virtual void StartApplication () = 0;
  virtual void StopApplication () = 0;

  virtual void SetDataPacketSize (const Flow &flow);

  ns3::Ptr<ns3::Socket> CreateSocket (ns3::Ptr<ns3::Node> srcNode, FlowProtocol protocol);
  virtual packetSize_t CalculateHeaderSize (FlowProtocol protocol);
  uint32_t CalculateTcpBufferSize (const Flow &flow);
  uint32_t CalculateTcpBufferSize (const Path &path, packetSize_t packetSize);

  id_t m_id{0}; /**< The id of the flow that this application represents. */
  packetSize_t m_dataPacketSize{0}; /**< The data packet size in bytes. */
};

class ReceiverBase : public ApplicationBase
{
public:
  virtual ~ReceiverBase ();

protected:
  ReceiverBase (id_t id);
};

class TransmitterBase : public ApplicationBase
{
public:
  virtual ~TransmitterBase ();

protected:
  TransmitterBase (id_t id);
  virtual void TransmitPacket () = 0;

  void SetApplicationGoodputRate (const Flow &flow, ResultsContainer &resContainer);
  void SendPacket (ns3::Ptr<ns3::Socket> txSocket, ns3::Ptr<ns3::Packet> packet,
                   packetNumber_t pktNumber);

  double m_dataRateBps; /**< Application's bit rate in bps. */
  ns3::Time m_transmissionInterval; /**< The time between packet transmissions. */
  ns3::EventId m_sendEvent; /**< The Event Id of the pending TransmitPacket event. */
  packetNumber_t m_packetNumber{0};
};

#endif /* application_base_h */
