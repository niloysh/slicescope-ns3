
#include "slice-traffic-helper.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SliceTrafficHelper");

SliceTrafficHelper::SliceTrafficHelper(Ipv4Address dest, uint16_t port)
    : m_dest(dest),
      m_port(port)
{
    // Random number generators for selecting slice-specific parameters
    m_randomDataRate = CreateObject<UniformRandomVariable>();
    m_randomPacketSize = CreateObject<UniformRandomVariable>();
}

SliceTrafficHelper::~SliceTrafficHelper()
{
}

Ptr<CustomTrafficGenerator>
SliceTrafficHelper::CreateTrafficGenerator(SliceType slice, uint32_t maxPackets)
{
    double minRate;
    double maxRate;
    uint32_t minPacketSize;
    uint32_t maxPacketSize;

    switch (slice)
    {
    case eMBB:
        minRate = 100.0;
        maxRate = 1000.0;
        minPacketSize = 1000;
        maxPacketSize = 1500;
        break;

    case URLLC:
        minRate = 0.1;
        maxRate = 10.0;
        minPacketSize = 20;
        maxPacketSize = 250;
        break;

    case mMTC:
        minRate = 0.001;
        maxRate = 0.1;
        minPacketSize = 20;
        maxPacketSize = 125;
        break;

    default:
        NS_FATAL_ERROR("Invalid slice type");
    }

    double selectedRate = m_randomDataRate->GetValue(minRate, maxRate);
    Ptr<CustomTrafficGenerator> generator = CreateObject<CustomTrafficGenerator>();
    generator->Setup(m_dest, m_port, selectedRate, minPacketSize, maxPacketSize, maxPackets);

    return generator;
}

std::string
SliceTrafficHelper::GetSliceTypeString(SliceType sliceType)
{
    switch (sliceType)
    {
    case eMBB:
        return "eMBB";
    case URLLC:
        return "URLLC";
    case mMTC:
        return "mMTC";
    default:
        return "Unknown";
    }
}

} // namespace ns3