#ifndef SLICE_H
#define SLICE_H
#include "ns3/application-container.h"
#include "ns3/node.h"

#include <cstdint>
#include <sys/types.h>
#include <unordered_map>

namespace ns3
{

class Slice : public Object
{
  public:
    enum SliceType
    {
        eMBB,
        URLLC,
        mMTC
    };

    static const std::unordered_map<SliceType, uint8_t> dscpMap;

    Slice();
    ~Slice() override;

    static TypeId GetTypeId();
    void Configure();
    void InstallApps();
    std::vector<ApplicationContainer> GetSourceApps();
    std::vector<ApplicationContainer> GetSinkApps();
    uint32_t GetSliceId() const;
    SliceType GetSliceType() const;

  private:
    static uint32_t _m_sliceId;
    uint32_t m_sliceId;
    SliceType m_sliceType;
    Ptr<Node> m_sourceNode;
    Ptr<Node> m_sinkNode;
    std::vector<ApplicationContainer> m_sourceApps;
    std::vector<ApplicationContainer> m_sinkApps;
    uint8_t m_dscp;
    uint32_t m_numApps;
    uint32_t m_maxPackets;
    Ptr<RandomVariableStream> m_dataRateVar;
    Ptr<RandomVariableStream> m_packetSizeVar;
    double m_startTime;
    double m_stopTime;
};
} // namespace ns3

#endif // SLICE_H