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

void
PrintQueueStatistics(QueueDiscContainer allQueueDiscs)
{
    NS_LOG_INFO("====== Queue Statistics ======");
    NS_LOG_INFO("Number of queue discs: " << allQueueDiscs.GetN());

    for (uint32_t i = 0; i < allQueueDiscs.GetN(); i++)
    {
        Ptr<CustomQueueDisc> queueDisc = DynamicCast<CustomQueueDisc>(allQueueDiscs.Get(i));
        if (!queueDisc)
        {
            continue;
        }

        queueDisc->PrintQueueStatistics();
    }
}

int
main(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    ns3::RngSeedManager::SetSeed(1);
    ns3::RngSeedManager::SetRun(7);

    LogComponentEnable("Example_16", LOG_LEVEL_INFO);
    LogComponentEnable("CustomPacketSink", ns3::LOG_LEVEL_WARN);
    LogComponentEnable("CustomTrafficGenerator", LOG_LEVEL_WARN);
    LogComponentEnable("Slice", LOG_LEVEL_INFO);
    LogComponentEnable("SliceHelper", LOG_LEVEL_INFO);
    LogComponentEnable("LinearTopologyHelper", LOG_LEVEL_INFO);
    LogComponentEnable("FatTreeTopologyHelper", LOG_LEVEL_INFO);
    LogComponentEnable("TopologyHelper", ns3::LOG_LEVEL_INFO);
    LogComponentEnable("CustomQueueDisc", LOG_LEVEL_INFO);

    // PointToPointHelper p2pHosts;
    // p2pHosts.SetDeviceAttribute("DataRate", StringValue("500Mbps"));
    // p2pHosts.SetChannelAttribute("Delay", StringValue("1ms"));

    // PointToPointHelper p2pSwitches;
    // p2pSwitches.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    // p2pSwitches.SetChannelAttribute("Delay", StringValue("1ms"));

    // Ptr<LinearTopologyHelper> linearTopo = CreateObject<LinearTopologyHelper>();
    // linearTopo->SetHostChannelHelper(p2pHosts);
    // linearTopo->SetSwitchChannelHelper(p2pSwitches);
    // linearTopo->SetAttribute("CustomQueueDiscs", BooleanValue(true));
    // linearTopo->CreateTopology(3);
    // NodeContainer hosts = linearTopo->GetHosts();

    PointToPointHelper p2pCoreToAgg;
    p2pCoreToAgg.SetDeviceAttribute("DataRate", StringValue("4Gbps"));
    p2pCoreToAgg.SetChannelAttribute("Delay", StringValue("1ms"));

    PointToPointHelper p2pAggToEdge;
    p2pAggToEdge.SetDeviceAttribute("DataRate", StringValue("2Gbps"));
    p2pAggToEdge.SetChannelAttribute("Delay", StringValue("1ms"));

    PointToPointHelper p2pEdgetoHost;
    p2pEdgetoHost.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2pEdgetoHost.SetChannelAttribute("Delay", StringValue("1ms"));

    Ptr<FatTreeTopologyHelper> fatTreeTopo = CreateObject<FatTreeTopologyHelper>();
    fatTreeTopo->SetCoreToAggChannelHelper(p2pCoreToAgg);
    fatTreeTopo->SetAggToEdgeChannelHelper(p2pAggToEdge);
    fatTreeTopo->SetEdgeToHostChannelHelper(p2pEdgetoHost);
    fatTreeTopo->SetAttribute("CustomQueueDiscs", BooleanValue(true));
    fatTreeTopo->CreateTopology(4);
    NodeContainer hosts = fatTreeTopo->GetHosts();

    NodeContainer sources;
    NodeContainer sinks;

    sources.Add(hosts.Get(0));
    sources.Add(hosts.Get(1));
    sources.Add(hosts.Get(2));
    sources.Add(hosts.Get(3));
    sinks.Add(hosts.Get(15));

    Ptr<SliceHelper> sliceHelper = CreateObject<SliceHelper>();
    sliceHelper->SetAttribute("SimulationDuration", DoubleValue(totalSimDuration.GetSeconds()));
    sliceHelper->SetAttribute("MaxPackets", UintegerValue(100));

    std::vector<Ptr<Slice>> slices = sliceHelper->CreateSlices(sources, sinks, 2);

    Simulator::Schedule(Seconds(1.0), &ProgressCallback);
    Simulator::Stop(totalSimDuration);

    Simulator::Run();

    sliceHelper->ReportSliceStats();
    QueueDiscContainer allQueueDiscs = fatTreeTopo->GetQueueDiscs();
    PrintQueueStatistics(allQueueDiscs);
    Simulator::Destroy();

    return 0;
}
