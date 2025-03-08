#ifndef SLICESCOPE_HEADER_H
#define SLICESCOPE_HEADER_H

#include "ns3/header.h"

namespace ns3
{

class SlicescopeHeader : public Header
{
  public:
    SlicescopeHeader();

    void SetDscp(uint8_t dscp);
    uint8_t GetDscp() const;

    void SetBitmap(uint8_t bitmap);
    uint8_t GetBitmap() const;

    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
    void Print(std::ostream& os) const override;

  private:
    uint8_t m_dscp;
    uint8_t m_bitmap;
};

} // namespace ns3

#endif // SLICESCOPE_HEADER_H