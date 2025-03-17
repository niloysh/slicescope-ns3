#include "custom-traffic-generator.h"

#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4.h"
#include "ns3/log.h"
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
            .AddAttribute("PacketSize",
                          "The size of the packets to send in bytes",
                          UintegerValue(1000),
                          MakeUintegerAccessor(&CustomTrafficGenerator::m_packetSize),
                          MakeUintegerChecker<uint32_t>())
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
    NS_LOG_INFO("Starting " << this->GetInstanceTypeId().GetName() << " on node "
                            << GetNode()->GetId() << "...");

    m_running = true;
    m_packetsSent = 0;

    double packetSizeBits = static_cast<double>(m_packetSize) * 8;
    double lambda = m_dataRate * 1e6 / packetSizeBits; // Packets per second

    // Poisson process (exponential inter-arrival times)
    m_interPacketTime = CreateObject<ExponentialRandomVariable>();
    m_interPacketTime->SetAttribute("Mean", DoubleValue(1.0 / lambda));

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

        NS_LOG_INFO(this->GetInstanceTypeId().GetName()
                    << " on node " << GetNode()->GetId() << " sending to " << m_destIp << ":"
                    << m_destPort);
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

    NS_LOG_INFO("Stopping " << this->GetInstanceTypeId().GetName() << " on node "
                            << GetNode()->GetId() << "...");
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
        NS_LOG_INFO("Reached maxPackets limit (" << m_maxPackets << "), stopping...");
        StopApplication();
        return;
    }

    if (!m_socket)
    {
        NS_LOG_WARN("Socket is null, unable to send packet.");
        return;
    }

    Ptr<Packet> packet = Create<Packet>(m_packetSize);

    // Set ToS (Traffic Class field)
    int tos = m_dscp << 2;
    m_socket->SetIpTos(tos);

    // Send packet
    int bytesSent = m_socket->Send(packet);
    if (bytesSent > 0)
    {
        m_packetsSent++;
        double nextTime = m_interPacketTime->GetValue();
        NS_LOG_INFO("Sent packet #" << m_packetsSent << " of size " << m_packetSize
                                    << " bytes, next in " << nextTime << "s");

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
    return m_packetsSent * m_packetSize;
}

} // namespace ns3
