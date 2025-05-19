/**
 * @file example_6.cc
 * @brief UDP client-server traffic across two slicescope-enabled switches in a chained topology
 *
 * ### Topology
 * ```
 * Terminal 0 --- Switch 0 --- Switch 1 --- Terminal 1
 * ```
 *
 * - CSMA links (100 Mbps, 6.56 µs delay)
 * - Two slicescope-enabled switches chained in series
 * - Terminal 0 sends a single UDP packet to Terminal 1
 * - Uses raw UdpClient and PacketSink applications
 *
 * ### Run
 *
 * ./ns3 run "scratch/example_6"
 *
 */

#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/slicescope-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TwoTerminalsTwoSwitches");

int
main(int argc, char* argv[])
{
    // Set up the logging
    LogComponentEnable("TwoTerminalsTwoSwitches", LOG_LEVEL_INFO);
    // LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    // LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable("SlicescopeSwitchNetDevice", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    LogComponentEnable("UdpClient", LOG_LEVEL_INFO);

    // Create nodes
    NodeContainer terminals;
    terminals.Create(2);

    NodeContainer bridges;
    bridges.Create(2);

    // Install Internet stack on terminals
    InternetStackHelper internet;
    internet.Install(terminals);

    // Create CSMA channel and set attributes
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    // Create NetDevice containers for the CSMA links
    NetDeviceContainer terminalDevices1;
    NetDeviceContainer terminalDevices2;
    NetDeviceContainer bridgeDevices1;
    NetDeviceContainer bridgeDevices2;

    // Connect terminals to bridges using CSMA links
    terminalDevices1 = csma.Install(NodeContainer(terminals.Get(0), bridges.Get(0)));
    bridgeDevices1 = csma.Install(NodeContainer(bridges.Get(0), bridges.Get(1)));
    terminalDevices2 = csma.Install(NodeContainer(bridges.Get(1), terminals.Get(1)));

    // Create bridge devices and install on bridge nodes
    // BridgeHelper bridge;
    SlicescopeSwitchHelper slicescope;
    NetDeviceContainer bridgeNetDevices1;
    NetDeviceContainer bridgeNetDevices2;

    bridgeNetDevices1.Add(terminalDevices1.Get(1));
    bridgeNetDevices1.Add(bridgeDevices1.Get(0));

    bridgeNetDevices2.Add(bridgeDevices1.Get(1));
    bridgeNetDevices2.Add(terminalDevices2.Get(0));

    // bridge.Install(bridges.Get(0), bridgeNetDevices1);
    // bridge.Install(bridges.Get(1), bridgeNetDevices2);

    slicescope.Install(bridges.Get(0), bridgeNetDevices1);
    slicescope.Install(bridges.Get(1), bridgeNetDevices2);

    // Assign IP addresses to terminal devices
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer terminalInterfaces;
    terminalInterfaces.Add(address.Assign(terminalDevices1.Get(0)));
    terminalInterfaces.Add(address.Assign(terminalDevices2.Get(1)));

    // Create an application to send data from terminal 0 to terminal 1
    uint16_t port = 9;
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApps = packetSinkHelper.Install(terminals.Get(1));
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(10.0));

    // Create a UDP client application on node 0 (h1)
    UdpClientHelper client(terminalInterfaces.GetAddress(1), port); // h2's IP address
    client.SetAttribute("MaxPackets", UintegerValue(1));
    client.SetAttribute("Interval", TimeValue(MilliSeconds(1.0))); // 500 Mbps => 1 ms interval
    client.SetAttribute("PacketSize", UintegerValue(1024));        // Packet size

    ApplicationContainer clientApps = client.Install(terminals.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    // Enable pcap tracing
    csma.EnablePcapAll("example6");

    // Run the simulation
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}