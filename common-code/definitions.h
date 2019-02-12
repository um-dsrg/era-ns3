#ifndef definitions_h
#define definitions_h

#include <cstdint>

using id_t = uint32_t;
using portNum_t = uint16_t;
using delay_t = double;
using dataRate_t = double;
using packetSize_t = uint32_t;
using packetNumber_t = uint64_t;
using splitRatio_t = double;

enum class NodeType : char { Switch = 'S', Terminal = 'T' };
enum class FlowProtocol : char { Tcp = 'T', Udp = 'U', Icmp = 'I', Undefined = 'X' };

// Structure definitions /////////////////////////////////////////////////////////
#include "ns3/nstime.h"
struct LinkStatistic /*!< Stores the link statistics */
{
  ns3::Time timeFirstTx;
  ns3::Time timeLastTx;
  uint32_t packetsTransmitted;
  uint32_t bytesTransmitted;

  LinkStatistic () : timeFirstTx (0), timeLastTx (0), packetsTransmitted (0), bytesTransmitted (0)
  {
  }
};

#endif /* definitions_h */
