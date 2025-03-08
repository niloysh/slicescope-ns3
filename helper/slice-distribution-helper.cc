#include "slice-distribution-helper.h"

#include "slice-traffic-helper.h"

#include "ns3/double.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SliceDistributionHelper");

SliceDistributionHelper::SliceDistributionHelper(Ipv4Address destIp,
                                                 uint16_t destPort,
                                                 uint32_t numSlices)
    : m_destIp(destIp),
      m_destPort(destPort),
      m_maxPackets(0),
      m_numSlices(numSlices) // Default: no limit
{
}

void
SliceDistributionHelper::SetSlices(
    const std::vector<std::pair<SliceTrafficHelper::SliceType, double>>& slices)
{
    m_slices = slices;
}

void
SliceDistributionHelper::SetSources(const NodeContainer& sources)
{
    m_sources = sources;
}

void
SliceDistributionHelper::SetMaxPackets(uint32_t maxPackets)
{
    m_maxPackets = maxPackets;
}

ApplicationContainer
SliceDistributionHelper::Install()
{
    ApplicationContainer apps;

    if (m_sources.GetN() == 0)
    {
        NS_FATAL_ERROR("No sources specified");
    }

    if (m_slices.size() == 0)
    {
        NS_FATAL_ERROR("No slices specified");
    }

    // Create a random variable to select slice types based on probabilities
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    rand->SetAttribute("Min", DoubleValue(0.0));
    rand->SetAttribute("Max", DoubleValue(1.0));

    for (uint32_t i = 0; i < m_numSlices; ++i)
    {
        // Determine the source node for this slice
        Ptr<Node> sourceNode = m_sources.Get(i % m_sources.GetN());

        // Determine the slice type based on probabilities
        double randVal = rand->GetValue();
        double cumulativeProb = 0.0;
        SliceTrafficHelper::SliceType sliceType = SliceTrafficHelper::eMBB; // Default
        for (const auto& slice : m_slices)
        {
            cumulativeProb += slice.second;
            if (randVal <= cumulativeProb)
            {
                sliceType = slice.first;
                break;
            }
        }

        SliceTrafficHelper helper(m_destIp, m_destPort);
        Ptr<CustomTrafficGenerator> generator =
            helper.CreateTrafficGenerator(sliceType, m_maxPackets);

        sourceNode->AddApplication(generator);
        apps.Add(generator);

        NS_LOG_INFO("Node " << sourceNode->GetId() << " is sending "
                            << helper.GetSliceTypeString(sliceType));
    }

    return apps;
}

} // namespace ns3