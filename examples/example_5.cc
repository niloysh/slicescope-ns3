/**
 * @file example_5.cc
 * @brief UDP echo traffic between two terminals via a slicescope-enabled switch with Layer 3
 * support
 *
 * ### Topology
 * ```
 *   Terminal 1 ----+
 *                  |
 *             [Slicescope Switch]
 *                  |
 *   Terminal 2 ----+
 * ```
 *
 * - CSMA links (100 Mbps, 6.56 µs delay)
 * - Switch node uses SlicescopeSwitchNetDevice with Layer 3 forwarding
 * - Terminal 2 sends a single UDP echo request to Terminal 1
 * - Demonstrates custom switch integration and Rx/Tx packet tracing
 *
 * ### Run
 *
 * ./ns3 run "scratch/example_5"
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

NS_LOG_COMPONENT_DEFINE("TwoTerminalsOneSwitch");

void
ReceivePacket(Ptr<NetDevice> device, Ptr<const Packet> packet)
{
    std::cout << "Received packet on device " << device->GetIfIndex() << " with size "
              << packet->GetSize() << " bytes" << std::endl;
}

void
TransmitPacket(Ptr<NetDevice> device, Ptr<const Packet> packet)
{
    std::cout << "Transmitted packet on device " << device->GetIfIndex() << " with size "
              << packet->GetSize() << " bytes" << std::endl;
}

int
main(int argc, char* argv[])
{
    // Enable logging
    LogComponentEnable("TwoTerminalsOneSwitch", LOG_LEVEL_INFO);
    // LogComponentEnable("ArpL3Protocol", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable("SlicescopeSwitchNetDevice", LOG_LEVEL_INFO);
    // LogComponentEnable("SlicescopeSwitchHelper", LOG_LEVEL_LOGIC);

    // Create nodes
    NodeContainer terminals;
    terminals.Create(2);
    NodeContainer switchNode;
    switchNode.Create(1);

    // Create CSMA helper
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    // Install CSMA devices on the terminals and the switch
    NetDeviceContainer terminalDevices;
    NetDeviceContainer switchDevices;
    for (uint32_t i = 0; i < terminals.GetN(); ++i)
    {
        NetDeviceContainer link = csma.Install(NodeContainer(terminals.Get(i), switchNode));
        terminalDevices.Add(link.Get(0));
        switchDevices.Add(link.Get(1));
    }

    // Create a bridge net device and install it on the switch node
    // BridgeHelper bridge;
    // NetDeviceContainer bridgeDevices = bridge.Install(switchNode.Get(0), switchDevices);

    SlicescopeSwitchHelper slicescopeSwitch;
    NetDeviceContainer slicescopeSwitchDevices =
        slicescopeSwitch.Install(switchNode.Get(0), switchDevices);

    Ptr<NetDevice> device = slicescopeSwitchDevices.Get(0);
    Ptr<SlicescopeSwitchNetDevice> slicescopeDevice =
        device->GetObject<SlicescopeSwitchNetDevice>();

    if (slicescopeDevice)
    {
        NS_LOG_INFO("Enabling Layer 3 on the switch");
        device->SetAttribute("EnableLayer3", BooleanValue(true));
    }

    // Install Internet stack on the terminals
    InternetStackHelper stack;
    stack.Install(terminals);

    // Assign IP addresses to the terminals
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(terminalDevices);

    // Create a UDP echo server on terminal 1
    uint16_t port = 9; // Well-known echo port number
    UdpEchoServerHelper echoServer(port);
    ApplicationContainer serverApps = echoServer.Install(terminals.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    // Create a UDP echo client on terminal 2
    UdpEchoClientHelper echoClient(interfaces.GetAddress(0), port);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(terminals.Get(1));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    // Enable packet tracing
    for (uint32_t i = 0; i < switchDevices.GetN(); ++i)
    {
        Ptr<NetDevice> device = switchDevices.Get(i);
        device->TraceConnectWithoutContext("Rx", MakeCallback(&ReceivePacket));
        device->TraceConnectWithoutContext("Tx", MakeCallback(&TransmitPacket));
    }

    // Enable pcap tracing
    csma.EnablePcapAll("example_5");

    // Run the simulation
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}