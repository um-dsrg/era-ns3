#ifndef FLOW_H
#define FLOW_H

#include <map>
#include <vector>
#include <iostream>

#include "ns3/data-rate.h"
#include "ns3/ipv4-address.h"

#include "definitions.h"
#include "terminal.h"

struct Link
{
  id_t id;
  CustomDevice *srcNode;
  CustomDevice *dstNode;
  NodeType srcNodeType;
  NodeType dstNodeType;
  delay_t delay;
  dataRate_t capacity;
};

struct Path
{
  static portNum_t portNumCounter; //!< Global port number counter.

  Path ();
  void AddLink (Link const *link);
  const std::vector<Link const *> &GetLinks () const;
  friend std::ostream &operator<< (std::ostream &output, const Path &path);

  id_t id{0};
  portNum_t srcPort{0};
  portNum_t dstPort{0};
  ns3::DataRate dataRate;

private:
  std::vector<Link const *> m_links; //!< The link ids this path goes through.
};

struct Flow
{
  Flow () = default;
  void AddPath (const Path &path);
  const std::vector<Path> &GetPaths () const;

  bool operator< (const Flow &other) const;
  friend std::ostream &operator<< (std::ostream &output, const Flow &flow);

  id_t id{0};
  Terminal *srcNode{0};
  Terminal *dstNode{0};
  ns3::DataRate dataRate;
  packetSize_t packetSize;
  FlowProtocol protocol{FlowProtocol::Undefined};

  typedef std::map<id_t, Flow> FlowContainer;

private:
  std::vector<Path> m_paths; //!< The paths this flow is using.
};

#endif // FLOW_H
