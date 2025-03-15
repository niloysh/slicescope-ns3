#include "topology-helper.h"

#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/string.h"

#include <sys/types.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TopologyHelper");

TopologyHelper::TopologyHelper(uint32_t numSwitches, uint32_t numHosts)
    : subnetCounter(1)
{
    NS_LOG_INFO("Creating topology with " << numSwitches << " switches and " << numHosts
                                          << " hosts");
    switches.Create(numSwitches);
    hosts.Create(numHosts);

    // Install internet stack on all nodes
    internet.Install(hosts);
    internet.Install(switches);
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
TopologyHelper::CreateTopology(std::vector<std::pair<int, int>> hostSwitchLinks,
                               std::vector<std::pair<int, int>> interSwitchLinks)
{
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
TopologyHelper::CreateLinks(const std::vector<std::pair<int, int>>& links,
                            NodeContainer& nodeGroupA,
                            NodeContainer& nodeGroupB,
                            std::vector<NodeContainer>& nodePairs,
                            PointToPointHelper& p2p,
                            std::vector<NetDeviceContainer>& devicePairs)
{
    for (const auto& link : links)
    {
        int nodeIdA = link.first;
        int nodeIdB = link.second;

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