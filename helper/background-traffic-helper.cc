#include "background-traffic-helper.h"

#include "ns3/application-container.h"
#include "ns3/application-helper.h"
#include "ns3/applications-module.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/packet.h"
#include "ns3/string.h"

namespace ns3
{

void
BackgroundTrafficHelper::Install(TrafficType type,
                                 Ptr<Node> source,
                                 Ptr<Node> sink,
                                 Ipv4Address sinkAddr,
                                 double startTime,
                                 double stopTime,
                                 std::string dataRate = "100Mbps",
                                 uint32_t packetSize = 1024,
                                 uint32_t maxPackets = 1000,
                                 uint32_t maxBytes = 0)
{
    m_bytesSent = 0;
    m_bytesReceived = 0;
    m_packetsSent = 0;
    m_packetsReceived = 0;

    uint16_t port = 9000 + static_cast<uint16_t>(type);
    std::string protocol = (type == BULK) ? "ns3::TcpSocketFactory" : "ns3::UdpSocketFactory";

    // Install sink
    PacketSinkHelper sinkHelper(protocol, InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sinkHelper.Install(sink);
    sinkApps.Start(Seconds(startTime));
    sinkApps.Stop(Seconds(stopTime));

    m_sinkApp = DynamicCast<PacketSink>(sinkApps.Get(0));
    m_sinkApp->TraceConnectWithoutContext("Rx",
                                          MakeCallback(&BackgroundTrafficHelper::RxTrace, this));

    // Install source app
    if (type == UDP)
    {
        UdpClientHelper client(sinkAddr, port);
        client.SetAttribute("MaxPackets", UintegerValue(maxPackets));
        client.SetAttribute("Interval", TimeValue(MicroSeconds(100)));
        client.SetAttribute("PacketSize", UintegerValue(packetSize));

        ApplicationContainer apps = client.Install(source);
        apps.Start(Seconds(startTime));
        apps.Stop(Seconds(stopTime));
    }
    else if (type == ONOFF)
    {
        OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(sinkAddr, port));
        onoff.SetAttribute("DataRate", StringValue(dataRate));
        onoff.SetAttribute("PacketSize", UintegerValue(packetSize));
        onoff.SetAttribute("OnTime", StringValue("ns3::ExponentialRandomVariable[Mean=0.5]"));
        onoff.SetAttribute("OffTime", StringValue("ns3::ExponentialRandomVariable[Mean=0.5]"));

        ApplicationContainer apps = onoff.Install(source);
        apps.Start(Seconds(startTime));
        apps.Stop(Seconds(stopTime));
    }
    else if (type == BULK)
    {
        BulkSendHelper bulk("ns3::TcpSocketFactory", InetSocketAddress(sinkAddr, port));
        bulk.SetAttribute("SendSize", UintegerValue(packetSize));
        bulk.SetAttribute("MaxBytes", UintegerValue(maxBytes));

        ApplicationContainer apps = bulk.Install(source);
        apps.Start(Seconds(startTime));
        apps.Stop(Seconds(stopTime));
    }

    // Hook PhyTxEnd for sent bytes
    for (uint32_t i = 0; i < source->GetNDevices(); ++i)
    {
        Ptr<NetDevice> dev = source->GetDevice(i);
        if (dev)
        {
            dev->TraceConnectWithoutContext("PhyTxEnd",
                                            MakeCallback(&BackgroundTrafficHelper::TxTrace, this));
        }
    }
}

uint64_t
BackgroundTrafficHelper::GetTotalBytesSent() const
{
    return m_bytesSent;
}

uint64_t
BackgroundTrafficHelper::GetTotalBytesReceived() const
{
    if (m_sinkApp)
    {
        return m_sinkApp->GetTotalRx();
    }
    return m_bytesReceived;
}

uint32_t
BackgroundTrafficHelper::GetTotalPacketsSent() const
{
    return m_packetsSent;
}

uint32_t
BackgroundTrafficHelper::GetTotalPacketsReceived() const
{
    return m_packetsReceived;
}

void
BackgroundTrafficHelper::TxTrace(Ptr<const Packet> packet)
{
    m_bytesSent += packet->GetSize();
    m_packetsSent++;
}

void
BackgroundTrafficHelper::RxTrace(Ptr<const Packet> packet, const Address& addr)
{
    m_bytesReceived += packet->GetSize();
    m_packetsReceived++;
}

void
BackgroundTrafficHelper::InstallSaturatingTraffic(NodeContainer sources,
                                                  NodeContainer sinks,
                                                  double startTime,
                                                  double stopTime,
                                                  uint32_t packetSize,
                                                  uint16_t basePort)
{
    for (uint32_t i = 0; i < sources.GetN(); ++i)
    {
        Ptr<Node> source = sources.Get(i);
        Ptr<Node> sink = sinks.Get(i % sinks.GetN()); // round-robin
        uint16_t port = basePort + i;

        Ipv4Address sinkAddr = sink->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

        BulkSendHelper bulk("ns3::TcpSocketFactory", InetSocketAddress(sinkAddr, port));
        bulk.SetAttribute("SendSize", UintegerValue(packetSize));
        bulk.SetAttribute("MaxBytes", UintegerValue(0)); // unlimited

        ApplicationContainer apps = bulk.Install(source);
        apps.Start(Seconds(startTime));
        apps.Stop(Seconds(stopTime));

        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApps = sinkHelper.Install(sink);
        sinkApps.Start(Seconds(startTime));
        sinkApps.Stop(Seconds(stopTime));
    }
}

} // namespace ns3
