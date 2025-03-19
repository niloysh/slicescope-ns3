#include "linear-topology-helper.h"

#include <ns3/names.h>

#include <unordered_set>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LinearTopologyHelper");

TypeId
LinearTopologyHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LinearTopologyHelper")
            .SetParent<TopologyHelper>()
            .SetGroupName("Helper")
            .AddConstructor<LinearTopologyHelper>()
            .AddAttribute("SubnetCounter",
                          "Counter for subnet addresses",
                          IntegerValue(1),
                          MakeIntegerAccessor(&LinearTopologyHelper::m_subnetCounter),
                          MakeIntegerChecker<int>())
            .AddAttribute("CustomQueueDiscs",
                          "Enable custom queue discs",
                          BooleanValue(false),
                          MakeBooleanAccessor(&LinearTopologyHelper::m_customQueueDiscs),
                          MakeBooleanChecker());
    return tid;
}

LinearTopologyHelper::LinearTopologyHelper()
{
}

LinearTopologyHelper::~LinearTopologyHelper()
{
}

void
LinearTopologyHelper::CreateTopology(const uint32_t numNodes)
{
    NS_LOG_INFO("[LinearTopologyHelper] Creating linear topology with " << numNodes << " nodes...");

    // Prepare host-to-switch and switch-to-switch links
    std::vector<std::pair<uint32_t, uint32_t>> hostToSwitchLinks;
    std::vector<std::pair<uint32_t, uint32_t>> switchToSwitchLinks;

    hostToSwitchLinks.reserve(numNodes);
    switches.Create(numNodes);
    hosts.Create(numNodes);

    InternetStackHelper internet;
    internet.Install(hosts);
    internet.Install(switches);

    for (uint32_t i = 0; i < numNodes; ++i)
    {
        uint32_t switchIndex = i;
        uint32_t hostIndex = i;
        Ptr<Node> host = hosts.Get(hostIndex);
        Ptr<Node> switchNode = switches.Get(switchIndex);
        Names::Add("h" + std::to_string(i), hosts.Get(i));
        Names::Add("s" + std::to_string(i), switches.Get(i));
        NetDeviceContainer devicePair = CreateLink(host, switchNode, p2pHosts);
        devicePairs.push_back(devicePair);
    }

    // Create inter-switch links
    switchToSwitchLinks.reserve(numNodes - 1);
    for (uint32_t i = 0; i < numNodes - 1; ++i)
    {
        uint32_t switchIndexA = i;
        uint32_t switchIndexB = i + 1;
        Ptr<Node> switchNodeA = switches.Get(switchIndexA);
        Ptr<Node> switchNodeB = switches.Get(switchIndexB);
        NetDeviceContainer devicePair = CreateLink(switchNodeA, switchNodeB, p2pSwitches);
        devicePairs.push_back(devicePair);
    }

    AssignIPAddresses(devicePairs);

    // Enable global routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    MapSwitchesToNetDevices();
    if (m_customQueueDiscs)
    {
        NS_LOG_INFO("[LinearToplogyHelper] Setting custom queue discs");
        SetQueueDiscs(switchNetDevices);
    }
}

void
LinearTopologyHelper::SetHostChannelHelper(PointToPointHelper p2pHosts)
{
    this->p2pHosts = p2pHosts;
}

void
LinearTopologyHelper::SetSwitchChannelHelper(PointToPointHelper p2pSwitches)
{
    this->p2pSwitches = p2pSwitches;
}

} // namespace ns3