#ifndef LINEAR_TOPOLOGY_HELPER_H
#define LINEAR_TOPOLOGY_HELPER_H

#include "topology-helper.h"

namespace ns3
{

class LinearTopologyHelper : public TopologyHelper
{
  public:
    static TypeId GetTypeId();
    LinearTopologyHelper();
    ~LinearTopologyHelper() override;

    void CreateTopology(const uint32_t numNodes);
    void SetHostChannelHelper(PointToPointHelper p2pHosts);
    void SetSwitchChannelHelper(PointToPointHelper p2pSwitches);
};

} // namespace ns3

#endif // LINEAR_TOPOLOGY_HELPER_H