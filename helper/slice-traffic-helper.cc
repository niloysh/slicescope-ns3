#include "slice-traffic-helper.h"

#include "ns3/custom-packet-sink.h"
#include "ns3/double.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/random-variable-stream.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SliceTrafficHelper");

SliceTrafficHelper::SliceTrafficHelper()
    : m_maxPackets(0), // Unlimited packets
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
SliceTrafficHelper::SetSinks(const NodeContainer& sinks)
{
    m_sinks = sinks;
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

std::pair<ApplicationContainer, ApplicationContainer>
SliceTrafficHelper::Install()
{
    ApplicationContainer apps;
    ApplicationContainer receivers;

    if (m_sources.GetN() == 0)
    {
        NS_FATAL_ERROR("No sources specified");
    }

    if (m_sinks.GetN() == 0)
    {
        NS_FATAL_ERROR("No sinks specified");
    }

    if (m_sliceProbabilities.size() == 0)
    {
        NS_FATAL_ERROR("No slices specified");
    }

    // Create a random variable to select slice types based on probabilities
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    rand->SetAttribute("Min", DoubleValue(0.0));
    rand->SetAttribute("Max", DoubleValue(1.0));

    for (uint32_t i = 0; i < m_numSlices; ++i)
    {
        // Determine the source and sink node for this slice
        // Source and sink should be different

        Ptr<Node> sourceNode;
        Ptr<Node> sinkNode;
        do
        {
            sourceNode = m_sources.Get(rand->GetInteger(0, m_sources.GetN() - 1));
            sinkNode = m_sinks.Get(rand->GetInteger(0, m_sinks.GetN() - 1));
        } while (sourceNode == sinkNode);

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

        // NS_LOG_INFO("Node: " << sourceNode->GetId() << " Apps: " << m_appsPerSlice
        //                      << " Slice: " << sliceTypeNames[sliceType]);

        SliceInfo sliceInfo;
        sliceInfo.sliceType = sliceType;
        sliceInfo.numApps = m_appsPerSlice;
        sliceInfo.sourceNodeId = sourceNode->GetId();
        sliceInfo.sinkNodeId = sinkNode->GetId();

        m_sliceInfo[sourceNode->GetId()] = sliceInfo;

        for (uint32_t j = 0; j < m_appsPerSlice; ++j)
        {
            // configure the receiver
            uint16_t destPort = m_randomDataRate->GetInteger(1024, 65535);
            Ptr<CustomPacketSink> sinkApp = CreateObject<CustomPacketSink>();
            sinkApp->SetAttribute("Port", UintegerValue(destPort));
            sinkNode->AddApplication(sinkApp);
            receivers.Add(sinkApp);

            // configure the generator
            Ptr<Ipv4> ipv4_sink = sinkNode->GetObject<Ipv4>();
            Ipv4Address ip_sink = ipv4_sink->GetAddress(1, 0).GetLocal();
            Ptr<CustomTrafficGenerator> sourceApp =
                CreateTrafficGenerator(sliceType, m_maxPackets, ip_sink, destPort);
            // sourceApp->SetSliceType(sliceTypeNames[sliceType]);
            sourceNode->AddApplication(sourceApp);
            apps.Add(sourceApp);

            m_generators.push_back(sourceApp);
            m_receivers.push_back(sinkApp);
        }
    }

    return std::make_pair(apps, receivers);
}

Ptr<CustomTrafficGenerator>
SliceTrafficHelper::CreateTrafficGenerator(SliceType sliceType,
                                           uint32_t maxPackets,
                                           Ipv4Address destIp,
                                           uint16_t destPort)
{
    SliceParams params = m_sliceParams[sliceType];
    double selectedRate = m_randomDataRate->GetValue(params.minRateMbps, params.maxRateMbps);
    Ptr<CustomTrafficGenerator> generator = CreateObject<CustomTrafficGenerator>();
    generator->SetAttribute("DestIp", Ipv4AddressValue(destIp));
    generator->SetAttribute("DestPort", UintegerValue(destPort));
    generator->SetAttribute("MaxPackets", UintegerValue(maxPackets));
    generator->SetAttribute("DataRate", DoubleValue(selectedRate));
    return generator;
}

std::map<uint32_t, SliceTrafficHelper::SliceInfo>
SliceTrafficHelper::GetSliceInfo() const
{
    return m_sliceInfo;
}

} // namespace ns3