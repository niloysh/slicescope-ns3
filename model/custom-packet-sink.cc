#include "custom-packet-sink.h"

#include "ns3/inet-socket-address.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"

#include <cstdint>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CustomPacketSink");

NS_OBJECT_ENSURE_REGISTERED(CustomPacketSink);

TypeId
CustomPacketSink::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CustomPacketSink")
                            .SetParent<Application>()
                            .SetGroupName("Applications")
                            .AddConstructor<CustomPacketSink>()
                            .AddAttribute("Port",
                                          "Listening port",
                                          UintegerValue(9),
                                          MakeUintegerAccessor(&CustomPacketSink::m_port),
                                          MakeUintegerChecker<uint16_t>());
    return tid;
}

CustomPacketSink::CustomPacketSink()
    : m_socket(nullptr),
      m_port(9),
      m_totalBytes(0),
      m_totalPackets(0)
{
}

CustomPacketSink::~CustomPacketSink()
{
    if (m_socket)
    {
        m_socket->Close();
    }
}

void
CustomPacketSink::StartApplication()
{
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        InetSocketAddress localAddress(Ipv4Address::GetAny(), m_port);
        m_socket->Bind(localAddress);
        std::string className = this->GetInstanceTypeId().GetName();
        NS_LOG_INFO(className << " on node " << GetNode()->GetId() << " listening on port "
                              << m_port);
        m_socket->SetRecvCallback(MakeCallback(&CustomPacketSink::HandleRead, this));
    }
}

void
CustomPacketSink::StopApplication()
{
    NS_LOG_INFO("Stopping " << this->GetInstanceTypeId().GetName() << " on node "
                            << GetNode()->GetId() << "...");
    if (m_socket)
    {
        m_socket->Close();
        m_socket = nullptr;
    }
}

void
CustomPacketSink::HandleRead(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (!packet)
        {
            NS_LOG_WARN("Received null packet, skipping...");
            continue;
        }

        m_totalPackets++;
        m_totalBytes += packet->GetSize();
        double receiveTime = Simulator::Now().GetSeconds(); // Store timestamp

        InetSocketAddress senderAddress = InetSocketAddress::ConvertFrom(from);
        Ipv4Address srcIp = senderAddress.GetIpv4();
        uint16_t srcPort = senderAddress.GetPort();

        std::pair<Ipv4Address, uint16_t> flowKey(srcIp, srcPort);
        m_flowStats[flowKey].totalBytes += packet->GetSize();
        m_flowStats[flowKey].totalPackets++;
        m_flowStats[flowKey].timestamps.push_back(receiveTime);

        Address localAddress;
        m_socket->GetSockName(localAddress); // Get the actual bound address

        InetSocketAddress receiverAddress = InetSocketAddress::ConvertFrom(localAddress);

        Ipv4Address destIp = receiverAddress.GetIpv4();
        uint16_t destPort = receiverAddress.GetPort();

        NS_LOG_INFO("Received packet from " << srcIp << ":" << srcPort << " to " << destIp << ":"
                                            << destPort << " size: " << packet->GetSize()
                                            << " at time " << receiveTime);
    }
}

void
CustomPacketSink::PrintStats() const
{
    uint32_t numFlows = m_flowStats.size();
    NS_LOG_INFO("=== Sink Statistics: NumFlows: " << numFlows << " TotalPackets: " << m_totalPackets
                                                  << " TotalBytes: " << m_totalBytes);

    for (const auto& entry : m_flowStats)
    {
        Ipv4Address srcIp = entry.first.first;
        uint16_t srcPort = entry.first.second;
        const FlowStats& stats = entry.second;

        NS_LOG_INFO("Flow: " << srcIp << ":" << srcPort << " Packets: " << stats.totalPackets
                             << " Bytes: " << stats.totalBytes);
    }
}

uint32_t
CustomPacketSink::GetTotalPackets() const
{
    return m_totalPackets;
}

uint32_t
CustomPacketSink::GetTotalBytes() const
{
    return m_totalBytes;
}

std::map<std::pair<Ipv4Address, uint16_t>, FlowStats>
CustomPacketSink::GetFlowStats() const
{
    return m_flowStats;
}

} // namespace ns3
