#ifndef flow_h
#define flow_h

#include <map>
#include <vector>
#include <iostream>
#include <tinyxml2.h>

#include "ns3/data-rate.h"
#include "ns3/ipv4-address.h"

#include "definitions.h"
#include "device/terminal/terminal.h"

struct Link
{
  using linkContainer_t = std::map<id_t, Link>;

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
  explicit Path () = default;

  static portNum_t GeneratePortNumber ();
  void AddLink (Link const *link);
  const std::vector<Link const *> &GetLinks () const;
  friend std::ostream &operator<< (std::ostream &output, const Path &path);

  id_t id{0};
  portNum_t srcPort{0};
  portNum_t dstPort{0};
  ns3::DataRate dataRate;

private:
  static portNum_t portNumCounter; //!< Global port number counter.
  std::vector<Link const *> m_links; //!< The link ids this path goes through.
};

struct Flow
{
  using flowContainer_t = std::map<id_t, Flow>;

  explicit Flow () = default;

  void AddDataPath (const Path &path);
  const std::vector<Path> &GetDataPaths () const;

  void AddAckPath (const Path &path);
  const std::vector<Path> &GetAckPaths () const;

  bool operator< (const Flow &other) const;
  friend std::ostream &operator<< (std::ostream &output, const Flow &flow);

  id_t id{0};
  Terminal *srcNode{0};
  Terminal *dstNode{0};
  ns3::DataRate dataRate;
  packetSize_t packetSize;
  FlowProtocol protocol{FlowProtocol::Undefined};

private:
  std::vector<Path> m_dataPaths; /**< The data paths this flow is using */
  std::vector<Path> m_ackPaths; /**< The ack paths this flow is using */
};

Flow::flowContainer_t ParseFlows (tinyxml2::XMLNode *rootNode,
                                  Terminal::terminalContainer_t &terminalContainer,
                                  Link::linkContainer_t &linkContainer);

#endif /* flow_h */
