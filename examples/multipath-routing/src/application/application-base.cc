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

/**
 * @brief Calculates the required TCP buffer size
 *
 * @param flow The flow
 */
uint32_t
ApplicationBase::CalculateTcpBufferSize (const Flow &flow)
{
  uint64_t flowDr{flow.dataRate.GetBitRate ()};

  delay_t maxPathDelay{0.0};

  for (const auto &path : flow.GetDataPaths ())
    {
      auto pathDelay{0.0};

      for (const auto &link : path.GetLinks ())
        {
          pathDelay += link->delay;
        }
      maxPathDelay = std::max (maxPathDelay, pathDelay);
    }

  maxPathDelay /= 1000; // Convert from ms to Seconds
  auto rtt{maxPathDelay * 2}; // Round Trip Time in Seconds
  auto bdp{ceil ((flowDr * (rtt)) / 8)}; // The Bandwidth Delay Product value in bytes

  return bdp;
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

/*******************************************/
/* TransmitterBase                         */
/*******************************************/

TransmitterBase::TransmitterBase (id_t id) : ApplicationBase (id)
{
}

TransmitterBase::~TransmitterBase ()
{
}

void
TransmitterBase::SendPacket (ns3::Ptr<ns3::Socket> txSocket, ns3::Ptr<ns3::Packet> packet,
                             packetNumber_t pktNumber)
{
  auto numBytesSent = txSocket->Send (packet);

  if (numBytesSent == -1)
    {
      std::stringstream ss;
      ss << "Flow " << m_id << " Packet " << pktNumber << " failed to transmit. Packet size "
         << packet->GetSize () << "\n";
      auto error = txSocket->GetErrno ();

      switch (error)
        {
        case Socket::SocketErrno::ERROR_NOTERROR:
          ss << "ERROR_NOTERROR";
          break;
        case Socket::SocketErrno::ERROR_ISCONN:
          ss << "ERROR_ISCONN";
          break;
        case Socket::SocketErrno::ERROR_NOTCONN:
          ss << "ERROR_NOTCONN";
          break;
        case Socket::SocketErrno::ERROR_MSGSIZE:
          ss << "ERROR_MSGSIZE";
          break;
        case Socket::SocketErrno::ERROR_AGAIN:
          ss << "ERROR_AGAIN";
          break;
        case Socket::SocketErrno::ERROR_SHUTDOWN:
          ss << "ERROR_SHUTDOWN";
          break;
        case Socket::SocketErrno::ERROR_OPNOTSUPP:
          ss << "ERROR_OPNOTSUPP";
          break;
        case Socket::SocketErrno::ERROR_AFNOSUPPORT:
          ss << "ERROR_AFNOSUPPORT";
          break;
        case Socket::SocketErrno::ERROR_INVAL:
          ss << "ERROR_INVAL";
          break;
        case Socket::SocketErrno::ERROR_BADF:
          ss << "ERROR_BADF";
          break;
        case Socket::SocketErrno::ERROR_NOROUTETOHOST:
          ss << "ERROR_NOROUTETOHOST";
          break;
        case Socket::SocketErrno::ERROR_NODEV:
          ss << "ERROR_NODEV";
          break;
        case Socket::SocketErrno::ERROR_ADDRNOTAVAIL:
          ss << "ERROR_ADDRNOTAVAIL";
          break;
        case Socket::SocketErrno::ERROR_ADDRINUSE:
          ss << "ERROR_ADDRINUSE";
          break;
        case Socket::SocketErrno::SOCKET_ERRNO_LAST:
          ss << "SOCKET_ERRNO_LAST";
          break;
        }

      NS_ABORT_MSG (ss.str ());
    }
}

void
TransmitterBase::SetDataPacketSize (const Flow &flow)
{
  m_dataPacketSize = flow.packetSize - CalculateHeaderSize (flow.protocol);
}

void
TransmitterBase::SetApplicationGoodputRate (const Flow &flow, ResultsContainer &resContainer)
{
  auto pktSizeExclHdr{flow.packetSize - CalculateHeaderSize (flow.protocol)};

  m_dataRateBps =
      (pktSizeExclHdr * flow.dataRate.GetBitRate ()) / static_cast<double> (flow.packetSize);
  NS_LOG_INFO ("Flow throughput: " << flow.dataRate << "\n"
                                   << "Flow goodput: " << m_dataRateBps << "bps");

  resContainer.LogFlowTxGoodputRate (flow.id, (m_dataRateBps / static_cast<double> (1000000)));
}
