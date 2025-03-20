#include "custom-packet-sink.h"

#include "time-tag.h"

#include "ns3/boolean.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4.h"
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
                                          MakeUintegerChecker<uint16_t>())
                            .AddAttribute("ComputeDataRate",
                                          "Whether to compute the data rate",
                                          BooleanValue(false),
                                          MakeBooleanAccessor(&CustomPacketSink::m_computeDataRate),
                                          MakeBooleanChecker());
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

        Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4>();
        Ipv4Address serverIp = ipv4->GetAddress(1, 0).GetLocal();
        InetSocketAddress localAddress(serverIp, m_port);
        m_socket->Bind(localAddress);
        std::string className = this->GetInstanceTypeId().GetName();
        NS_LOG_INFO("[Node " << GetNode()->GetId() << "] Sink started → Listening on " << serverIp
                             << ":" << m_port);

        m_socket->SetRecvCallback(MakeCallback(&CustomPacketSink::HandleRead, this));

        if (m_computeDataRate)
        {
            m_dataRateEvent =
                Simulator::Schedule(Seconds(1.0), &CustomPacketSink::ComputeDataRate, this);
        }
    }
}

void
CustomPacketSink::StopApplication()
{
    NS_LOG_INFO("[Node " << GetNode()->GetId() << "] Sink stopped");

    if (m_socket)
    {
        m_socket->Close();
        m_socket = nullptr;
    }

    Simulator::Cancel(m_dataRateEvent);
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

        double receiveTime = Simulator::Now().GetSeconds();

        if (m_totalPackets == 0)
        {
            m_firstPacketTime = receiveTime;
        }
        m_lastPacketTime = receiveTime;

        m_totalPackets++;
        m_totalBytes += packet->GetSize();

        TimeTag tag;
        if (packet->PeekPacketTag(tag))
        {
            double sentTime = tag.GetTime().GetSeconds();
            double owd = receiveTime - sentTime;
            m_owd.push_back(owd);
            m_owdRecords.emplace_back(Simulator::Now(), owd);
        }

        InetSocketAddress senderAddress = InetSocketAddress::ConvertFrom(from);
        Ipv4Address srcIp = senderAddress.GetIpv4();
        uint16_t srcPort = senderAddress.GetPort();

        std::pair<Ipv4Address, uint16_t> flowKey(srcIp, srcPort);
        m_flowStats[flowKey].totalBytes += packet->GetSize();
        m_flowStats[flowKey].totalPackets++;

        Address localAddress;
        m_socket->GetSockName(localAddress); // Get the actual bound address

        InetSocketAddress receiverAddress = InetSocketAddress::ConvertFrom(localAddress);

        Ipv4Address destIp = receiverAddress.GetIpv4();
        uint16_t destPort = receiverAddress.GetPort();

        NS_LOG_DEBUG("[Rx] Node " << GetNode()->GetId() << " → Pkt #" << m_totalPackets << " | "
                                  << srcIp << ":" << srcPort << " → " << destIp << ":" << destPort
                                  << " | " << packet->GetSize() << "B"
                                  << " | Time: " << receiveTime << "s"
                                  << " | OWD: " << (m_owd.back() * 1000) << "ms");
    }
}

uint32_t
CustomPacketSink::GetTotalPacketsReceived() const
{
    return m_totalPackets;
}

uint32_t
CustomPacketSink::GetTotalBytesReceived() const
{
    return m_totalBytes;
}

std::map<std::pair<Ipv4Address, uint16_t>, FlowStats>
CustomPacketSink::GetFlowStats() const
{
    return m_flowStats;
}

void
CustomPacketSink::ComputeDataRate()
{
    if (m_totalPackets > 0)
    {
        double elapsedTime = m_lastPacketTime - m_firstPacketTime;
        if (elapsedTime > 0)
        {
            double dataRateMbps = (m_totalBytes * 8) / (elapsedTime * 1e6); // Convert to Mbps

            NS_LOG_INFO("[DataRate] Node " << GetNode()->GetId() << " | " << dataRateMbps << " Mbps"
                                           << " | Time: " << elapsedTime << "s"
                                           << " | Bytes: " << m_totalBytes);
        }
    }

    // Schedule next data rate computation
    m_dataRateEvent = Simulator::Schedule(Seconds(1.0), &CustomPacketSink::ComputeDataRate, this);
}

std::vector<double>
CustomPacketSink::GetOwd() const
{
    return m_owd;
}

std::vector<std::pair<Time, double>>
CustomPacketSink::GetOwdRecords() const
{
    return m_owdRecords;
}

} // namespace ns3
