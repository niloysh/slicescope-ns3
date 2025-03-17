#ifndef SLICE_TRAFFIC_HELPER_H
#define SLICE_TRAFFIC_HELPER_H

#include "ns3/application-container.h"
#include "ns3/custom-packet-sink.h"
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

    std::unordered_map<SliceType, std::string> sliceTypeNames = {{eMBB, "eMBB"},
                                                                 {URLLC, "URLLC"},
                                                                 {mMTC, "mMTC"}};

    struct SliceParams
    {
        double minRateMbps = 1.0;
        double maxRateMbps = 5.0;
        uint32_t minPacketSize = 100;
        uint32_t maxPacketSize = 1500;
        uint32_t minApps = 1;
        uint32_t maxApps = 2;
    };

    /**
     * \brief Information about a slice
     * A slice can contain multiple applications.
     * Each slice has a fixed source and sink node. The applications in the slice share the same
     * source and sink, but can have different ports.
     */
    struct SliceInfo
    {
        SliceType sliceType;
        uint32_t numApps;
        uint32_t sourceNodeId;
        uint32_t sinkNodeId;
        std::vector<ApplicationContainer> sourceApps;
        std::vector<ApplicationContainer> sinkApps;
    };

    SliceTrafficHelper();
    ~SliceTrafficHelper();

    void SetSliceProbabilities(std::unordered_map<SliceType, double> sliceProbabilities);
    void SetSliceParams(SliceType sliceType, const SliceParams& params);
    void SetSources(const NodeContainer& sources);
    void SetSinks(const NodeContainer& sinks);
    void SetMaxPackets(uint32_t maxPackets);
    void SetAppsPerSlice(uint32_t appsPerSlice);
    void SetNumSlices(uint32_t numSlices);
    Ptr<CustomTrafficGenerator> CreateTrafficGenerator(SliceType sliceType,
                                                       uint32_t maxPackets,
                                                       Ipv4Address destIp,
                                                       uint16_t destPort);

    std::pair<ApplicationContainer, ApplicationContainer> Install();
    std::map<uint32_t, SliceInfo> GetSliceInfo() const;

  private:
    std::unordered_map<SliceType, SliceParams> m_sliceParams;
    std::unordered_map<SliceType, double> m_sliceProbabilities;
    NodeContainer m_sources;
    NodeContainer m_sinks;
    uint32_t m_maxPackets;
    uint32_t m_numSlices;
    bool m_setAppsPerSlice = false;
    uint32_t m_appsPerSlice;
    std::vector<Ptr<CustomTrafficGenerator>> m_generators;
    std::vector<Ptr<CustomPacketSink>> m_receivers;

    Ptr<UniformRandomVariable> m_randomDataRate;
    Ptr<UniformRandomVariable> m_randomPacketSize;
    Ptr<UniformRandomVariable> m_randomAppsPerSlice;

    std::map<uint32_t, SliceInfo> m_sliceInfo;
};

} // namespace ns3

#endif /* SLICE_TRAFFIC_HELPER_H */