/**
 * @file example_15.cc
 * @brief Demonstrates how to configure and run a single Slice in a linear topology
 *
 * ### Topology
 * ```
 * Host 0 --- Switch 0 --- Switch 1 --- Switch 2 --- Host 2
 * ```
 *
 * - Built using `LinearTopologyHelper::CreateTopology(3)`
 * - Host 0 is source, Host 2 is sink
 * - 1 Gbps host-to-switch, 10 Gbps inter-switch, 2 ms delay
 *
 * ### Key Features
 * - Demonstrates use of a `Slice` to configure traffic apps automatically
 * - Slice type set to `eMBB` via attribute
 * - Automatically installs `CustomTrafficGenerator` and `CustomPacketSink`
 * - Reports total packets sent/received at the end
 *
 * ### Run
 *
 * ./ns3 run "scratch/example_15"
 *
 */

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

NS_LOG_COMPONENT_DEFINE("Example_15");

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

    LogComponentEnable("Example_15", LOG_LEVEL_INFO);
    // LogComponentEnable("CustomPacketSink", ns3::LOG_LEVEL_INFO);
    // LogComponentEnable("CustomTrafficGenerator", LOG_LEVEL_INFO);
    LogComponentEnable("Slice", LOG_LEVEL_INFO);

    LinearTopologyHelper topo;
    PointToPointHelper p2pHosts;
    p2pHosts.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2pHosts.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper p2pSwitches;
    p2pSwitches.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
    p2pSwitches.SetChannelAttribute("Delay", StringValue("2ms"));

    topo.SetHostChannelHelper(p2pHosts);
    topo.SetSwitchChannelHelper(p2pSwitches);

    topo.CreateTopology(3);

    // Get hosts
    NodeContainer hosts = topo.GetHosts();

    Ptr<Slice> slice = CreateObject<Slice>();
    slice->SetAttribute("SliceType", EnumValue(Slice::eMBB));
    slice->SetAttribute("SourceNode", PointerValue(hosts.Get(0)));
    slice->SetAttribute("SinkNode", PointerValue(hosts.Get(2)));
    slice->SetAttribute("NumApps", UintegerValue(1));
    slice->SetAttribute("MaxPackets", UintegerValue(0));
    slice->SetAttribute("StartTime", DoubleValue(5.0));
    slice->SetAttribute("StopTime", DoubleValue(totalSimDuration.GetSeconds()));
    slice->InstallApps();

    Simulator::Schedule(Seconds(1.0), &ProgressCallback);
    Simulator::Stop(totalSimDuration);

    Simulator::Run();

    std::vector<ApplicationContainer> sourceApps = slice->GetSourceApps();
    std::vector<ApplicationContainer> sinkApps = slice->GetSinkApps();

    uint32_t totalPacketsSent = 0;
    uint32_t totalPacketsReceived = 0;

    for (uint32_t i = 0; i < sourceApps.size(); i++)
    {
        Ptr<CustomTrafficGenerator> generator =
            sourceApps[i].Get(0)->GetObject<CustomTrafficGenerator>();
        totalPacketsSent += generator->GetTotalPacketsSent();
    }

    for (uint32_t i = 0; i < sinkApps.size(); i++)
    {
        Ptr<CustomPacketSink> sink = sinkApps[i].Get(0)->GetObject<CustomPacketSink>();
        totalPacketsReceived += sink->GetTotalRxPackets();
    }

    NS_LOG_INFO("==== Simulation Summary ====");
    NS_LOG_INFO("Total sent: " << totalPacketsSent << " packets");
    NS_LOG_INFO("Total received: " << totalPacketsReceived << " packets");
    NS_LOG_INFO("==== End Simulation ====");

    Simulator::Destroy();

    return 0;
}
