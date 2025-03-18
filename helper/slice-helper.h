#ifndef SLICE_HELPER_H
#define SLICE_HELPER_H

#include "ns3/node-container.h"
#include "ns3/object.h"
#include "ns3/slice.h"

namespace ns3
{

class SliceHelper : public Object
{
  public:
    static TypeId GetTypeId();

    SliceHelper();

    // Use SetAttribute instead of separate setter methods
    void SetAttribute(std::string name, const AttributeValue& value);

    std::vector<Ptr<Slice>> CreateSlices(NodeContainer sources,
                                         NodeContainer sinks,
                                         uint32_t numSlices);

    std::vector<Ptr<Slice>> GetSlices() const;
    void ReportSliceStats();

  private:
    uint32_t m_numSlices;
    double m_simulationDuration;
    uint32_t m_maxPackets;
    uint32_t m_numApps;
    std::vector<Ptr<Slice>> m_slices;
};

} // namespace ns3

#endif // SLICE_HELPER_H
