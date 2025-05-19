/**
 * @file example_3.cc
 * @brief Two UDP echo clients request responses from a common echo server in a mesh-like topology
 *
 * ### Topology
 * ```
 * (src1) n0 ---- n2 ---- n3 (dst)
 *        |       |
 * (src2) n1 ---- n4
 * ```
 *
 * - Point-to-point links (10 Mbps, 2 ms delay)
 * - n3 runs a UDP Echo Server (UdpEchoServerHelper)
 * - n0 and n1 each send a single Echo Request and receive Echo Replies
 * - Demonstrates request-response interaction and round-trip behavior
 *
 * ### Run
 *
 * ./ns3 run "scratch/example_3"
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

    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

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

    // Set up a UDP Echo server on n3 (destination)
    UdpEchoServerHelper echoServer(port);
    ApplicationContainer serverApp = echoServer.Install(nodes.Get(3)); // n3
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    // Get the IP address of n3 dynamically
    Ptr<Ipv4> ipv4_n3 = nodes.Get(3)->GetObject<Ipv4>();
    Ipv4Address ip_n3 = ipv4_n3->GetAddress(1, 0).GetLocal();

    // Set up a UDP Echo client on n0 (src1) and n1 (src2)
    UdpEchoClientHelper echoClient(ip_n3, 9);                // Target IP: n3's IP
    echoClient.SetAttribute("MaxPackets", UintegerValue(1)); // Send 1 packet
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024)); // 1 KB packets

    ApplicationContainer clientApp1 = echoClient.Install(nodes.Get(0)); // n0
    ApplicationContainer clientApp2 = echoClient.Install(nodes.Get(1)); // n1
    clientApp1.Start(Seconds(2.0));
    clientApp1.Stop(Seconds(10.0));
    clientApp2.Start(Seconds(3.0));
    clientApp2.Stop(Seconds(10.0));

    // Enable pcap on dst node
    p2p.EnablePcap("example_3", d23.Get(1), true);

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}