#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/simulator.h"
#include "ns3/inet-socket-address.h"

#include "unipath-transmitter.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("UnipathTransmitter");

UnipathTransmitter::UnipathTransmitter (const Flow &flow, ResultsContainer &resContainer)
    : TransmitterBase (flow.id), m_resContainer (resContainer)
{
  auto path = flow.GetDataPaths ().front ();
  srcPort = path.srcPort;
  txSocket = CreateSocket (flow.srcNode->GetNode (), flow.protocol);
  txSocket->TraceConnectWithoutContext ("RTO",
                                        MakeCallback (&UnipathTransmitter::RtoChanged, this));
  dstAddress = Address (InetSocketAddress (flow.dstNode->GetIpAddress (), path.dstPort));

  // Set the data packet size
  SetDataPacketSize (flow);

  // Set the transmit path id to the first path
  const auto& dataPaths = flow.GetDataPaths();
  m_transmitPathId = dataPaths.front().id;

  // Set the application's good put rate in bps
  SetApplicationGoodputRate (flow, resContainer);

  // Calculate the transmission interval
  auto pktSizeBits = static_cast<double> (m_dataPacketSize * 8);
  double transmissionInterval = pktSizeBits / m_dataRateBps;
  NS_ABORT_MSG_IF (transmissionInterval <= 0 || std::isnan (transmissionInterval),
                   "The transmission interval cannot be less than or equal to 0 OR nan. "
                   "Transmission interval: "
                       << transmissionInterval);
  m_transmissionInterval = Seconds (transmissionInterval);
}

UnipathTransmitter::~UnipathTransmitter ()
{
  txSocket = nullptr;
}

void
UnipathTransmitter::StartApplication ()
{
  if (m_dataRateBps <= 1e-5)
    { // Do not transmit anything if not allocated any data rate
      NS_LOG_INFO ("Flow " << m_id << " did NOT start transmission on a single path");
      return;
    }

  NS_LOG_INFO (Simulator::Now().GetSeconds() << "s - UnipathTransmitter: Flow " << m_id <<
               " started transmission");

  InetSocketAddress srcAddr = InetSocketAddress (Ipv4Address::GetAny (), srcPort);
  if (txSocket->Bind (srcAddr) == -1)
    {
      NS_ABORT_MSG ("Failed to bind socket");
    }

  if (txSocket->Connect (dstAddress) == -1)
    {
      NS_ABORT_MSG ("Failed to connect socket");
    }

  TransmitPacket ();
}

void
UnipathTransmitter::StopApplication ()
{
  NS_LOG_INFO (Simulator::Now().GetSeconds() << "s - UnipathTransmitter: Flow " << m_id <<
               " stopped transmission");
  Simulator::Cancel (m_sendEvent);
}

void
UnipathTransmitter::TransmitPacket ()
{
  if (txSocket->GetTxAvailable () >= m_dataPacketSize)
    {
      auto packetNumber = m_packetNumber++;
      Ptr<Packet> packet = Create<Packet> (m_dataPacketSize);
      SendPacket (txSocket, packet, packetNumber);

      m_resContainer.LogPacketTransmission (m_id, Simulator::Now (), packetNumber,
                                            packet->GetSize (), m_transmitPathId, txSocket);
    }

  m_sendEvent =
      Simulator::Schedule (m_transmissionInterval, &UnipathTransmitter::TransmitPacket, this);
}

void
UnipathTransmitter::RtoChanged (ns3::Time oldVal, ns3::Time newVal)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds () << "s - Flow " << m_id << "RTO value change: " <<
               "Old Value " << oldVal.GetSeconds () << " New Value: " << newVal.GetSeconds ());
}
