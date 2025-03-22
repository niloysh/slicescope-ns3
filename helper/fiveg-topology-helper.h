#ifndef FIVEG_TOPOLOGY_HELPER_H
#define FIVEG_TOPOLOGY_HELPER_H

#include "topology-helper.h"

namespace ns3
{

class FiveGTopologyHelper : public TopologyHelper
{
  public:
    static TypeId GetTypeId();
    FiveGTopologyHelper();
    ~FiveGTopologyHelper() override;

    void CreateTopology();
    NodeContainer m_gnbNodes;
    NodeContainer m_accessNodes;
    NodeContainer m_preAggNodes;
    NodeContainer m_aggNodes;
    NodeContainer m_coreNodes;
    NodeContainer m_upfNodes;

    PointToPointHelper p2pGnbToAccess;
    PointToPointHelper p2pPreAggToAgg;
    PointToPointHelper p2pAggRing;
    PointToPointHelper p2pAggToCore;
};

} // namespace ns3

#endif // FIVEG_TOPOLOGY_HELPER_H