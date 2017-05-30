#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <cstdint>

// Variable definitions ///////////////////////////////////////////////////////
typedef uint32_t LinkId_t;
typedef uint32_t NodeId_t;
typedef uint32_t FlowId_t;

// Structure definitions /////////////////////////////////////////////////////////

#include "ns3/nstime.h"
struct LinkStatistic
{
  ns3::Time timeFirstTx;
  ns3::Time timeLastTx;
  uint32_t packetsTransmitted;
  uint32_t bytesTransmitted;

  LinkStatistic () : timeFirstTx (0), timeLastTx (0), packetsTransmitted (0), bytesTransmitted (0)
  {}
};

#endif /* DEFINITIONS_H */
