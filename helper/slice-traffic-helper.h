#ifndef SLICE_TRAFFIC_HELPER_H
#define SLICE_TRAFFIC_HELPER_H

#include "ns3/application-container.h"
#include "ns3/custom-traffic-generator.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/random-variable-stream.h"

#include <cstdint>
#include <sys/types.h>
#include <unordered_map>

namespace ns3
{

class SliceTrafficHelper

{
  public:
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

    SliceTrafficHelper(Ipv4Address destIp, uint16_t destPort);
    ~SliceTrafficHelper();

    void SetSliceProbabilities(std::unordered_map<SliceType, double> sliceProbabilities);
    void SetSliceParams(SliceType sliceType, const SliceParams& params);
    void SetSources(const NodeContainer& sources);
    void SetMaxPackets(uint32_t maxPackets);
    void SetAppsPerSlice(uint32_t appsPerSlice);
    void SetNumSlices(uint32_t numSlices);

    ApplicationContainer Install();

  private:
    Ipv4Address m_destIp;
    uint16_t m_destPort;
    std::unordered_map<SliceType, SliceParams> m_sliceParams;
    std::unordered_map<SliceType, double> m_sliceProbabilities;
    NodeContainer m_sources;
    double m_dataRateMbps;
    uint32_t m_maxPackets;
    uint32_t m_numSlices;
    bool m_setAppsPerSlice = false;
    uint32_t m_appsPerSlice;
    std::vector<Ptr<CustomTrafficGenerator>> m_generators;

    Ptr<UniformRandomVariable> m_randomDataRate;
    Ptr<UniformRandomVariable> m_randomPacketSize;
    Ptr<UniformRandomVariable> m_randomAppsPerSlice;

    Ptr<CustomTrafficGenerator> CreateTrafficGenerator(SliceType sliceType, uint32_t maxPackets);
};

} // namespace ns3

#endif