#include <boost/numeric/conversion/cast.hpp>

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ppp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/udp-socket-factory.h"

#include "../mptcp-header.h"
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

void
ApplicationBase::SetDataPacketSize (const Flow &flow)
{
  m_dataPacketSize = flow.packetSize - CalculateHeaderSize (flow.protocol);

  NS_LOG_INFO ("ApplicationBase - Packet size incl. headers: "
               << flow.packetSize << " excl. headers: " << m_dataPacketSize << "bytes");
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
 * @brief Calculates the required TCP buffer size for a given flow
 *
 * The path with the highest delay value is taken as the delay value.
 *
 * @param flow The flow
 */
uint32_t
ApplicationBase::CalculateTcpBufferSize (const Flow &flow, bool mstcpFlow)
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
  auto bdp{ceil ((flowDr * (rtt)) / 8)}; // The Bandwidth Delay Product (BDP) value in bytes

  return GetTcpBufferSizeFromBdp (bdp, mstcpFlow);
}

/**
 * @brief Calculates the required TCP buffer for a given path
 *
 * @param path The path
 * @param packetSize The packet size
 * @return The Tcp buffer size in bytes
 */
uint32_t
ApplicationBase::CalculateTcpBufferSize (const Path &path, bool mstcpFlow)
{
  auto pathDr = uint64_t{path.dataRate.GetBitRate ()};

  auto pathDelay{0.0};

  for (const auto &link : path.GetLinks ())
    {
      pathDelay += link->delay;
    }

  pathDelay /= 1000; // Convert from ms to Seconds
  auto rtt{pathDelay * 2}; // Round Trip Time in Seconds
  auto bdp{ceil ((pathDr * (rtt)) / 8)}; // The Bandwidth Delay Product (BDP) value in bytes

  return GetTcpBufferSizeFromBdp (bdp, mstcpFlow);
}

/**
 * @brief Calculates the TCP buffer size given the BDP value in terms of number
 * of packets.
 *
 * The TCP buffer is calculated such that the buffer size is in terms of the
 * number of packets it can hold. The buffer size is such that an integer number
 * of packets can fit in the buffer.
 *
 * If the calculated buffer size is too small, a minimum set value will be used
 * instead.
 *
 * @param bdp The BDP value in bytes
 * @param mstcpFlow Flag indicating whether the flow is an MSTCP flow or not
 * @return uint32_t The TCP buffer size in bytes
 */
uint32_t
ApplicationBase::GetTcpBufferSizeFromBdp (double bdp, bool mstcpFlow)
{
  using boost::numeric_cast;

  auto packetSize = m_dataPacketSize;
  if (mstcpFlow)
    {
      packetSize += MptcpHeader ().GetSerializedSize ();
    }

  auto actualBufferSize = uint32_t{0};

  try
    {
      // The 4096 minimum is used to replicate the value used by Linux
      auto minimumValue = numeric_cast<uint32_t> (ceil (4096.0 / packetSize) * packetSize);
      auto calculatedBufferSize = numeric_cast<uint32_t> (ceil (bdp / packetSize) * packetSize);

      // If the calculated buffer size is smaller than the minimum set value, the
      // minimumValue will be set.
      actualBufferSize = std::max (minimumValue, calculatedBufferSize);
  } catch (const boost::numeric::positive_overflow &e)
    {
      NS_ABORT_MSG ("GetTcpBufferSizeFromBdp - Calculating the TCP buffer size for Flow: "
                    << m_id << "caused a positive overflow.\nError message: " << e.what ());
  } catch (const boost::numeric::negative_overflow &e)
    {
      NS_ABORT_MSG ("GetTcpBufferSizeFromBdp - Calculating the TCP buffer size for Flow: "
                    << m_id << "caused a negative overflow.\nError message: " << e.what ());
  } catch (const boost::numeric::bad_numeric_cast &e)
    {
      NS_ABORT_MSG ("GetTcpBufferSizeFromBdp - Calculating the TCP buffer size for Flow: "
                    << m_id << "caused a bad numeric cast.\nError message: " << e.what ());
  }

  return actualBufferSize;
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
TransmitterBase::SetApplicationGoodputRate (const Flow &flow, ResultsContainer &resContainer)
{
  auto pktSizeExclHdr{flow.packetSize - CalculateHeaderSize (flow.protocol)};

  m_dataRateBps =
      (pktSizeExclHdr * flow.dataRate.GetBitRate ()) / static_cast<double> (flow.packetSize);
  NS_LOG_INFO ("Flow throughput: " << flow.dataRate << "\n"
                                   << "Flow goodput: " << m_dataRateBps << "bps");

  resContainer.LogFlowTxGoodputRate (flow.id, (m_dataRateBps / static_cast<double> (1000000)));
}
