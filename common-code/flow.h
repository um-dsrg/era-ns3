#include <vector>
#include <iostream>
#include "ns3/ipv4-address.h"

#include "definitions.h"
#include "terminal.h"

struct Path
{
  static portNum_t portNumCounter; //!< Global port number counter.

  Path ();
  void AddLink (id_t linkId);

  friend std::ostream &operator<< (std::ostream &output, Path &path);

  id_t id{0};
  portNum_t srcPort{0};
  portNum_t dstPort{0};
  dataRate_t dataRate{0.0}; // FIXME Update the data rate when parsing the result file.

private:
  std::vector<id_t> links; //!< The link ids this path goes through.
};

struct Flow
{
  Flow () = default;
  void AddPath (const Path &path);

  bool operator< (const Flow &other) const;
  friend std::ostream &operator<< (std::ostream &output, Flow &value);

  id_t id{0};
  Terminal *srcNode{0};
  Terminal *dstNode{0};
  FlowProtocol protocol{FlowProtocol::Undefined};
  ns3::Ipv4Address srcAddress; // TODO May remove this
  ns3::Ipv4Address dstAddress; // TODO May remove this

private:
  std::vector<Path> m_paths; //!< The paths this flow is using.
};
