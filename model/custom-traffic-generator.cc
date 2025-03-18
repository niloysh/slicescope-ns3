#include "custom-traffic-generator.h"

#include "time-tag.h"

#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"

#include <cstdint>
#include <sys/types.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CustomTrafficGenerator");

NS_OBJECT_ENSURE_REGISTERED(CustomTrafficGenerator);

TypeId
CustomTrafficGenerator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CustomTrafficGenerator")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<CustomTrafficGenerator>()
            .AddAttribute("DestIp",
                          "The destination IP address",
                          Ipv4AddressValue(),
                          MakeIpv4AddressAccessor(&CustomTrafficGenerator::m_destIp),
                          MakeIpv4AddressChecker())
            .AddAttribute("DestPort",
                          "The destination port",
                          UintegerValue(1234),
                          MakeUintegerAccessor(&CustomTrafficGenerator::m_destPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("MaxPackets",
                          "The maximum number of packets to send (0 = unlimited)",
                          UintegerValue(0),
                          MakeUintegerAccessor(&CustomTrafficGenerator::m_maxPackets),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("DataRate",
                          "The data rate in Mbps",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&CustomTrafficGenerator::m_dataRate),
                          MakeDoubleChecker<double>())
            .AddAttribute(
                "PacketSizeVar",
                "Random variable defining packet size distribution",
                PointerValue(CreateObject<ConstantRandomVariable>()), // Default to constant size
                MakePointerAccessor(&CustomTrafficGenerator::m_packetSizeVar),
                MakePointerChecker<RandomVariableStream>())
            .AddAttribute("Dscp",
                          "The DSCP value to set in the IP header",
                          UintegerValue(0),
                          MakeUintegerAccessor(&CustomTrafficGenerator::m_dscp),
                          MakeUintegerChecker<uint8_t>());
    return tid;
}

CustomTrafficGenerator::CustomTrafficGenerator()
    : m_socket(nullptr),
      m_packetsSent(0) // Ensure correct initialization
{
    NS_LOG_INFO("CustomTrafficGenerator created");
}

CustomTrafficGenerator::~CustomTrafficGenerator()
{
    if (m_socket)
    {
        m_socket->Close();
    }
}

void
CustomTrafficGenerator::StartApplication()
{
    NS_LOG_INFO("[Node " << GetNode()->GetId() << "] Generator started → Dest: " << m_destIp << ":"
                         << m_destPort);

    m_running = true;
    m_packetsSent = 0;

    m_jitterVar = CreateObject<NormalRandomVariable>();  // Jitter around the mean
    m_jitterVar->SetAttribute("Mean", DoubleValue(0.0)); // Centered around 0
    m_jitterVar->SetAttribute("Variance", DoubleValue(0.0));

    PrecomputeInterarrivalTimes();

    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        if (m_socket->Bind() == -1)
        {
            NS_LOG_ERROR("Failed to bind socket.");
            return;
        }
        if (m_socket->Connect(InetSocketAddress(m_destIp, m_destPort)) == -1)
        {
            NS_LOG_ERROR("Failed to connect socket to " << m_destIp << ":" << m_destPort);
            return;
        }
    }

    // Schedule first packet
    Simulator::ScheduleNow(&CustomTrafficGenerator::SendPacket, this);
}

void
CustomTrafficGenerator::StopApplication()
{
    if (!m_running)
    {
        return;
    }

    NS_LOG_INFO("[Node " << GetNode()->GetId() << "] Generator stopped → Sent: " << m_packetsSent
                         << " pkts");
    m_running = false;

    if (m_socket)
    {
        m_socket->Close();
        m_socket = nullptr;
    }
    Simulator::Cancel(m_sendEvent);
}

void
CustomTrafficGenerator::SendPacket()
{
    if (m_maxPackets > 0 && m_packetsSent >= m_maxPackets)
    {
        StopApplication();
        return;
    }

    if (!m_socket)
    {
        NS_LOG_WARN("Socket is null, unable to send packet.");
        return;
    }

    auto packetSize = static_cast<uint32_t>(m_packetSizeVar->GetValue());
    packetSize = std::max(packetSize, 20U);   // Ensure minimum size of 20 bytes
    packetSize = std::min(packetSize, 1500U); // Ensure maximum size of 1500 bytes

    Ptr<Packet> packet = Create<Packet>(packetSize);

    // Add timestamp to packet
    TimeTag timestamp;
    timestamp.SetTime(Simulator::Now());
    packet->AddPacketTag(timestamp);

    // Set ToS (Traffic Class field)
    int tos = m_dscp << 2;
    m_socket->SetIpTos(tos);

    // Send packet
    int bytesSent = m_socket->Send(packet);
    if (bytesSent > 0)
    {
        m_packetsSent++;

        double nextTime = m_precomputedInterarrival.front();
        m_precomputedInterarrival.pop();
        if (m_precomputedInterarrival.empty())
        {
            PrecomputeInterarrivalTimes(); // Refill queue when empty
        }

        NS_LOG_DEBUG("[Tx] Node " << GetNode()->GetId() << " → Pkt #" << m_packetsSent
                                  << " | Size: " << packetSize << "B"
                                  << " | Next: " << (nextTime * 1000) << "ms");

        // Schedule next packet
        m_sendEvent =
            Simulator::Schedule(Seconds(nextTime), &CustomTrafficGenerator::SendPacket, this);
    }
    else
    {
        NS_LOG_WARN("Packet sending failed.");
    }
}

uint32_t
CustomTrafficGenerator::GetTotalPacketsSent() const
{
    return m_packetsSent;
}

uint32_t
CustomTrafficGenerator::GetTotalBytesSent() const
{
    return m_bytesSent;
}

void
CustomTrafficGenerator::PrecomputeInterarrivalTimes()
{
    m_precomputedInterarrival = std::queue<double>();
    for (int i = 0; i < 100; i++)
    {
        double packetSizeBits = static_cast<double>(m_packetSizeVar->GetValue()) * 8;
        double meanInterarrivalTime = packetSizeBits / (m_dataRate * 1e6);
        double jitter = m_jitterVar->GetValue();
        double interarrivalTime = std::max(meanInterarrivalTime + jitter, 0.0);
        m_precomputedInterarrival.push(interarrivalTime);
    }
}

} // namespace ns3
