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
#include <string>
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
                            .AddConstructor<TopologyHelper>();
    return tid;
}

TopologyHelper::TopologyHelper()
    : m_subnetCounter(1)
{
}

TopologyHelper::~TopologyHelper()
{
}

NetDeviceContainer
TopologyHelper::CreateLink(Ptr<Node> nodeA, Ptr<Node> nodeB, PointToPointHelper& p2p)
{
    std::string nodeNameA = Names::FindName(nodeA);
    std::string nodeNameB = Names::FindName(nodeB);
    NS_LOG_DEBUG("[TopologyHelper] Creating link between " << nodeNameA << " and " << nodeNameB);
    NodeContainer nodePair = NodeContainer(nodeA, nodeB);
    return p2p.Install(nodePair);
}

void
TopologyHelper::AssignIPAddresses(std::vector<NetDeviceContainer>& devicePairs)
{
    for (const auto& devicePair : devicePairs)
    {
        std::ostringstream subnet;
        subnet << "10.1." << m_subnetCounter << ".0";
        NS_LOG_DEBUG("[TopologyHelper] Assigning IP addresses for subnet " << subnet.str());

        ipv4.SetBase(subnet.str().c_str(), "255.255.255.0");
        Ipv4InterfaceContainer interfacePair = ipv4.Assign(devicePair);
        m_subnetCounter++;
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
        NS_LOG_DEBUG("[TopologyHelper] Setting QueueDiscs for " << nodeName);

        TrafficControlHelper tch;
        tch.Uninstall(netDevices); // Remove existing queue discs

        tch.SetRootQueueDisc("ns3::CustomQueueDisc");
        QueueDiscContainer queueDiscs = tch.Install(netDevices);

        if (queueDiscs.GetN() != netDevices.GetN())
        {
            NS_LOG_ERROR("[TopologyHelper] QueueDisc installation failed for " << nodeName);
            continue;
        }

        for (uint32_t i = 0; i < netDevices.GetN(); i++)
        {
            Ptr<NetDevice> device = netDevices.Get(i);
            Ptr<CustomQueueDisc> queueDisc = DynamicCast<CustomQueueDisc>(queueDiscs.Get(i));
            NS_LOG_DEBUG("[TopologyHelper] Installing QueueDisc on " << nodeName << " port "
                                                                     << i + 1);
            queueDisc->SetAttribute("Node", PointerValue(node));
            queueDisc->SetAttribute("NetDevice", PointerValue(device));
            queueDisc->SetAttribute("Port", UintegerValue(i + 1));
            queueDisc->SetQueueWeights(sliceTypeToQueueWeightMap);

            allQueueDiscs.Add(queueDisc);
        }
    }
}

QueueDiscContainer
TopologyHelper::GetQueueDiscs()
{
    return allQueueDiscs;
}

void
TopologyHelper::MapSwitchesToNetDevices()
{
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
}

void
TopologyHelper::SetQueueWeights(std::map<Slice::SliceType, uint32_t> sliceTypeToQueueWeightMap)
{
    for (uint32_t i = 0; i < allQueueDiscs.GetN(); i++)
    {
        Ptr<CustomQueueDisc> queueDisc = DynamicCast<CustomQueueDisc>(allQueueDiscs.Get(i));
        if (!queueDisc)
        {
            continue;
        }

        queueDisc->SetQueueWeights(sliceTypeToQueueWeightMap);
    }
}

} // namespace ns3