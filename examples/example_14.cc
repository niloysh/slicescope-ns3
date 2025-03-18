#include "ns3/application-helper.h"
#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/slicescope-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Example_14");

Time totalSimDuration = Seconds(10);

void
ProgressCallback()
{
    double progress = Simulator::Now().GetSeconds() / totalSimDuration.GetSeconds() * 100;
    std::cout << "[ " << progress << "% ] "
              << "Simulation time elapsed: " << Simulator::Now().GetSeconds() << "s" << std::endl;
    if (!Simulator::IsFinished())
    {
        Simulator::Schedule(Seconds(1.0), &ProgressCallback);
    }
}

int
main(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    LogComponentEnable("Example_14", LOG_LEVEL_INFO);
    LogComponentEnable("CustomPacketSink", LOG_LEVEL_INFO);
    LogComponentEnable("CustomTrafficGenerator", LOG_LEVEL_DEBUG);

    AdvancedTopologyHelper topo;
    PointToPointHelper p2pHosts;
    p2pHosts.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2pHosts.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper p2pSwitches;
    p2pSwitches.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
    p2pSwitches.SetChannelAttribute("Delay", StringValue("2ms"));

    topo.SetHostChannelHelper(p2pHosts);
    topo.SetSwitchChannelHelper(p2pSwitches);

    topo.CreateLinearTopology(3);

    // Get hosts
    NodeContainer hosts = topo.GetHosts();

    // Set up sink
    Ptr<CustomPacketSink> sinkApp = CreateObject<CustomPacketSink>();
    sinkApp->SetAttribute("Port", UintegerValue(9));
    hosts.Get(2)->AddApplication(sinkApp);

    // Get sink IP address
    Ipv4Address sinkAddr = hosts.Get(2)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

    Ptr<NormalRandomVariable> normalVar = CreateObject<NormalRandomVariable>();
    normalVar->SetAttribute("Mean", DoubleValue(1000));
    normalVar->SetAttribute("Variance", DoubleValue(200));

    // Helper function to create generators
    auto CreateGenerator = [&](Ptr<Node> node, uint32_t maxPackets) {
        Ptr<CustomTrafficGenerator> generator = CreateObject<CustomTrafficGenerator>();
        generator->SetAttribute("DestIp", Ipv4AddressValue(sinkAddr));
        generator->SetAttribute("DestPort", UintegerValue(9));
        generator->SetAttribute("DataRate", DoubleValue(10.0)); // Mbps
        generator->SetAttribute("PacketSizeVar", PointerValue(normalVar));
        generator->SetAttribute("MaxPackets", UintegerValue(maxPackets));
        node->AddApplication(generator);
        return generator;
    };

    // Create generator applications
    ApplicationContainer generatorApps;
    generatorApps.Add(CreateGenerator(hosts.Get(0), 2));
    generatorApps.Add(CreateGenerator(hosts.Get(1), 3));

    // Set start/stop times
    sinkApp->SetStartTime(Seconds(1.0));
    sinkApp->SetStopTime(Seconds(10.0));
    generatorApps.Start(Seconds(1.0));
    generatorApps.Stop(Seconds(9.0));

    Simulator::Schedule(Seconds(1.0), &ProgressCallback);
    Simulator::Stop(totalSimDuration);

    Simulator::Run();

    uint32_t totalPacketsSent = 0;
    uint32_t totalPacketsReceived = sinkApp->GetTotalPacketsReceived();

    for (uint32_t i = 0; i < generatorApps.GetN(); i++)
    {
        Ptr<CustomTrafficGenerator> generator =
            generatorApps.Get(i)->GetObject<CustomTrafficGenerator>();
        totalPacketsSent += generator->GetTotalPacketsSent();
    }

    std::vector<double> rtt = sinkApp->GetRtt();
    double rttMin = *std::min_element(rtt.begin(), rtt.end());
    double rttMax = *std::max_element(rtt.begin(), rtt.end());
    double rttAvg = std::accumulate(rtt.begin(), rtt.end(), 0.0) / rtt.size();

    NS_LOG_INFO("==== Simulation Summary ====");
    NS_LOG_INFO("Total sent: " << totalPacketsSent << " packets");
    NS_LOG_INFO("Total received: " << totalPacketsReceived << " packets");
    NS_LOG_INFO("RTT min: " << rttMin * 1000 << "ms");
    NS_LOG_INFO("RTT max: " << rttMax * 1000 << "ms");
    NS_LOG_INFO("RTT avg: " << rttAvg * 1000 << "ms");
    NS_LOG_INFO("==== End Simulation ====");

    Simulator::Destroy();

    return 0;
}
