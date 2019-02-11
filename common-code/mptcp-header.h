#ifndef mptcp_header_h
#define mptcp_header_h

#include "ns3/header.h"

#include "definitions.h"

class MptcpHeader : public ns3::Header {
public:
    // must be implemented to become a valid new header.
    static ns3::TypeId GetTypeId (void);
    virtual ns3::TypeId GetInstanceTypeId (void) const;
    virtual uint32_t GetSerializedSize (void) const;
    virtual void Serialize (ns3::Buffer::Iterator start) const;
    virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
    virtual void Print (std::ostream &os) const;
    
    void SetPacketNumber (packetNumber_t packetNumber);
    packetNumber_t GetPacketNumber () const;
    
private:
    packetNumber_t m_packetNumber {0};
};

#endif /* mptcp_header_h */
