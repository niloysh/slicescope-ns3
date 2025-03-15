#include "topology-helper.h"

#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/string.h"

#include <cstdint>
#include <sys/types.h>
#include <unordered_set>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TopologyHelper");

TopologyHelper::TopologyHelper()
    : subnetCounter(1)
{
}

TopologyHelper::~TopologyHelper()
{
}

void
TopologyHelper::SetHostChannelHelper(PointToPointHelper p2pHosts)
{
    this->p2pHosts = p2pHosts;
}

void
TopologyHelper::SetSwitchChannelHelper(PointToPointHelper p2pSwitches)
{
    this->p2pSwitches = p2pSwitches;
}

void
TopologyHelper::CreateTopology(std::vector<std::pair<uint32_t, uint32_t>> hostSwitchLinks,
                               std::vector<std::pair<uint32_t, uint32_t>> interSwitchLinks)
{
    std::unordered_set<uint32_t> hostIndices;
    std::unordered_set<uint32_t> switchIndices;

    for (const auto& link : hostSwitchLinks)
    {
        hostIndices.insert(link.first);
        switchIndices.insert(link.second);
    }

    for (const auto& link : interSwitchLinks)
    {
        switchIndices.insert(link.first);
        switchIndices.insert(link.second);
    }

    hosts.Create(hostIndices.size());
    switches.Create(switchIndices.size());

    // Install internet stack on all nodes
    internet.Install(hosts);
    internet.Install(switches);

    CreateLinks(hostSwitchLinks,
                hosts,
                switches,
                nodePairsHostSwitch,
                p2pHosts,
                devicePairsHostSwitch);
    CreateLinks(interSwitchLinks,
                switches,
                switches,
                nodePairsInterSwitch,
                p2pSwitches,
                devicePairsInterSwitch);

    AssignIPAddresses(devicePairsHostSwitch);
    AssignIPAddresses(devicePairsInterSwitch);

    // Enable static global routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
}

void
TopologyHelper::CreateLinks(const std::vector<std::pair<uint32_t, uint32_t>>& links,
                            NodeContainer& nodeGroupA,
                            NodeContainer& nodeGroupB,
                            std::vector<NodeContainer>& nodePairs,
                            PointToPointHelper& p2p,
                            std::vector<NetDeviceContainer>& devicePairs)
{
    for (const auto& link : links)
    {
        uint32_t nodeIdA = link.first;
        uint32_t nodeIdB = link.second;

        if (nodeIdA >= nodeGroupA.GetN() || nodeIdB >= nodeGroupB.GetN())
        {
            NS_FATAL_ERROR("Node ID out of range");
            return;
        }

        NodeContainer nodePair = NodeContainer(nodeGroupA.Get(nodeIdA), nodeGroupB.Get(nodeIdB));
        nodePairs.push_back(nodePair);

        // Install point-to-point devices
        NetDeviceContainer devicePair = p2p.Install(nodePair);
        devicePairs.push_back(devicePair);
    }
}

void
TopologyHelper::AssignIPAddresses(std::vector<NetDeviceContainer>& devicePairs)
{
    for (const auto& devicePair : devicePairs)
    {
        std::ostringstream subnet;
        subnet << "10.1." << subnetCounter << ".0";
        NS_LOG_DEBUG("Assigning IP addresses for subnet " << subnet.str());

        ipv4.SetBase(subnet.str().c_str(), "255.255.255.0");
        Ipv4InterfaceContainer interfacePair = ipv4.Assign(devicePair);
        interfacePairs.push_back(interfacePair);

        subnetCounter++;
    }
}

NodeContainer
TopologyHelper::GetSwitches()
{
    return switches;
}

NodeContainer
TopologyHelper::GetHosts()
{
    return hosts;
}

} // namespace ns3