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

NS_LOG_COMPONENT_DEFINE("Example_16");

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

    LogComponentEnable("Example_16", LOG_LEVEL_INFO);
    // LogComponentEnable("CustomPacketSink", ns3::LOG_LEVEL_INFO);
    // LogComponentEnable("CustomTrafficGenerator", LOG_LEVEL_INFO);
    LogComponentEnable("Slice", LOG_LEVEL_INFO);
    LogComponentEnable("SliceHelper", LOG_LEVEL_INFO);

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

    NodeContainer hosts = topo.GetHosts();
    NodeContainer sources;
    NodeContainer sinks;

    sources.Add(hosts.Get(0));
    sources.Add(hosts.Get(1));
    sinks.Add(hosts.Get(2));

    Ptr<SliceHelper> sliceHelper = CreateObject<SliceHelper>();
    sliceHelper->SetAttribute("SimulationDuration", DoubleValue(10.0));
    sliceHelper->SetAttribute("MaxPackets", UintegerValue(1000));

    std::vector<Ptr<Slice>> slices = sliceHelper->CreateSlices(sources, sinks, 20);

    Simulator::Schedule(Seconds(1.0), &ProgressCallback);
    Simulator::Stop(totalSimDuration);

    Simulator::Run();

    sliceHelper->ReportSliceStats();

    Simulator::Destroy();

    return 0;
}
