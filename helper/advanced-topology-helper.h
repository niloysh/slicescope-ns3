#ifndef ADVANCED_TOPOLOGY_HELPER_H
#define ADVANCED_TOPOLOGY_HELPER_H

#include "topology-helper.h"

#include "ns3/point-to-point-helper.h"

#include <cstdint>

namespace ns3
{

class AdvancedTopologyHelper
{
  public:
    AdvancedTopologyHelper();
    ~AdvancedTopologyHelper();

    void SetHostChannelHelper(PointToPointHelper p2pHosts);
    void SetSwitchChannelHelper(PointToPointHelper p2pSwitches);

    /**
     * Create a linear topology. Each node has one host connected to it.
     * @param numNodes The number of nodes in the topology
     */
    void CreateLinearTopology(uint32_t numNodes);

    /**
     * Create a tree topology. The root switch has fanout number of children switches, each of which
     * has fanout number of hosts connected to it.
     * @param depth The depth of the tree
     * @param fanout The fanout of the tree
     */
    void CreateTreeTopology(uint32_t depth, uint32_t fanout);

    NodeContainer GetHosts();
    NodeContainer GetSwitches();

  private:
    TopologyHelper topoHelper;
};
} // namespace ns3

#endif // ADVANCED_TOPOLOGY_HELPER_H