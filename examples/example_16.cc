/**
 * @file example_16.cc
 * @brief Large-scale slice-aware simulation supporting multiple topologies, TCP flows, and queue
 * instrumentation
 *
 * ### Topologies Supported
 * - `linear` : Chain of switches and hosts (default)
 * - `fattree`: Multi-tier data center topology
 * - `fiveg`  : Core + RAN topology with congestion sources
 *
 * ### Run
 * ./ns3 run "scratch/example_16 --topology=linear"
 *
 * ### Key Features
 * - Slice-based flow orchestration across TCP connections
 * - Background traffic generation with randomized bursts
 * - Queue weight configuration and custom queue disc stats
 * - ECMP routing and TCP CUBIC tuning with large buffers
 * - Real-time logs for simulation progress and sink data rates
 * - Support for RTT, OWD logging and exportable stats
 *
 * ### Output
 * - Aggregate throughput at sinks (stdout)
 * - Queue statistics (log)
 * - Per-slice performance metrics (log)
 * - OWD records: `owd_records.csv`
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

#include <iostream>
#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Example_16");

Time totalSimDuration = Seconds(10);
std::map<uint32_t, uint64_t> lastRxBytes; // nodeId -> bytes

void
PrintSinkDataRates(NodeContainer sinks, Time interval)
{
    double totalRateMbps = 0.0;

    for (auto it = sinks.Begin(); it != sinks.End(); ++it)
    {
        Ptr<Node> sinkNode = *it;
        uint32_t nodeId = sinkNode->GetId();

        uint64_t totalRxBytes = 0;

        // Sum over all applications on the node
        for (uint32_t i = 0; i < sinkNode->GetNApplications(); ++i)
        {
            Ptr<Application> app = sinkNode->GetApplication(i);
            Ptr<CustomPacketSink> customSinkApp = DynamicCast<CustomPacketSink>(app);
            if (customSinkApp)
            {
                totalRxBytes += customSinkApp->GetTotalRx();
            }

            Ptr<PacketSink> sinkApp = DynamicCast<PacketSink>(app);
            if (sinkApp)
            {
                totalRxBytes += sinkApp->GetTotalRx();
            }
        }

        uint64_t lastBytes = lastRxBytes[nodeId];
        uint64_t deltaBytes = totalRxBytes - lastBytes;
        double rateMbps = (deltaBytes * 8.0) / interval.GetSeconds() / 1e6;

        lastRxBytes[nodeId] = totalRxBytes;
        totalRateMbps += rateMbps;
    }

    std::cout << "[DataRate] Time " << Simulator::Now().GetSeconds()
              << "s - Aggregate Sink Rate: " << totalRateMbps << " Mbps" << std::endl;

    Simulator::Schedule(interval, &PrintSinkDataRates, sinks, interval);
}

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
    cmd.AddValue("topology", "Topology type (linear, fattree, fiveg)", topologyType);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    ns3::RngSeedManager::SetSeed(2); // seed 2
    ns3::RngSeedManager::SetRun(2);  // run 1

    // Set TCP variant to CUBIC
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpCubic::GetTypeId()));

    // increase socket buffer size to avoid sender-side stalling
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 20)); // 1 MB
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 20)); // 1 MB

    // Enable ecmp
    Config::SetDefault("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue(true));
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));

    LogComponentEnable("Example_16", LOG_LEVEL_INFO);
    LogComponentEnable("CustomPacketSink", ns3::LOG_LEVEL_WARN);
    LogComponentEnable("CustomTrafficGenerator", LOG_LEVEL_WARN);
    LogComponentEnable("Slice", LOG_LEVEL_INFO);
    LogComponentEnable("SliceHelper", LOG_LEVEL_INFO);
    LogComponentEnable("LinearTopologyHelper", LOG_LEVEL_INFO);
    LogComponentEnable("FatTreeTopologyHelper", LOG_LEVEL_INFO);
    LogComponentEnable("FiveGTopologyHelper", LOG_LEVEL_INFO);
    LogComponentEnable("TopologyHelper", ns3::LOG_LEVEL_INFO);
    LogComponentEnable("CustomQueueDisc", LOG_LEVEL_INFO);
    LogComponentEnable("BackgroundTrafficHelper", LOG_LEVEL_INFO);

    if (topologyType != "linear" && topologyType != "fattree" && topologyType != "fiveg")
    {
        NS_LOG_ERROR("Invalid topology type. Use --topology=linear or --topology=fattree or "
                     "--topology=fiveg");
        return 1;
    }

    NodeContainer sources;
    NodeContainer sinks;
    NodeContainer hosts;

    Ptr<TopologyHelper> topo;
    Ptr<Node> bgSource;
    Ptr<Node> bgSink;
    BackgroundTrafficHelper bgHelper;

    if (topologyType == "linear")
    {
        PointToPointHelper p2pHosts;
        p2pHosts.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
        p2pHosts.SetChannelAttribute("Delay", StringValue("1ms"));

        PointToPointHelper p2pSwitches;
        p2pSwitches.SetDeviceAttribute("DataRate", StringValue("100Mbps")); // 65Mbps
        p2pSwitches.SetChannelAttribute("Delay", StringValue("1ms"));

        NS_LOG_INFO("Creating linear topology...");
        topo = CreateObject<LinearTopologyHelper>();
        Ptr<LinearTopologyHelper> linearTopo = DynamicCast<LinearTopologyHelper>(topo);
        linearTopo->SetHostChannelHelper(p2pHosts);
        linearTopo->SetSwitchChannelHelper(p2pSwitches);
        linearTopo->SetAttribute("CustomQueueDiscs", BooleanValue(true));
        linearTopo->CreateTopology(3);
        hosts = linearTopo->GetHosts();

        sources.Add(hosts.Get(0));
        sources.Add(hosts.Get(1));
        sinks.Add(hosts.Get(2));

        bgSource = hosts.Get(0);
        bgSink = hosts.Get(1);
    }
    else if (topologyType == "fattree")
    {
        PointToPointHelper p2pCoreToAgg;
        p2pCoreToAgg.SetDeviceAttribute("DataRate", StringValue("2000Mbps"));
        p2pCoreToAgg.SetChannelAttribute("Delay", StringValue("1ms"));

        PointToPointHelper p2pAggToEdge;
        p2pAggToEdge.SetDeviceAttribute("DataRate", StringValue("500Mbps"));
        p2pAggToEdge.SetChannelAttribute("Delay", StringValue("1ms"));

        PointToPointHelper p2pEdgetoHost;
        p2pEdgetoHost.SetDeviceAttribute("DataRate", StringValue("500Mbps"));
        p2pEdgetoHost.SetChannelAttribute("Delay", StringValue("1ms"));

        NS_LOG_INFO("Creating fat-tree topology...");
        topo = CreateObject<FatTreeTopologyHelper>();
        Ptr<FatTreeTopologyHelper> fatTreeTopo = DynamicCast<FatTreeTopologyHelper>(topo);
        fatTreeTopo->SetCoreToAggChannelHelper(p2pCoreToAgg);
        fatTreeTopo->SetAggToEdgeChannelHelper(p2pAggToEdge);
        fatTreeTopo->SetEdgeToHostChannelHelper(p2pEdgetoHost);
        fatTreeTopo->SetAttribute("CustomQueueDiscs", BooleanValue(true));
        fatTreeTopo->CreateTopology(4);
        hosts = fatTreeTopo->GetHosts();

        sources.Add(hosts.Get(0));
        sources.Add(hosts.Get(1));
        sources.Add(hosts.Get(2));
        sources.Add(hosts.Get(3));
        sinks.Add(hosts.Get(15));

        bgSource = hosts.Get(4);
        bgSink = hosts.Get(14);
    }

    else if (topologyType == "fiveg")
    {
        NS_LOG_INFO("Creating 5G topology...");
        topo = CreateObject<FiveGTopologyHelper>();
        Ptr<FiveGTopologyHelper> fiveGTopo = DynamicCast<FiveGTopologyHelper>(topo);
        fiveGTopo->SetAttribute("CustomQueueDiscs", BooleanValue(true));
        fiveGTopo->CreateTopology();
        hosts = fiveGTopo->GetHosts();

        sources.Add(fiveGTopo->m_gnbNodes);
        sinks.Add(fiveGTopo->m_upfNodes.Get(1)); // UPF at the core

        NodeContainer congestionSources = fiveGTopo->m_congestionSources;
        NodeContainer congestionSinks = fiveGTopo->m_congestionSinks;

        bgHelper.ScheduleRandomBursts(BackgroundTrafficHelper::BULK,
                                      congestionSources,
                                      congestionSinks,
                                      totalSimDuration.GetSeconds(),
                                      30,
                                      "10Mbps",
                                      "500Mbps",
                                      0.5,
                                      5);
    }

    topo->SetQueueWeights({{Slice::URLLC, 80}, {Slice::eMBB, 15}, {Slice::mMTC, 5}});

    Ptr<SliceHelper> sliceHelper = CreateObject<SliceHelper>();
    sliceHelper->SetAttribute("SimulationDuration", DoubleValue(totalSimDuration.GetSeconds()));
    sliceHelper->SetAttribute("MaxPackets", UintegerValue(0));
    sliceHelper->SetAttribute("NumApps", UintegerValue(0));

    std::map<Slice::SliceType, uint32_t> numSlicesPerType = {{Slice::URLLC, 5}, // 2
                                                             {Slice::eMBB, 5},  // 5
                                                             {Slice::mMTC, 5}}; // 3

    std::vector<Ptr<Slice>> slices = sliceHelper->CreateSlices(sources, sinks, numSlicesPerType);

    Simulator::Schedule(Seconds(1.0), &ProgressCallback);
    NodeContainer allSinks;
    allSinks.Add(sinks);
    Simulator::Schedule(Seconds(1.0), &PrintSinkDataRates, allSinks, Seconds(1.0));
    Simulator::Stop(totalSimDuration);

    Simulator::Run();

    sliceHelper->ReportSliceStats();
    QueueDiscContainer allQueueDiscs = topo->GetQueueDiscs();
    PrintQueueStatistics(allQueueDiscs);
    sliceHelper->ExportOwdRecords("owd_records.csv");

    NS_LOG_INFO("====== Background Traffic Statistics ======");
    NS_LOG_INFO("Bytes sent: " << bgHelper.GetTotalBytesSent());
    NS_LOG_INFO("Bytes received: " << bgHelper.GetTotalBytesReceived());

    Simulator::Destroy();

    return 0;
}
