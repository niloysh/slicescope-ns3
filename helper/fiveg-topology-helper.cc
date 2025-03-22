#include "fiveg-topology-helper.h"

#include "ns3/names.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FiveGTopologyHelper");

TypeId
FiveGTopologyHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::FiveGTopologyHelper")
            .SetParent<TopologyHelper>()
            .SetGroupName("Helper")
            .AddConstructor<FiveGTopologyHelper>()
            .AddAttribute("SubnetCounter",
                          "Counter for subnet addresses",
                          IntegerValue(1),
                          MakeIntegerAccessor(&FiveGTopologyHelper::m_subnetCounter),
                          MakeIntegerChecker<int>())
            .AddAttribute("CustomQueueDiscs",
                          "Enable custom queue discs",
                          BooleanValue(false),
                          MakeBooleanAccessor(&FiveGTopologyHelper::m_customQueueDiscs),
                          MakeBooleanChecker());
    return tid;
}

FiveGTopologyHelper::FiveGTopologyHelper()
{
    m_gnbNodes.Create(3);
    m_accessNodes.Create(3);
    m_preAggNodes.Create(2);
    m_aggNodes.Create(4);
    m_coreNodes.Create(3);
    m_upfNodes.Create(2); // 1 at core and 1 at edge

    hosts.Add(m_gnbNodes);
    hosts.Add(m_upfNodes);

    switches.Add(m_accessNodes);
    switches.Add(m_preAggNodes);
    switches.Add(m_aggNodes);
    switches.Add(m_coreNodes);

    for (uint32_t i = 0; i < m_gnbNodes.GetN(); ++i)
    {
        Names::Add("gNB" + std::to_string(i), m_gnbNodes.Get(i));
    }

    for (uint32_t i = 0; i < m_accessNodes.GetN(); ++i)
    {
        Names::Add("Access" + std::to_string(i), m_accessNodes.Get(i));
    }

    for (uint32_t i = 0; i < m_preAggNodes.GetN(); ++i)
    {
        Names::Add("PreAgg" + std::to_string(i), m_preAggNodes.Get(i));
    }

    for (uint32_t i = 0; i < m_aggNodes.GetN(); ++i)
    {
        Names::Add("Agg" + std::to_string(i), m_aggNodes.Get(i));
    }

    for (uint32_t i = 0; i < m_coreNodes.GetN(); ++i)
    {
        Names::Add("Core" + std::to_string(i), m_coreNodes.Get(i));
    }

    for (uint32_t i = 0; i < m_upfNodes.GetN(); ++i)
    {
        Names::Add("UPF" + std::to_string(i), m_upfNodes.Get(i));
    }
}

FiveGTopologyHelper::~FiveGTopologyHelper()
{
}

void
FiveGTopologyHelper::CreateTopology()
{
    NS_LOG_INFO("[FiveGTopologyHelper] Creating 5G topology...");

    InternetStackHelper internet;
    internet.Install(hosts);
    internet.Install(switches);

    p2pGnbToAccess.SetDeviceAttribute("DataRate", StringValue("10Mbps")); // 10 Gbps
    p2pGnbToAccess.SetChannelAttribute("Delay", StringValue("0.5ms"));

    PointToPointHelper p2pAccessToPreAgg;
    p2pAccessToPreAgg.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2pAccessToPreAgg.SetChannelAttribute("Delay", StringValue("1ms"));

    p2pPreAggToAgg.SetDeviceAttribute("DataRate", StringValue("25Mbps"));
    p2pPreAggToAgg.SetChannelAttribute("Delay", StringValue("2ms"));

    p2pAggRing.SetDeviceAttribute("DataRate", StringValue("40Mbps"));
    p2pAggRing.SetChannelAttribute("Delay", StringValue("1ms"));

    p2pAggToCore.SetDeviceAttribute("DataRate", StringValue("100Mbps")); // 100 Gbps
    p2pAggToCore.SetChannelAttribute("Delay", StringValue("5ms"));

    // Connect gNBs to corresponding access nodes
    NetDeviceContainer devicePair =
        CreateLink(m_gnbNodes.Get(0), m_accessNodes.Get(0), p2pGnbToAccess);
    devicePairs.push_back(devicePair);

    devicePair = CreateLink(m_gnbNodes.Get(1), m_accessNodes.Get(1), p2pGnbToAccess);
    devicePairs.push_back(devicePair);

    devicePair = CreateLink(m_gnbNodes.Get(2), m_accessNodes.Get(2), p2pGnbToAccess);
    devicePairs.push_back(devicePair);

    // connect each access node to all pre-aggregation nodes
    for (uint32_t i = 0; i < m_accessNodes.GetN(); i++)
    {
        for (uint32_t j = 0; j < m_preAggNodes.GetN(); j++)
        {
            devicePair = CreateLink(m_accessNodes.Get(i), m_preAggNodes.Get(j), p2pAccessToPreAgg);
            devicePairs.push_back(devicePair);
        }
    }

    // UPF at the edge
    devicePair = CreateLink(m_preAggNodes.Get(0), m_upfNodes.Get(0), p2pPreAggToAgg);
    devicePairs.push_back(devicePair);

    devicePair = CreateLink(m_preAggNodes.Get(1), m_upfNodes.Get(0), p2pPreAggToAgg);
    devicePairs.push_back(devicePair);

    // connect pre-aggregation nodes to aggregation nodes
    devicePair = CreateLink(m_preAggNodes.Get(0), m_aggNodes.Get(0), p2pPreAggToAgg);
    devicePairs.push_back(devicePair);

    devicePair = CreateLink(m_preAggNodes.Get(1), m_aggNodes.Get(1), p2pPreAggToAgg);
    devicePairs.push_back(devicePair);

    devicePair = CreateLink(m_preAggNodes.Get(0), m_preAggNodes.Get(1), p2pPreAggToAgg);
    devicePairs.push_back(devicePair);

    // connect aggregation nodes in a ring
    devicePair = CreateLink(m_aggNodes.Get(0), m_aggNodes.Get(2), p2pAggRing);
    devicePairs.push_back(devicePair);

    devicePair = CreateLink(m_aggNodes.Get(1), m_aggNodes.Get(3), p2pAggRing);
    devicePairs.push_back(devicePair);

    devicePair = CreateLink(m_aggNodes.Get(0), m_aggNodes.Get(1), p2pAggRing);
    devicePairs.push_back(devicePair);

    devicePair = CreateLink(m_aggNodes.Get(2), m_aggNodes.Get(3), p2pAggRing);
    devicePairs.push_back(devicePair);

    // connect aggregation nodes to core nodes
    devicePair = CreateLink(m_aggNodes.Get(0), m_coreNodes.Get(0), p2pAggToCore);
    devicePairs.push_back(devicePair);

    devicePair = CreateLink(m_aggNodes.Get(1), m_coreNodes.Get(1), p2pAggToCore);
    devicePairs.push_back(devicePair);

    // connect core nodes in a ring
    devicePair = CreateLink(m_coreNodes.Get(0), m_coreNodes.Get(1), p2pAggToCore);
    devicePairs.push_back(devicePair);

    devicePair = CreateLink(m_coreNodes.Get(0), m_coreNodes.Get(2), p2pAggToCore);
    devicePairs.push_back(devicePair);

    devicePair = CreateLink(m_coreNodes.Get(1), m_coreNodes.Get(2), p2pAggToCore);
    devicePairs.push_back(devicePair);

    // connect core nodes to UPF at the core
    devicePair = CreateLink(m_coreNodes.Get(2), m_upfNodes.Get(1), p2pAggToCore);
    devicePairs.push_back(devicePair);

    AssignIPAddresses(devicePairs);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    MapSwitchesToNetDevices();
    if (m_customQueueDiscs)
    {
        NS_LOG_INFO("[FiveGTopologyHelper] Custom queue discs enabled");
        SetQueueDiscs(switchNetDevices);
    }
}

} // namespace ns3
