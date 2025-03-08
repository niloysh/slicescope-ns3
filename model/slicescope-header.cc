#include "slicescope-header.h"

#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3
{

SlicescopeHeader::SlicescopeHeader()
    : m_dscp(0),
      m_bitmap(0)
{
}

void
SlicescopeHeader::SetDscp(uint8_t dscp)
{
    m_dscp = dscp;
}

uint8_t
SlicescopeHeader::GetDscp() const
{
    return m_dscp;
}

void
SlicescopeHeader::SetBitmap(uint8_t bitmap)
{
    m_bitmap = bitmap;
}

uint8_t
SlicescopeHeader::GetBitmap() const
{
    return m_bitmap;
}

TypeId
SlicescopeHeader::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SlicescopeHeader").SetParent<Header>().AddConstructor<SlicescopeHeader>();
    return tid;
}

TypeId
SlicescopeHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SlicescopeHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_dscp);
    start.WriteU8(m_bitmap);
}

uint32_t
SlicescopeHeader::Deserialize(Buffer::Iterator start)
{
    m_dscp = start.ReadU8();
    m_bitmap = start.ReadU8();
    return GetSerializedSize();
}

uint32_t
SlicescopeHeader::GetSerializedSize() const
{
    return 2;
}

void
SlicescopeHeader::Print(std::ostream& os) const
{
    os << "DSCP=" << static_cast<uint32_t>(m_dscp) << " Bitmap=" << static_cast<uint32_t>(m_bitmap);
}

} // namespace ns3