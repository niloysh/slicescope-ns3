#ifndef TOPOLOGY_HELPER_H
#define TOPOLOGY_HELPER_H

#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/queue-disc-container.h"
#include "ns3/slice.h"
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
    // virtual void CreateTopology() = 0;

    NodeContainer GetSwitches();
    NodeContainer GetHosts();
    QueueDiscContainer GetQueueDiscs();

    int m_subnetCounter;
    bool m_customQueueDiscs;

    void SetQueueWeights(std::map<Slice::SliceType, uint32_t> sliceTypeToQueueWeightMap);

  protected:
    NodeContainer switches;
    NodeContainer hosts;

    std::vector<NetDeviceContainer> devicePairs;
    std::map<Ptr<Node>, NetDeviceContainer> switchNetDevices;
    QueueDiscContainer allQueueDiscs;

    InternetStackHelper internet;
    PointToPointHelper p2pHosts;
    PointToPointHelper p2pSwitches;
    Ipv4AddressHelper ipv4;

    NetDeviceContainer CreateLink(Ptr<Node> nodeA, Ptr<Node> nodeB, PointToPointHelper& p2p);
    void MapSwitchesToNetDevices();
    void AssignIPAddresses(std::vector<NetDeviceContainer>& devicePairs);
    void SetQueueDiscs(std::map<Ptr<Node>, NetDeviceContainer> switchNetDevices);

  private:
    std::map<Slice::SliceType, uint32_t> sliceTypeToQueueWeightMap;
};

} // namespace ns3

#endif // TOPOLOGY_HELPER_H