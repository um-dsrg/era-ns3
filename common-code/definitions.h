#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <cstdint>

// Variable definitions ///////////////////////////////////////////////////////
typedef uint32_t id_t;
typedef uint16_t portNum_t;
typedef double delay_t;
typedef double dataRate_t;
typedef uint32_t packetSize_t;
typedef uint64_t packetNumber_t;
// TODO Remove the below typedefs
typedef uint32_t LinkId_t;
typedef uint32_t NodeId_t;
typedef uint32_t FlowId_t;

// Enum definitions /////////////////////////////////////////////////////////
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

#endif /* DEFINITIONS_H */
