#include "advanced-topology-helper.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AdvancedTopologyHelper");

AdvancedTopologyHelper::AdvancedTopologyHelper()
{
}

AdvancedTopologyHelper::~AdvancedTopologyHelper()
{
}

void
AdvancedTopologyHelper::SetHostChannelHelper(PointToPointHelper p2pHosts)
{
    topoHelper.SetHostChannelHelper(p2pHosts);
}

void
AdvancedTopologyHelper::SetSwitchChannelHelper(PointToPointHelper p2pSwitches)
{
    topoHelper.SetSwitchChannelHelper(p2pSwitches);
}

void
AdvancedTopologyHelper::CreateLinearTopology(uint32_t numNodes)
{
    std::vector<std::pair<uint32_t, uint32_t>> hostSwitchLinks;
    std::vector<std::pair<uint32_t, uint32_t>> interSwitchLinks;

    hostSwitchLinks.reserve(numNodes);
    for (uint32_t i = 0; i < numNodes; ++i)
    {
        hostSwitchLinks.emplace_back(i, i); // Host i connected to Switch i
    }
    interSwitchLinks.reserve(numNodes - 1);
    for (uint32_t i = 0; i < numNodes - 1; ++i)
    {
        interSwitchLinks.emplace_back(i, i + 1); // Switch i connected to Switch i+1
    }

    topoHelper.CreateTopology(hostSwitchLinks, interSwitchLinks);
}

void
AdvancedTopologyHelper::CreateTreeTopology(uint32_t depth, uint32_t fanout)
{
    std::vector<std::pair<uint32_t, uint32_t>> hostSwitchLinks;
    std::vector<std::pair<uint32_t, uint32_t>> interSwitchLinks;

    uint32_t numSwitches = 0;
    uint32_t numHosts = 0;

    std::vector<uint32_t> prevLayer; // Stores switch IDs of the previous layer
    std::vector<uint32_t> currLayer; // Stores switch IDs of the current layer

    // Root switch (depth = 0)
    prevLayer.push_back(numSwitches++);

    for (uint32_t d = 1; d < depth; ++d)
    {
        currLayer.clear();

        for (uint32_t parentSwitch : prevLayer)
        {
            for (uint32_t f = 0; f < fanout; ++f)
            {
                currLayer.push_back(numSwitches);
                interSwitchLinks.emplace_back(parentSwitch, numSwitches);
                ++numSwitches;
            }
        }

        prevLayer = currLayer;
    }

    // Attach hosts to leaf switches (last layer)
    for (uint32_t leafSwitch : prevLayer)
    {
        for (uint32_t f = 0; f < fanout; ++f)
        {
            hostSwitchLinks.emplace_back(numHosts, leafSwitch);
            ++numHosts;
        }
    }

    topoHelper.CreateTopology(hostSwitchLinks, interSwitchLinks);
}

NodeContainer
AdvancedTopologyHelper::GetHosts()
{
    return topoHelper.GetHosts();
}

NodeContainer
AdvancedTopologyHelper::GetSwitches()
{
    return topoHelper.GetSwitches();
}

} // namespace ns3