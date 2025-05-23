/**
 * @file example_1.cc
 * @brief Two LANs connected via routers with UDP echo traffic
 *
 * ### Topology
 * ```
 * LAN1 (n1 nodes) ----+                   +---- LAN2 (n2 nodes)
 *                     |                   |
 *                  Router1 ---------- Router2
 * ```
 *
 * - LANs use CSMA (100 Mbps, 6.56 µs delay)
 * - Routers connected via point-to-point (10 Mbps, 2 ms delay)
 * - Echo clients on LAN1, echo servers on LAN2
 *
 * ### Run
 *
 * ./ns3 run "example_1 --n1=4 --n2=4"
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

NS_LOG_COMPONENT_DEFINE("Example_1");

uint32_t total_client_tx = 0;
uint32_t total_client_rx = 0;
uint32_t total_server_tx = 0;
uint32_t total_server_rx = 0;

void
ClientTx(std::string context, Ptr<const Packet> packet)
{
    total_client_tx++;
}

void
ClientRx(std::string context, Ptr<const Packet> packet)
{
    total_client_rx++;
}

void
ServerRx(std::string context, Ptr<const Packet> packet)
{
    total_server_rx++;
}

int
main(int argc, char* argv[])
{
    CommandLine cmd;

    uint32_t n1 = 4;
    uint32_t n2 = 4;

    cmd.AddValue("n1", "Number of LAN 1 nodes", n1);
    cmd.AddValue("n2", "Number of LAN 2 nodes", n2);

    cmd.Parse(argc, argv);

    // For the first network
    NodeContainer lan1_nodes;

    // For the second network
    NodeContainer lan2_nodes;

    // for the nodes in the middle.
    NodeContainer router_nodes;

    lan1_nodes.Create(n1);
    lan2_nodes.Create(n2);
    router_nodes.Create(2);

    // Let's create LAN 1 by attaching a CsmaNetDevice to all the nodes on the LAN
    CsmaHelper csma1;
    csma1.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma1.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    // Router 1 is accessible on LAN 1, so we add it to the list nodes.
    lan1_nodes.Add(router_nodes.Get(0));
    // Actually attaching CsmaNetDevice to all LAN 1 nodes.
    NetDeviceContainer lan1Devices;
    lan1Devices = csma1.Install(lan1_nodes);

    // Doing the same for LAN 2
    CsmaHelper csma2;
    csma2.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma2.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    // Router 2 is on LAN 2, so we add it to the node container
    lan2_nodes.Add(router_nodes.Get(1));

    NetDeviceContainer lan2Devices;
    lan2Devices = csma2.Install(lan2_nodes);

    /* So far our two LANs are disjoint, r1 and r2 need to be connected */
    // A PointToPoint connection between the two routers
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer routerDevices;
    routerDevices = pointToPoint.Install(router_nodes);

    // Setting IP addresses. Notice that router 1 & 2 are in LAN 1 & 2 respectively.
    InternetStackHelper stack;
    stack.Install(lan1_nodes);
    stack.Install(lan2_nodes);

    Ipv4AddressHelper address;
    // For LAN 1
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer lan1interfaces;
    lan1interfaces = address.Assign(lan1Devices);
    // For LAN 2
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer lan2interfaces;
    lan2interfaces = address.Assign(lan2Devices);

    // For PointToPoint
    address.SetBase("10.1.100.0", "255.255.255.0");
    Ipv4InterfaceContainer routerInterfaces;
    routerInterfaces = address.Assign(routerDevices);

    // Let's install a UdpEchoServer on all nodes of LAN2
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(lan2_nodes);
    serverApps.Start(Seconds(0));
    serverApps.Stop(Seconds(10));

    // Let's create UdpEchoClients in all LAN1 nodes.
    UdpEchoClientHelper echoClient(lan2interfaces.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(200)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    // We'll install UdpEchoClient on two nodes in lan1 nodes
    NodeContainer clientNodes(lan1_nodes.Get(0), lan1_nodes.Get(1));
    ApplicationContainer clientApps = echoClient.Install(clientNodes);
    clientApps.Start(Seconds(1));
    clientApps.Stop(Seconds(10));

    // For routers to be able to forward packets, they need to have routing rules.
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    csma1.EnablePcap("lan1", lan1Devices);
    csma2.EnablePcap("lan2", lan2Devices);
    pointToPoint.EnablePcapAll("routers");

    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(serverApps.Get(0));

    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::UdpEchoClient/Tx",
                    MakeCallback(&ClientTx));
    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::UdpEchoClient/Rx",
                    MakeCallback(&ClientRx));
    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::UdpEchoServer/Rx",
                    MakeCallback(&ServerRx));

    // Run the simulation
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "Client Tx: " << total_client_tx << "\tClient Rx: " << total_client_rx
              << std::endl;
    std::cout << "Server Rx: " << total_server_rx << std::endl;

    return 0;
}