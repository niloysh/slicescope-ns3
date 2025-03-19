#ifndef FAT_TREE_TOPOLOGY_HELPER_H
#define FAT_TREE_TOPOLOGY_HELPER_H

#include "topology-helper.h"

namespace ns3
{

class FatTreeTopologyHelper : public TopologyHelper
{
  public:
    static TypeId GetTypeId();
    FatTreeTopologyHelper();
    ~FatTreeTopologyHelper() override;

    /**
     * Create a fat-tree topology with k pods.
     * A pod contains edge switches and aggregation switches.
     * Each pod contains k/2 edge switches and k/2 aggregation switches.
     * Each edge switch is connected to k/2 hosts. Therefore, the number of hosts is (k * k) / 4 per
     * pod. Each aggregation switch is connected to all k/2 edge switches in the pod.
     * Each core switch is connected to all k/2 aggregation switches in each pod.
     */
    void CreateTopology(const uint32_t numPods);
    void SetCoreToAggChannelHelper(PointToPointHelper p2pCoreToAgg);
    void SetAggToEdgeChannelHelper(PointToPointHelper p2pAggToEdge);
    void SetEdgeToHostChannelHelper(PointToPointHelper p2pEdgetoHost);

  private:
    uint32_t m_numPods;
    uint32_t m_numHosts;
    uint32_t m_numEdgeSwitches;
    uint32_t m_numAggSwitches;
    uint32_t m_numCoreSwitches;

    NodeContainer edgeSwitches;
    NodeContainer aggSwitches;
    NodeContainer coreSwitches;

    PointToPointHelper p2pEdgeToHost;
    PointToPointHelper p2pAggToEdge;
    PointToPointHelper p2pCoreToAgg;
};

} // namespace ns3

#endif // FAT_TREE_TOPOLOGY_HELPER_H