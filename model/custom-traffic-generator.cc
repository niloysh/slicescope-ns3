#include "custom-traffic-generator.h"

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/udp-socket-factory.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CustomTrafficGenerator");

NS_OBJECT_ENSURE_REGISTERED(CustomTrafficGenerator);

TypeId
CustomTrafficGenerator::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CustomTrafficGenerator")
                            .SetParent<Application>()
                            .SetGroupName("Applications")
                            .AddConstructor<CustomTrafficGenerator>();
    return tid;
}

CustomTrafficGenerator::CustomTrafficGenerator()
    : m_socket(nullptr),
      m_sendEvent(),
      m_maxPackets(0),
      m_packetsSent(0)
{
}

CustomTrafficGenerator::~CustomTrafficGenerator()
{
    if (m_socket)
    {
        m_socket->Close();
    }
}

void
CustomTrafficGenerator::Setup(Ipv4Address dest,
                              uint16_t port,
                              double dataRateMbps,
                              uint32_t minSize,
                              uint32_t maxSize,
                              uint32_t maxPackets)

{
    m_destIp = dest;
    m_destPort = port;
    m_minSize = minSize;
    m_maxSize = maxSize;
    m_maxPackets = maxPackets;
    m_packetsSent = 0;

    // Calculate average packet size in bits
    double averagePacketSizeBits = (minSize + maxSize) / 2.0 * 8.0;

    // Calculate lambda for Poisson process
    double lambda = dataRateMbps * 1e6 / averagePacketSizeBits;

    // Poisson process (exponential inter-arrival times)
    m_interPacketTime = CreateObject<ExponentialRandomVariable>();
    m_interPacketTime->SetAttribute("Mean", DoubleValue(1.0 / lambda));

    // Packet size variation
    m_packetSize = CreateObject<UniformRandomVariable>();
    m_packetSize->SetAttribute("Min", DoubleValue(minSize));
    m_packetSize->SetAttribute("Max", DoubleValue(maxSize));
}

void
CustomTrafficGenerator::StartApplication()
{
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        m_socket->Connect(InetSocketAddress(m_destIp, m_destPort));
    }

    SendPacket();
}

void
CustomTrafficGenerator::StopApplication()
{
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
        NS_LOG_INFO("Reached maxPackets limit (" << m_maxPackets << ")");
        StopApplication();
        return;
    }

    uint32_t packetSize = m_packetSize->GetInteger();
    Ptr<Packet> packet = Create<Packet>(packetSize);
    m_socket->Send(packet);
    m_packetsSent++;

    double nextTime = m_interPacketTime->GetValue();
    NS_LOG_INFO("Sent packet of size " << packetSize << " bytes, next in " << nextTime << "s");

    m_sendEvent = Simulator::Schedule(Seconds(nextTime), &CustomTrafficGenerator::SendPacket, this);
}

} // namespace ns3
