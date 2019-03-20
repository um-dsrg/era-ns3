#ifndef application_base_h
#define application_base_h

#include <map>

#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/application.h"

#include "../flow.h"
#include "../definitions.h"

class ApplicationBase : public ns3::Application
{
public:
  // TODO: All of these need to be removed from here!
  const std::map<packetNumber_t, ns3::Time> &GetDelayLog () const;
  virtual double GetMeanRxGoodput ();
  virtual double GetTxGoodput ();
  std::map<packetNumber_t, ns3::Time> m_delayLog;

protected:
  ApplicationBase (id_t id);
  virtual ~ApplicationBase ();

  virtual void StartApplication () = 0;
  virtual void StopApplication () = 0;

  ns3::Ptr<ns3::Socket> CreateSocket (ns3::Ptr<ns3::Node> srcNode, FlowProtocol protocol);
  // This needs to be removed from here!
  void LogPacketTime (packetNumber_t packetNumber);

  virtual packetSize_t CalculateHeaderSize (FlowProtocol protocol);

  id_t m_id{0}; /**< The id of the flow that this application represents. */
};

class ReceiverBase : public ApplicationBase
{
public:
  double GetMeanRxGoodput ();

  virtual ~ReceiverBase ();

protected:
  ReceiverBase (id_t id);

  /* Goodput calculation related variables */
  uint64_t m_totalRecvBytes{0};
  bool m_firstPacketReceived{false};
  ns3::Time m_firstRxPacket{0};
  ns3::Time m_lastRxPacket{0};
};

class TransmitterBase : public ApplicationBase
{
public:
  double GetTxGoodput ();

  virtual ~TransmitterBase ();

protected:
  TransmitterBase (id_t id);

  virtual void TransmitPacket () = 0;

  void SetDataPacketSize (const Flow &flow);
  void SetApplicationGoodputRate (const Flow &flow);

  double m_dataRateBps; /**< Application's bit rate in bps. */
  ns3::Time m_transmissionInterval; /**< The time between packet transmissions. */
  packetSize_t m_dataPacketSize{0}; /**< The data packet size in bytes. */
  ns3::EventId m_sendEvent; /**< The Event Id of the pending TransmitPacket event. */
  packetNumber_t m_packetNumber{0};
};

#endif /* application_base_h */
