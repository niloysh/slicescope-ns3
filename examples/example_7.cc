/**
 * @file example_7.cc
 * @brief Demonstrates how to use the CustomTrafficGenerator to send packets
 *
 * ### Topology
 * ```
 * (src1) n0 ---- n2 ---- n3 (dst)
 *        |       |
 * (src2) n1 ---- n4
 * ```
 *
 * - Point-to-point links (10 Mbps, 2 ms delay)
 * - n0 and n1 run `CustomTrafficGenerator` applications
 * - n3 runs a `CustomPacketSink` to receive and log packets
 *
 * ### Key Features
 * - Manual setup of source/destination apps using CustomTrafficGenerator and CustomPacketSink
 * - Packet burst control via `MaxPackets` attribute
 *
 * ### Run
 *
 * ./ns3 run "scratch/example_7"
 *
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

NS_LOG_COMPONENT_DEFINE("Example_7");

/*
 * (src1) n0 ---- n2 ---- n3 (dst)
 *        |       |
 * (src2) n1 ---- n4
 *
 */

int
main(int argc, char* argv[])
{
    CommandLine cmd;

    LogComponentEnable("CustomPacketSink", LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable("CustomTrafficGenerator", LOG_LEVEL_INFO);
    LogComponentEnable("Example_7", LOG_LEVEL_INFO);

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

    Ptr<CustomPacketSink> sinkApp = CreateObject<CustomPacketSink>();
    sinkApp->SetAttribute("Port", UintegerValue(port));
    nodes.Get(3)->AddApplication(sinkApp); // n3
    sinkApp->SetStartTime(Seconds(0.0));
    sinkApp->SetStopTime(Seconds(10.0));

    // Get the IP address of n3 dynamically
    Ptr<Ipv4> ipv4_n3 = nodes.Get(3)->GetObject<Ipv4>();
    Ipv4Address ip_n3 = ipv4_n3->GetAddress(1, 0).GetLocal();

    Ptr<CustomTrafficGenerator> trafficApp1 = CreateObject<CustomTrafficGenerator>();
    trafficApp1->SetAttribute("DestIp", Ipv4AddressValue(ip_n3));
    trafficApp1->SetAttribute("DestPort", UintegerValue(port));
    trafficApp1->SetAttribute("MaxPackets", UintegerValue(5));
    nodes.Get(0)->AddApplication(trafficApp1); // add source to n0
    trafficApp1->SetStartTime(Seconds(1.0));
    trafficApp1->SetStopTime(Seconds(2.0));

    Ptr<CustomTrafficGenerator> trafficApp2 = CreateObject<CustomTrafficGenerator>();
    trafficApp2->SetAttribute("DestIp", Ipv4AddressValue(ip_n3));
    trafficApp2->SetAttribute("DestPort", UintegerValue(port));
    trafficApp2->SetAttribute("MaxPackets", UintegerValue(10));
    nodes.Get(1)->AddApplication(trafficApp2); // add source to n1
    trafficApp2->SetStartTime(Seconds(1.0));
    trafficApp2->SetStopTime(Seconds(3.0));

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    NS_LOG_INFO("==== Simulation Summary ====");
    NS_LOG_INFO("Total received: " << sinkApp->GetTotalRxPackets() << " packets");
    NS_LOG_INFO("Total bytes received: " << sinkApp->GetTotalRx() << " bytes");
    NS_LOG_INFO("==== End Simulation ====");

    Simulator::Destroy();

    return 0;
}