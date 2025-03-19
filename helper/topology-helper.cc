#include "topology-helper.h"

#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/custom-queue-disc.h"
#include "ns3/internet-module.h"
#include "ns3/names.h"
#include "ns3/network-module.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ns3/traffic-control-helper.h"

#include <cstdint>
#include <sys/types.h>
#include <unordered_set>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TopologyHelper");

TypeId
TopologyHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TopologyHelper")
                            .SetParent<Object>()
                            .SetGroupName("Helper")
                            .AddConstructor<TopologyHelper>()
                            .AddAttribute("SubnetCounter",
                                          "Counter for subnet addresses",
                                          IntegerValue(1),
                                          MakeIntegerAccessor(&TopologyHelper::subnetCounter),
                                          MakeIntegerChecker<int>())
                            .AddAttribute("CustomQueueDiscs",
                                          "Enable custom queue discs",
                                          BooleanValue(false),
                                          MakeBooleanAccessor(&TopologyHelper::m_customQueueDiscs),
                                          MakeBooleanChecker());
    return tid;
}

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

    for (uint32_t i = 0; i < hosts.GetN(); i++)
    {
        std::ostringstream name;
        name << "h" << i;
        Names::Add(name.str(), hosts.Get(i));
    }

    for (uint32_t i = 0; i < switches.GetN(); i++)
    {
        std::ostringstream name;
        name << "s" << i;
        Names::Add(name.str(), switches.Get(i));
    }

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

    for (uint32_t i = 0; i < switches.GetN(); i++)
    {
        Ptr<Node> node = switches.Get(i);
        uint32_t numNetDevices = node->GetNDevices();
        NS_LOG_DEBUG("[TopologyHelper] Switch " << i << " has " << numNetDevices << " net devices");
        for (uint32_t j = 0; j < numNetDevices; j++)
        {
            Ptr<NetDevice> netDevice = node->GetDevice(j);
            Ipv4Address ipv4Addr = node->GetObject<Ipv4>()->GetAddress(j, 0).GetLocal();
            NS_LOG_DEBUG("[TopologyHelper] Switch " << i << " port " << j << " | IP " << ipv4Addr);

            // Skip the first net device (loopback)
            if (j != 0)
            {
                switchNetDevices[node].Add(netDevice);
            }
        }
    }

    if (m_customQueueDiscs)
    {
        TopologyHelper::SetQueueDiscs(switchNetDevices);
    }
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

void
TopologyHelper::SetQueueDiscs(std::map<Ptr<Node>, NetDeviceContainer> switchNetDevices)
{
    for (auto it = switchNetDevices.begin(); it != switchNetDevices.end(); ++it)
    {
        Ptr<Node> node = it->first;
        NetDeviceContainer netDevices = it->second;
        std::string nodeName = Names::FindName(node);

        TrafficControlHelper tch;
        tch.Uninstall(netDevices); // Remove existing queue discs

        tch.SetRootQueueDisc("ns3::CustomQueueDisc");
        QueueDiscContainer queueDiscs = tch.Install(netDevices);

        for (uint32_t i = 0; i < netDevices.GetN(); i++)
        {
            Ptr<NetDevice> device = netDevices.Get(i);
            Ptr<QueueDisc> queueDisc = queueDiscs.Get(i);
            NS_LOG_DEBUG("[TopologyHelper] Installing QueueDisc on " << nodeName << " port "
                                                                     << i + 1);
            queueDisc->SetAttribute("Node", PointerValue(node));
            queueDisc->SetAttribute("NetDevice", PointerValue(device));
            queueDisc->SetAttribute("Port", UintegerValue(i + 1));

            allQueueDiscs.Add(queueDisc);
        }
    }
}

QueueDiscContainer
TopologyHelper::GetQueueDiscs()
{
    return allQueueDiscs;
}

} // namespace ns3