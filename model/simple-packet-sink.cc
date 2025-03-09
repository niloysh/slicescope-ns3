#include "simple-packet-sink.h"

#include "ns3/inet-socket-address.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"

#include <cstdint>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SimplePacketSink");

NS_OBJECT_ENSURE_REGISTERED(SimplePacketSink);

TypeId
SimplePacketSink::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::SimplePacketSink")
                            .SetParent<Application>()
                            .SetGroupName("Applications")
                            .AddConstructor<SimplePacketSink>();
    return tid;
}

SimplePacketSink::SimplePacketSink()
    : m_socket(nullptr),
      m_totalBytes(0),
      m_totalPackets(0)
{
}

SimplePacketSink::~SimplePacketSink()
{
    if (m_socket)
    {
        m_socket->Close();
    }
}

void
SimplePacketSink::Setup(uint16_t port)
{
    m_localAddress = InetSocketAddress(Ipv4Address::GetAny(), port);
}

void
SimplePacketSink::StartApplication()
{
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        m_socket->Bind(m_localAddress);
        m_socket->SetRecvCallback(MakeCallback(&SimplePacketSink::HandleRead, this));
    }
}

void
SimplePacketSink::StopApplication()
{
    if (m_socket)
    {
        m_socket->Close();
        m_socket = nullptr;
    }
}

void
SimplePacketSink::HandleRead(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        m_totalPackets++;
        m_totalBytes += packet->GetSize();

        InetSocketAddress senderAddress = InetSocketAddress::ConvertFrom(from);
        Ipv4Address srcIp = senderAddress.GetIpv4();
        uint16_t srcPort = senderAddress.GetPort();

        std::pair<Ipv4Address, uint16_t> flowKey(srcIp, srcPort);
        m_flowStats[flowKey].totalBytes += packet->GetSize();
        m_flowStats[flowKey].totalPackets++;

        NS_LOG_DEBUG("Received packet from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
                                             << " Size: " << packet->GetSize() << " Total Packets: "
                                             << m_totalPackets << " Total Bytes: " << m_totalBytes);
    }
}

void
SimplePacketSink::PrintStats() const
{
    NS_LOG_INFO("=== Packet Sink Statistics ===");
    NS_LOG_INFO("Total Packets Received: " << m_totalPackets
                                           << " Total Bytes Received: " << m_totalBytes);

    for (const auto& entry : m_flowStats)
    {
        Ipv4Address srcIp = entry.first.first;
        uint16_t srcPort = entry.first.second;
        const FlowStats& stats = entry.second;

        NS_LOG_INFO("Flow: " << srcIp << ":" << srcPort << " Packets: " << stats.totalPackets
                             << " Bytes: " << stats.totalBytes);
    }
}

} // namespace ns3
