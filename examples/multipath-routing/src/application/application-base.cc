#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ppp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/udp-socket-factory.h"

#include "application-base.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ApplicationBase");

/*******************************************/
/* ApplicationBase                         */
/*******************************************/

const std::map<packetNumber_t, ns3::Time> &
ApplicationBase::GetDelayLog () const
{
  return m_delayLog;
}

ApplicationBase::ApplicationBase (id_t id) : m_id (id)
{
}

ApplicationBase::~ApplicationBase ()
{
}

Ptr<Socket>
ApplicationBase::CreateSocket (Ptr<Node> srcNode, FlowProtocol protocol)
{
  if (protocol == FlowProtocol::Tcp)
    { // Tcp Socket
      return Socket::CreateSocket (srcNode, TcpSocketFactory::GetTypeId ());
    }
  else if (protocol == FlowProtocol::Udp)
    { // Udp Socket
      return Socket::CreateSocket (srcNode, UdpSocketFactory::GetTypeId ());
    }
  else
    {
      NS_ABORT_MSG ("Cannot create non TCP/UDP socket");
    }
}

void
ApplicationBase::LogPacketTime (packetNumber_t packetNumber)
{
  m_delayLog.emplace (packetNumber, Simulator::Now ());
}

double
ApplicationBase::GetMeanRxGoodput ()
{
  NS_ABORT_MSG ("Error: The GetMeanRxGoodput function from ApplicationBase cannot "
                "be used directly");
  return 0.0;
}

double
ApplicationBase::GetTxGoodput ()
{
  NS_ABORT_MSG ("Error: The GetTxGoodput function from ApplicationBase cannot "
                "be used directly");
  return 0.0;
}

packetSize_t
ApplicationBase::CalculateHeaderSize (FlowProtocol protocol)
{
  packetSize_t headerSize{0};

  // Add TCP/UDP header size
  if (protocol == FlowProtocol::Tcp)
    {
      headerSize += 32; // 20 bytes header + 12 bytes options
    }
  else if (protocol == FlowProtocol::Udp)
    {
      headerSize += 8; // 8 bytes udp header
    }
  else
    {
      NS_ABORT_MSG ("Unknown protocol. Protocol " << static_cast<char> (protocol));
    }

  // Add Ip header size
  headerSize += Ipv4Header ().GetSerializedSize ();

  // Add Point To Point size
  headerSize += PppHeader ().GetSerializedSize ();

  return headerSize;
}

/*******************************************/
/* ReceiverBase                            */
/*******************************************/

ReceiverBase::ReceiverBase (id_t id) : ApplicationBase (id)
{
}

ReceiverBase::~ReceiverBase ()
{
}

double
ReceiverBase::GetMeanRxGoodput ()
{
  if (m_totalRecvBytes == 0)
    {
      return 0;
    }
  else
    {
      auto durationInSeconds = double{(m_lastRxPacket - m_firstRxPacket).GetSeconds ()};
      auto currentGoodPut = double{((m_totalRecvBytes * 8) / durationInSeconds) / 1'000'000};
      return currentGoodPut;
    }
}

/*******************************************/
/* TransmitterBase                         */
/*******************************************/

TransmitterBase::TransmitterBase (id_t id) : ApplicationBase (id)
{
}

TransmitterBase::~TransmitterBase ()
{
}

double
TransmitterBase::GetTxGoodput ()
{
  return m_dataRateBps / 1000000;
}

void
TransmitterBase::SetDataPacketSize (const Flow &flow)
{
  m_dataPacketSize = flow.packetSize - CalculateHeaderSize (flow.protocol);
}

void
TransmitterBase::SetApplicationGoodputRate (const Flow &flow)
{
  auto pktSizeExclHdr{flow.packetSize - CalculateHeaderSize (flow.protocol)};

  m_dataRateBps =
      (pktSizeExclHdr * flow.dataRate.GetBitRate ()) / static_cast<double> (flow.packetSize);
  NS_LOG_INFO ("Flow throughput: " << flow.dataRate << "\n"
                                   << "Flow goodput: " << m_dataRateBps << "bps");
}
