#include "ns3/application-helper.h"
#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/slicescope-module.h"
#include "ns3/traffic-control-module.h"

#include <cstdint>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Example_13");

int
main(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    LogComponentEnable("Example_13", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable("TopologyHelper", LOG_LEVEL_INFO);

    AdvancedTopologyHelper topo;
    PointToPointHelper p2pHosts;
    p2pHosts.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2pHosts.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper p2pSwitches;
    p2pSwitches.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
    p2pSwitches.SetChannelAttribute("Delay", StringValue("2ms"));

    topo.SetHostChannelHelper(p2pHosts);
    topo.SetSwitchChannelHelper(p2pSwitches);

    /*
        s0 -- s1 -- s2 -- s3
        |     |      |     |
        h0    h1    h2    h3
    */
    // topo.CreateLinearTopology(4);

    /* Create a tree topology
    Depth 0:           s0
                     /    \
    Depth 1:     s1           s2
                /  \        /     \
    Depth 2:    s3   s4    s5     s6   <- Leaf switches
               / \   / \   / \   / \
    Hosts:   h0 h1 h2 h3 h4 h5  h6 h7
    */
    topo.CreateTreeTopology(3, 2);

    NodeContainer hosts = topo.GetHosts();
    NS_LOG_INFO("Hosts: " << hosts.GetN());

    NodeContainer switches = topo.GetSwitches();
    NS_LOG_INFO("Switches: " << switches.GetN());

    for (uint32_t i = 0; i < hosts.GetN(); ++i)
    {
        Ptr<Node> host = hosts.Get(i);
        Ptr<Ipv4> ipv4 = host->GetObject<Ipv4>();
        Ipv4InterfaceAddress addr = ipv4->GetAddress(1, 0);
        NS_LOG_INFO("Host " << i << " IP address: " << addr.GetLocal());
    }

    for (uint32_t i = 0; i < switches.GetN(); ++i)
    {
        Ptr<Node> node = switches.Get(i);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        NS_LOG_INFO("Switch " << i << " number of interfaces: " << ipv4->GetNInterfaces());

        // for each interface print address
        for (uint32_t j = 0; j < ipv4->GetNInterfaces(); ++j)
        {
            Ipv4InterfaceAddress addr = ipv4->GetAddress(j, 0);
            NS_LOG_DEBUG("Switch " << i << " interface " << j << " address: " << addr.GetLocal());
        }
    }

    // Install UDPEchoServer application on h0
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(hosts.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    Ptr<Ipv4> ipv4_server = hosts.Get(0)->GetObject<Ipv4>(); // server on h0
    Ipv4Address ipv4_addr_server = ipv4_server->GetAddress(1, 0).GetLocal();
    NS_LOG_INFO("Server IP address: " << ipv4_addr_server);

    // Install UDPEchoClient application on h2
    UdpEchoClientHelper echoClient(ipv4_addr_server, 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(hosts.Get(7)); // client on h3
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(9.0));

    p2pHosts.EnablePcapAll("example_13");

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
