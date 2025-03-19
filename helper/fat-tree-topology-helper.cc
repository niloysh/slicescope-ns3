#include "fat-tree-topology-helper.h"

#include <ns3/names.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FatTreeTopologyHelper");

TypeId
FatTreeTopologyHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::FatTreeTopologyHelper")
            .SetParent<TopologyHelper>()
            .SetGroupName("Helper")
            .AddConstructor<FatTreeTopologyHelper>()
            .AddAttribute("SubnetCounter",
                          "Counter for subnet addresses",
                          IntegerValue(1),
                          MakeIntegerAccessor(&FatTreeTopologyHelper::m_subnetCounter),
                          MakeIntegerChecker<int>())
            .AddAttribute("CustomQueueDiscs",
                          "Enable custom queue discs",
                          BooleanValue(false),
                          MakeBooleanAccessor(&FatTreeTopologyHelper::m_customQueueDiscs),
                          MakeBooleanChecker());
    return tid;
}

FatTreeTopologyHelper::FatTreeTopologyHelper()
{
}

FatTreeTopologyHelper::~FatTreeTopologyHelper()
{
}

void
FatTreeTopologyHelper::CreateTopology(const uint32_t k)
{
    if (k % 2 != 0)
    {
        NS_FATAL_ERROR("[FatTreeTopologyHelper] k must be an even number");
        return;
    }
    NS_LOG_INFO("[FatTreeTopologyHelper] Creating fat-tree topology with k=" << k << "...");
    m_numPods = k;
    m_numHosts = (k * k * k) / 4;
    m_numEdgeSwitches = (k * k) / 2;
    m_numAggSwitches = (k * k) / 2;
    m_numCoreSwitches = (k / 2) * (k / 2);

    hosts.Create(m_numHosts);
    edgeSwitches.Create(m_numEdgeSwitches);
    aggSwitches.Create(m_numAggSwitches);
    coreSwitches.Create(m_numCoreSwitches);

    // Assign names to nodes
    for (uint32_t i = 0; i < m_numHosts; ++i)
    {
        Names::Add("h" + std::to_string(i), hosts.Get(i));
    }

    for (uint32_t i = 0; i < m_numEdgeSwitches; ++i)
    {
        Names::Add("e" + std::to_string(i), edgeSwitches.Get(i));
        switches.Add(edgeSwitches.Get(i));
    }

    for (uint32_t i = 0; i < m_numAggSwitches; ++i)
    {
        Names::Add("a" + std::to_string(i), aggSwitches.Get(i));
        switches.Add(aggSwitches.Get(i));
    }

    for (uint32_t i = 0; i < m_numCoreSwitches; ++i)
    {
        Names::Add("c" + std::to_string(i), coreSwitches.Get(i));
        switches.Add(coreSwitches.Get(i));
    }

    NS_LOG_INFO("[FatTreeTopologyHelper] Hosts: " << m_numHosts
                                                  << " | Edge switches: " << m_numEdgeSwitches
                                                  << " | Aggregation switches: " << m_numAggSwitches
                                                  << " | Core switches: " << m_numCoreSwitches);

    InternetStackHelper internet;
    internet.Install(hosts);
    internet.Install(edgeSwitches);
    internet.Install(aggSwitches);
    internet.Install(coreSwitches);

    std::vector<std::pair<uint32_t, uint32_t>> coreToAggLinks;
    std::vector<std::pair<uint32_t, uint32_t>> aggToEdgeLinks;
    std::vector<std::pair<uint32_t, uint32_t>> edgeToHostLinks;

    // host to edge links
    for (uint32_t pod = 0; pod < m_numPods; ++pod)
    {
        for (uint32_t i = 0; i < k / 2; ++i) // Each edge switch connects to k/2 hosts
        {
            uint32_t edgeIndex = pod * (k / 2) + i;
            for (uint32_t j = 0; j < k / 2; ++j)
            {
                uint32_t hostIndex = edgeIndex * (k / 2) + j;
                Ptr<Node> host = hosts.Get(hostIndex);
                Ptr<Node> edgeSwitch = edgeSwitches.Get(edgeIndex);
                NetDeviceContainer devicePair = CreateLink(host, edgeSwitch, p2pEdgeToHost);
                devicePairs.push_back(devicePair);
            }
        }
    }

    // edge to aggregation links
    for (uint32_t pod = 0; pod < m_numPods; ++pod)
    {
        for (uint32_t i = 0; i < k / 2; ++i)
        {
            uint32_t edgeIndex = pod * (k / 2) + i;
            for (uint32_t j = 0; j < k / 2; ++j)
            {
                uint32_t aggIndex = pod * (k / 2) + j;
                Ptr<Node> edgeSwitch = edgeSwitches.Get(edgeIndex);
                Ptr<Node> aggSwitch = aggSwitches.Get(aggIndex);
                NetDeviceContainer devicePair = CreateLink(edgeSwitch, aggSwitch, p2pAggToEdge);
                devicePairs.push_back(devicePair);
            }
        }
    }

    // aggregation to core links
    for (uint32_t pod = 0; pod < m_numPods; ++pod) // Each pod has k/2 agg switches
    {
        for (uint32_t i = 0; i < k / 2; ++i) // Each agg switch
        {
            uint32_t aggIndex = pod * (k / 2) + i;

            for (uint32_t j = 0; j < k / 2; ++j) // Connect to k/2 core switches
            {
                uint32_t coreIndex = i * (k / 2) + j; // core index calculation
                Ptr<Node> aggSwitch = aggSwitches.Get(aggIndex);
                Ptr<Node> coreSwitch = coreSwitches.Get(coreIndex);
                NetDeviceContainer devicePair = CreateLink(aggSwitch, coreSwitch, p2pCoreToAgg);
                devicePairs.push_back(devicePair);
            }
        }
    }

    AssignIPAddresses(devicePairs);

    // Enable global routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    MapSwitchesToNetDevices();
    if (m_customQueueDiscs)
    {
        NS_LOG_INFO("[FatTreeTopologyHelper] Setting custom queue discs");
        SetQueueDiscs(switchNetDevices);
    }
}

void
FatTreeTopologyHelper::SetCoreToAggChannelHelper(PointToPointHelper p2pCoreToAgg)
{
    this->p2pCoreToAgg = p2pCoreToAgg;
}

void
FatTreeTopologyHelper::SetAggToEdgeChannelHelper(PointToPointHelper p2pAggToEdge)
{
    this->p2pAggToEdge = p2pAggToEdge;
}

void
FatTreeTopologyHelper::SetEdgeToHostChannelHelper(PointToPointHelper p2pEdgeToHost)
{
    this->p2pEdgeToHost = p2pEdgeToHost;
}

} // namespace ns3