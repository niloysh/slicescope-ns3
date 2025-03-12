/**
 * @file example_9.cc
 * @brief Example 9: SliceTrafficHelper for multiple slices
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/slicescope-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Example_9");

/*
 * (src1) n0 ---- n2 ---- n3 (dst)
 *        |       |
 * (src2) n1 ---- n4
 *
 */

Time totalSimDuration; // Total simulation duration

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

    LogComponentEnable("SimplePacketSink", LOG_LEVEL_INFO);
    LogComponentEnable("SliceTrafficHelper", LOG_LEVEL_INFO);

    totalSimDuration = Seconds(10);

    NodeContainer nodes;
    nodes.Create(5); // Create 5 nodes (n0 to n4)

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    // Define links
    NetDeviceContainer d02 = p2p.Install(nodes.Get(0), nodes.Get(2));
    NetDeviceContainer d01 = p2p.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer d14 = p2p.Install(nodes.Get(1), nodes.Get(4));
    NetDeviceContainer d24 = p2p.Install(nodes.Get(2), nodes.Get(4));
    NetDeviceContainer d23 = p2p.Install(nodes.Get(2), nodes.Get(3));

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i02 = ipv4.Assign(d02);

    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i01 = ipv4.Assign(d01);

    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i14 = ipv4.Assign(d14);

    ipv4.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i24 = ipv4.Assign(d24);

    ipv4.SetBase("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer i23 = ipv4.Assign(d23);

    // Enable automatic static routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 9; // Discard port (RFC 863)

    Ptr<SimplePacketSink> sinkApp = CreateObject<SimplePacketSink>();
    sinkApp->Setup(port);
    nodes.Get(3)->AddApplication(sinkApp); // n3
    sinkApp->SetStartTime(Seconds(0.0));
    sinkApp->SetStopTime(Seconds(10.0));

    // Get the IP address of n3 dynamically
    Ptr<Ipv4> ipv4_n3 = nodes.Get(3)->GetObject<Ipv4>();
    Ipv4Address ip_n3 = ipv4_n3->GetAddress(1, 0).GetLocal();

    std::unordered_map<SliceTrafficHelper::SliceType, double> sliceProbabilities = {
        {SliceTrafficHelper::eMBB, 0.5},
        {SliceTrafficHelper::URLLC, 0.5},
        {SliceTrafficHelper::mMTC, 0.0}};

    NodeContainer sources;
    sources.Add(nodes.Get(0));
    sources.Add(nodes.Get(1));

    SliceTrafficHelper trafficHelper(ip_n3, port);
    trafficHelper.SetSliceProbabilities(sliceProbabilities);
    trafficHelper.SetSources(sources);
    trafficHelper.SetMaxPackets(2);
    trafficHelper.SetNumSlices(3);
    trafficHelper.SetAppsPerSlice(1);
    // trafficHelper.SetSliceParams(SliceTrafficHelper::URLLC, {1.0, 5.0, 100, 1500, 1, 5});

    ApplicationContainer apps = trafficHelper.Install();
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(9.0));

    p2p.EnablePcap("example_3", d23.Get(1), true);

    Simulator::Schedule(Seconds(1.0), &ProgressCallback);
    Simulator::Stop(totalSimDuration);
    Simulator::Run();

    sinkApp->PrintStats();

    Simulator::Destroy();

    return 0;
}