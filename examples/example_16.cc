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

#include <iostream>

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
    std::string topologyType = "linear";
    CommandLine cmd;
    cmd.AddValue("topology", "Topology type (linear, fattree)", topologyType);
    cmd.Parse(argc, argv);

    ns3::RngSeedManager::SetSeed(2); // seed 2
    ns3::RngSeedManager::SetRun(1);  // run 1

    LogComponentEnable("Example_16", LOG_LEVEL_INFO);
    LogComponentEnable("CustomPacketSink", ns3::LOG_LEVEL_WARN);
    LogComponentEnable("CustomTrafficGenerator", LOG_LEVEL_WARN);
    LogComponentEnable("Slice", LOG_LEVEL_INFO);
    LogComponentEnable("SliceHelper", LOG_LEVEL_INFO);
    LogComponentEnable("LinearTopologyHelper", LOG_LEVEL_INFO);
    LogComponentEnable("FatTreeTopologyHelper", LOG_LEVEL_INFO);
    LogComponentEnable("TopologyHelper", ns3::LOG_LEVEL_INFO);
    LogComponentEnable("CustomQueueDisc", LOG_LEVEL_INFO);

    if (topologyType != "linear" && topologyType != "fattree")
    {
        NS_LOG_ERROR("Invalid topology type. Use --topology=linear or --topology=fattree");
        return 1;
    }

    NodeContainer sources;
    NodeContainer sinks;

    Ptr<TopologyHelper> topo;

    if (topologyType == "linear")
    {
        PointToPointHelper p2pHosts;
        p2pHosts.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
        p2pHosts.SetChannelAttribute("Delay", StringValue("1ms"));

        PointToPointHelper p2pSwitches;
        p2pSwitches.SetDeviceAttribute("DataRate", StringValue("65Mbps"));
        p2pSwitches.SetChannelAttribute("Delay", StringValue("1ms"));

        NS_LOG_INFO("Creating linear topology...");
        topo = CreateObject<LinearTopologyHelper>();
        Ptr<LinearTopologyHelper> linearTopo = DynamicCast<LinearTopologyHelper>(topo);
        linearTopo->SetHostChannelHelper(p2pHosts);
        linearTopo->SetSwitchChannelHelper(p2pSwitches);
        linearTopo->SetAttribute("CustomQueueDiscs", BooleanValue(true));
        linearTopo->CreateTopology(3);
        NodeContainer hosts = linearTopo->GetHosts();

        sources.Add(hosts.Get(0));
        sources.Add(hosts.Get(1));
        sinks.Add(hosts.Get(2));
    }
    else
    {
        PointToPointHelper p2pCoreToAgg;
        p2pCoreToAgg.SetDeviceAttribute("DataRate", StringValue("400Mbps"));
        p2pCoreToAgg.SetChannelAttribute("Delay", StringValue("1ms"));

        PointToPointHelper p2pAggToEdge;
        p2pAggToEdge.SetDeviceAttribute("DataRate", StringValue("200Mbps"));
        p2pAggToEdge.SetChannelAttribute("Delay", StringValue("1ms"));

        PointToPointHelper p2pEdgetoHost;
        p2pEdgetoHost.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
        p2pEdgetoHost.SetChannelAttribute("Delay", StringValue("1ms"));

        NS_LOG_INFO("Creating fat-tree topology...");
        topo = CreateObject<FatTreeTopologyHelper>();
        Ptr<FatTreeTopologyHelper> fatTreeTopo = DynamicCast<FatTreeTopologyHelper>(topo);
        fatTreeTopo->SetCoreToAggChannelHelper(p2pCoreToAgg);
        fatTreeTopo->SetAggToEdgeChannelHelper(p2pAggToEdge);
        fatTreeTopo->SetEdgeToHostChannelHelper(p2pEdgetoHost);
        fatTreeTopo->SetAttribute("CustomQueueDiscs", BooleanValue(true));
        fatTreeTopo->CreateTopology(4);
        NodeContainer hosts = fatTreeTopo->GetHosts();

        sources.Add(hosts.Get(0));
        sources.Add(hosts.Get(1));
        sources.Add(hosts.Get(2));
        sources.Add(hosts.Get(3));
        sinks.Add(hosts.Get(15));
    }

    topo->SetQueueWeights({{Slice::URLLC, 80}, {Slice::eMBB, 15}, {Slice::mMTC, 5}});

    Ptr<SliceHelper> sliceHelper = CreateObject<SliceHelper>();
    sliceHelper->SetAttribute("SimulationDuration", DoubleValue(totalSimDuration.GetSeconds()));
    sliceHelper->SetAttribute("MaxPackets", UintegerValue(0));
    sliceHelper->SetAttribute("NumApps", UintegerValue(2));

    std::vector<Ptr<Slice>> slices = sliceHelper->CreateSlices(sources, sinks, 3);

    Simulator::Schedule(Seconds(1.0), &ProgressCallback);
    Simulator::Stop(totalSimDuration);

    Simulator::Run();

    sliceHelper->ReportSliceStats();
    QueueDiscContainer allQueueDiscs = topo->GetQueueDiscs();
    PrintQueueStatistics(allQueueDiscs);
    sliceHelper->ExportOwdRecords("owd_records.csv");
    Simulator::Destroy();

    return 0;
}
