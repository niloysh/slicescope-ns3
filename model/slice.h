#ifndef SLICE_H
#define SLICE_H
#include "ns3/application-container.h"
#include "ns3/node.h"

#include <cstdint>
#include <sys/types.h>
#include <unordered_map>

namespace ns3
{

class Slice
{
    enum SliceType
    {
        eMBB,
        URLLC,
        mMTC
    };

    struct SliceParams
    {
        double minRateMbps = 1.0;
        double maxRateMbps = 5.0;
        uint32_t minPacketSize = 100;
        uint32_t maxPacketSize = 1500;
        uint32_t minApps = 1;
        uint32_t maxApps = 2;
    };

    static const std::unordered_map<SliceType, uint8_t> dscpMap;

    Slice(SliceType sliceType,
          uint32_t sliceId,
          Ptr<Node> sourceNode,
          Ptr<Node> sinkNode,
          const SliceParams& params);

    ~Slice();

    void InstallApps();
    std::vector<ApplicationContainer> GetSourceApps();
    std::vector<ApplicationContainer> GetSinkApps();

  private:
    uint32_t m_sliceId;
    SliceType m_sliceType;
    Ptr<Node> m_sourceNode;
    Ptr<Node> m_sinkNode;
    SliceParams m_params;
    std::vector<ApplicationContainer> m_sourceApps;
    std::vector<ApplicationContainer> m_sinkApps;
    uint8_t m_dscp;
};
} // namespace ns3

#endif // SLICE_H