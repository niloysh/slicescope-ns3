/**
 * @file example_2.cc
 * @brief Two continuous UDP traffic sources send to a common sink in a mesh-like topology
 *
 * ### Topology
 * ```
 * (src1) n0 ---- n2 ---- n3 (dst)
 *        |       |
 * (src2) n1 ---- n4
 * ```
 *
 * - Point-to-point links (10 Mbps, 2 ms delay)
 * - n0 and n1 generate UDP traffic using OnOff applications
 * - n3 runs a UDP sink (PacketSinkHelper) that receives the traffic
 * - Demonstrates unidirectional UDP flow with configurable rate and packet size
 *
 * ### Run
 *
 * ./ns3 run "scratch/example_2"
 *
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Example_2");

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

    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);

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

    uint16_t port = 9; // Port number

    // Create a UDP sink (receiver) on n3
    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sink.Install(nodes.Get(3));
    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(10.0));

    // Configure UDP traffic from n0 to n3
    OnOffHelper udp1("ns3::UdpSocketFactory", Address(InetSocketAddress(i23.GetAddress(1), port)));
    udp1.SetConstantRate(DataRate("500kbps"));
    udp1.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer app1 = udp1.Install(nodes.Get(0));
    app1.Start(Seconds(2.0));
    app1.Stop(Seconds(9.0));

    // Configure UDP traffic from n1 to n3
    OnOffHelper udp2("ns3::UdpSocketFactory", Address(InetSocketAddress(i23.GetAddress(1), port)));
    udp2.SetConstantRate(DataRate("500kbps"));
    udp2.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer app2 = udp2.Install(nodes.Get(1));
    app2.Start(Seconds(3.0));
    app2.Stop(Seconds(9.0));

    // Enable pcap on dst node
    p2p.EnablePcap("example_2", d23.Get(1), true);

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}