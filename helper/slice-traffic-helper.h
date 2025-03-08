#ifndef SLICE_TRAFFIC_HELPER_H
#define SLICE_TRAFFIC_HELPER_H

#include "ns3/custom-traffic-generator.h"
#include "ns3/random-variable-stream.h"

#include <string>

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

    SliceTrafficHelper(Ipv4Address dest, uint16_t port);
    ~SliceTrafficHelper();

    Ptr<CustomTrafficGenerator> CreateTrafficGenerator(SliceType slice, uint32_t maxPackets);
    std::string GetSliceTypeString(SliceType sliceType);

  private:
    Ipv4Address m_dest;
    uint16_t m_port;

    Ptr<UniformRandomVariable> m_randomDataRate;
    Ptr<UniformRandomVariable> m_randomPacketSize;
};

} // namespace ns3

#endif /* SLICE_TRAFFIC_HELPER_H */
