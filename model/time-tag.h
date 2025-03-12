#ifndef TIME_TAG_H
#define TIME_TAG_H

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/tag.h"

namespace ns3
{

class TimeTag : public Tag
{
  public:
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::TimeTag").SetParent<Tag>().SetGroupName("Network");
        return tid;
    }

    TypeId GetInstanceTypeId() const override
    {
        return GetTypeId();
    }

    uint32_t GetSerializedSize() const override
    {
        return sizeof(uint64_t); // Store time as an integer (nanoseconds)
    }

    void Serialize(TagBuffer i) const override
    {
        i.WriteU64(m_time.GetNanoSeconds());
    }

    void Deserialize(TagBuffer i) override
    {
        m_time = NanoSeconds(i.ReadU64());
    }

    void Print(std::ostream& os) const override
    {
        os << "Timestamp: " << m_time.GetNanoSeconds() << " ns";
    }

    void SetTime(Time time)
    {
        m_time = time;
    }

    Time GetTime() const
    {
        return m_time;
    }

  private:
    Time m_time;
};

} // namespace ns3

#endif /* TIME_TAG_H */
