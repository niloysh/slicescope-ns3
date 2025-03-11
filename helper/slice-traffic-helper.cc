#include "slice-traffic-helper.h"

#include "ns3/double.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SliceTrafficHelper");

SliceTrafficHelper::SliceTrafficHelper(Ipv4Address destIp, uint16_t destPort)
    : m_destIp(destIp),
      m_destPort(destPort),
      m_maxPackets(0), // Default: no limit
      m_numSlices(1),
      m_appsPerSlice(0)
{
    m_randomAppsPerSlice = CreateObject<UniformRandomVariable>();
    m_randomDataRate = CreateObject<UniformRandomVariable>();
    m_randomPacketSize = CreateObject<UniformRandomVariable>();

    m_sliceParams[eMBB] = {};
    m_sliceParams[URLLC] = {};
    m_sliceParams[mMTC] = {};
}

SliceTrafficHelper::~SliceTrafficHelper()
{
}

void
SliceTrafficHelper::SetSliceProbabilities(std::unordered_map<SliceType, double> sliceProbabilities)
{
    m_sliceProbabilities = sliceProbabilities;
}

void
SliceTrafficHelper::SetSources(const NodeContainer& sources)
{
    m_sources = sources;
}

void
SliceTrafficHelper::SetMaxPackets(uint32_t maxPackets)
{
    m_maxPackets = maxPackets;
}

void
SliceTrafficHelper::SetAppsPerSlice(uint32_t appsPerSlice)
{
    m_setAppsPerSlice = true;
    m_appsPerSlice = appsPerSlice;
}

void
SliceTrafficHelper::SetNumSlices(uint32_t numSlices)
{
    m_numSlices = numSlices;
}

void
SliceTrafficHelper::SetSliceParams(SliceType sliceType, const SliceParams& params)
{
    m_sliceParams[sliceType] = params;
}

ApplicationContainer
SliceTrafficHelper::Install()
{
    ApplicationContainer apps;

    if (m_sources.GetN() == 0)
    {
        NS_FATAL_ERROR("No sources specified");
    }

    if (m_sliceProbabilities.size() == 0)
    {
        NS_FATAL_ERROR("No slices specified");
    }

    // Create a random variable to select slice types based on probabilities
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    rand->SetAttribute("Min", DoubleValue(0.0));
    rand->SetAttribute("Max", DoubleValue(1.0));

    NS_LOG_INFO("=== SliceTrafficHelper ===");
    for (uint32_t i = 0; i < m_numSlices; ++i)
    {
        // Determine the source node for this slice
        Ptr<Node> sourceNode = m_sources.Get(i % m_sources.GetN());

        // Determine the slice type based on probabilities
        double randVal = rand->GetValue();
        double cumulativeProb = 0.0;
        SliceType sliceType = eMBB;
        for (const auto& slice : m_sliceProbabilities)
        {
            cumulativeProb += slice.second;
            if (randVal < cumulativeProb)
            {
                sliceType = slice.first;
                break;
            }
        }

        if (!m_setAppsPerSlice)
        {
            m_appsPerSlice = m_randomAppsPerSlice->GetInteger(m_sliceParams[sliceType].minApps,
                                                              m_sliceParams[sliceType].maxApps);
        }

        for (uint32_t j = 0; j < m_appsPerSlice; ++j)
        {
            Ptr<CustomTrafficGenerator> generator = CreateTrafficGenerator(sliceType, m_maxPackets);
            generator->SetSliceType(sliceTypeNames[sliceType]);

            sourceNode->AddApplication(generator);
            apps.Add(generator);
        }

        NS_LOG_INFO("Node: " << sourceNode->GetId() << " Apps: " << m_appsPerSlice
                             << " Slice: " << sliceTypeNames[sliceType]);
    }

    return apps;
}

Ptr<CustomTrafficGenerator>
SliceTrafficHelper::CreateTrafficGenerator(SliceType sliceType, uint32_t maxPackets)
{
    SliceParams params = m_sliceParams[sliceType];
    double selectedRate = m_randomDataRate->GetValue(params.minRateMbps, params.maxRateMbps);
    Ptr<CustomTrafficGenerator> generator = CreateObject<CustomTrafficGenerator>();
    generator->Setup(m_destIp,
                     m_destPort,
                     selectedRate,
                     params.minPacketSize,
                     params.maxPacketSize,
                     maxPackets);

    return generator;
}

} // namespace ns3