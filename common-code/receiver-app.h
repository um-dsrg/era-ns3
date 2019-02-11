#ifndef receiver_app_h
#define receiver_app_h

#include <list>
#include <queue>
#include <tuple>

#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/data-rate.h"
#include "ns3/ipv4-address.h"
#include "ns3/application.h"

#include "flow.h"

class ReceiverApp : public ns3::Application {
public:
    ReceiverApp(const Flow& flow);
    virtual ~ReceiverApp();

    double GetMeanRxGoodput();

private:
    ns3::Ptr<ns3::Socket> CreateSocket(ns3::Ptr<ns3::Node> node, FlowProtocol protocol);
    virtual void StartApplication();
    virtual void StopApplication();
    void HandleAccept(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from);
    void HandleRead(ns3::Ptr<ns3::Socket> socket);

    struct PathInformation {
        portNum_t dstPort;
        ns3::Ptr<ns3::Socket> rxListenSocket; /**< The socket to listen for incoming connections. */
        ns3::Address dstAddress; /**< The path's destination address. */
    };

    FlowProtocol protocol{FlowProtocol::Undefined};
    bool m_appRunning {false}; /**< Flag that represents the application's running state. */
    std::vector<PathInformation> m_pathInfoContainer;

    /**
     In the case of TCP, each socket accept returns a new socket, so the
     listening socket is stored separately from the accepted sockets.
     */
    std::list<ns3::Ptr<ns3::Socket> > m_rxAcceptedSockets; /**< List of the accepted sockets. */

    /* Goodput calculation related variables */
    uint64_t m_totalRecvBytes{0};
    bool m_firstPacketReceived{false};
    ns3::Time m_firstRxPacket{0};
    ns3::Time m_lastRxPacket{0};

    id_t m_id{0}; /**< The id of the flow that this application represents. */

    /* Buffer related variables */
    packetNumber_t m_expectedPacketNum{0};
    void popInOrderPacketsFromQueue();
    // TODO: Set this to a struct to make the code more readable, a class may be worthwhile because we need
    //to log this
    typedef std::pair<packetNumber_t, packetSize_t> bufferContents_t;
    std::priority_queue<bufferContents_t, std::vector<bufferContents_t>, std::greater<>> m_recvBuffer;
};

std::tuple<packetNumber_t, packetSize_t> ExtractPacketDetails(ns3::Ptr<ns3::Packet> packet);

#endif /* receiver_app_h */
