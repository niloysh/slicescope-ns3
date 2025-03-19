#ifndef METADATA_TAG_H
#define METADATA_TAG_H

#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3
{

class MetadataTag : public Tag
{
  public:
    static TypeId GetTypeId();
    MetadataTag();
    ~MetadataTag() override;

    TypeId GetInstanceTypeId() const override;
    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    uint32_t GetSerializedSize() const override;
    void Print(std::ostream& os) const override;

    void SetIngressTimestamp(Time t);
    void SetEgressTimestamp(Time t);
    void SetInputPort(uint32_t port);
    void SetOutputPort(uint32_t port);
    Time GetIngressTimestamp() const;
    Time GetEgressTimestamp() const;
    uint32_t GetInputPort() const;
    uint32_t GetOutputPort() const;

  private:
    Time m_ingressTimestamp;
    Time m_egressTimestamp;
    uint32_t m_inputPort;
    uint32_t m_outputPort;
};

} // namespace ns3
#endif /* METADATA_TAG_H */
