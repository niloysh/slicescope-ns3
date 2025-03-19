#ifndef TOPOLOGY_HELPER_H
#define TOPOLOGY_HELPER_H

#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/queue-disc-container.h"
#include "ns3/string.h"

#include <cstdint>

namespace ns3
{

class TopologyHelper : public Object
{
  public:
    static TypeId GetTypeId();
    TopologyHelper();
    ~TopologyHelper() override;

    /**
     * \brief Method to manually create the topology based on host-switch and inter-switch links
     * @param hostSwitchLinks A vector of pairs representing the host-switch links
     * @param interSwitchLinks A vector of pairs representing the inter-switch links
     */
    void CreateTopology(std::vector<std::pair<uint32_t, uint32_t>> hostSwitchLinks,
                        std::vector<std::pair<uint32_t, uint32_t>> interSwitchLinks);

    void SetHostChannelHelper(PointToPointHelper p2pHosts);
    void SetSwitchChannelHelper(PointToPointHelper p2pSwitches);

    NodeContainer GetSwitches();
    NodeContainer GetHosts();
    QueueDiscContainer GetQueueDiscs();

  private:
    NodeContainer switches;
    NodeContainer hosts;

    std::vector<NodeContainer> nodePairsHostSwitch;
    std::vector<NetDeviceContainer> devicePairsHostSwitch;
    std::vector<NodeContainer> nodePairsInterSwitch;
    std::vector<NetDeviceContainer> devicePairsInterSwitch;
    std::vector<Ipv4InterfaceContainer> interfacePairs;
    std::map<Ptr<Node>, NetDeviceContainer> switchNetDevices; // Switch to net device (port) mapping
    QueueDiscContainer allQueueDiscs;

    InternetStackHelper internet;
    PointToPointHelper p2pHosts;
    PointToPointHelper p2pSwitches;
    Ipv4AddressHelper ipv4;

    int subnetCounter;

    /**
     * \brief Method to create links between nodes in a node group
     * @param links A vector of pairs representing the links to be created
     * @param nodeGroupA The first node group
     * @param nodeGroupB The second node group
     * @param nodePairs A vector of node containers to store the node pairs
     * @param p2p The point-to-point helper to use for creating the links
     * @param devicePairs A vector of net device containers to store the device pairs
     */
    void CreateLinks(const std::vector<std::pair<uint32_t, uint32_t>>& links,
                     NodeContainer& nodeGroupA,
                     NodeContainer& nodeGroupB,
                     std::vector<NodeContainer>& nodePairs,
                     PointToPointHelper& p2p,
                     std::vector<NetDeviceContainer>& devicePairs);

    /**
     * \brief Method to assign IP addresses to the interfaces of the created links
     * @param devicePairs A vector of net device containers representing the device pairs
     */
    void AssignIPAddresses(std::vector<NetDeviceContainer>& devicePairs);
    void SetQueueDiscs(std::map<Ptr<Node>, NetDeviceContainer> switchNetDevices);
    bool m_customQueueDiscs;
};

} // namespace ns3

#endif // TOPOLOGY_HELPER_H