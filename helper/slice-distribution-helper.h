#ifndef SLICE_DISTRIBUTION_HELPER_H
#define SLICE_DISTRIBUTION_HELPER_H

#include "ns3/application-container.h"
#include "ns3/custom-traffic-generator.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/random-variable-stream.h"
#include "ns3/slice-traffic-helper.h"

#include <sys/types.h>

namespace ns3
{

class SliceDistributionHelper
{
  public:
    SliceDistributionHelper(Ipv4Address destIp, uint16_t destPort, uint32_t numSlices);

    void SetSlices(const std::vector<std::pair<SliceTrafficHelper::SliceType, double>>& slices);
    void SetSources(const NodeContainer& sources);
    void SetMaxPackets(uint32_t maxPackets);

    ApplicationContainer Install();

  private:
    Ipv4Address m_destIp;
    uint16_t m_destPort;
    std::vector<std::pair<SliceTrafficHelper::SliceType, double>> m_slices;
    NodeContainer m_sources;
    double m_dataRateMbps;
    uint32_t m_maxPackets;
    uint32_t m_numSlices;
};

} // namespace ns3

#endif