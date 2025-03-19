#include "metadata-tag.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MetadataTag");
NS_OBJECT_ENSURE_REGISTERED(MetadataTag);

TypeId
MetadataTag::GetTypeId()
{
    static TypeId tid = TypeId("MetadataTag")
                            .SetParent<Tag>()
                            .SetGroupName("Network")
                            .AddConstructor<MetadataTag>();
    return tid;
}

MetadataTag::MetadataTag()
    : m_ingressTimestamp(0),
      m_egressTimestamp(0),
      m_inputPort(0),
      m_outputPort(0)
{
}

MetadataTag::~MetadataTag()
{
}

TypeId
MetadataTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MetadataTag::Serialize(TagBuffer i) const
{
    i.WriteU64(m_ingressTimestamp.GetNanoSeconds());
    i.WriteU64(m_egressTimestamp.GetNanoSeconds());
    i.WriteU32(m_inputPort);
    i.WriteU32(m_outputPort);
}

void
MetadataTag::Deserialize(TagBuffer i)
{
    m_ingressTimestamp = NanoSeconds(i.ReadU64());
    m_egressTimestamp = NanoSeconds(i.ReadU64());
    m_inputPort = i.ReadU32();
    m_outputPort = i.ReadU32();
}

uint32_t
MetadataTag::GetSerializedSize() const
{
    return sizeof(uint64_t) * 2 + sizeof(uint32_t) * 2;
}

void
MetadataTag::Print(std::ostream& os) const
{
    NS_LOG_INFO("Ingress timestamp: " << m_ingressTimestamp);
    NS_LOG_INFO("Egress timestamp: " << m_egressTimestamp);
    NS_LOG_INFO("Input port: " << m_inputPort);
    NS_LOG_INFO("Output port: " << m_outputPort);
}

void
MetadataTag::SetIngressTimestamp(Time t)
{
    m_ingressTimestamp = t;
}

void
MetadataTag::SetEgressTimestamp(Time t)
{
    m_egressTimestamp = t;
}

void
MetadataTag::SetInputPort(uint32_t port)
{
    m_inputPort = port;
}

void
MetadataTag::SetOutputPort(uint32_t port)
{
    m_outputPort = port;
}

Time
MetadataTag::GetIngressTimestamp() const
{
    return m_ingressTimestamp;
}

Time
MetadataTag::GetEgressTimestamp() const
{
    return m_egressTimestamp;
}

uint32_t
MetadataTag::GetInputPort() const
{
    return m_inputPort;
}

uint32_t
MetadataTag::GetOutputPort() const
{
    return m_outputPort;
}

} // namespace ns3