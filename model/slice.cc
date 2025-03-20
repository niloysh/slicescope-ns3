#include "slice.h"

#include "custom-packet-sink.h"
#include "custom-traffic-generator.h"

#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/ipv4.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include <ns3/names.h>

#include <sys/types.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Slice");

const std::unordered_map<Slice::SliceType, uint8_t> Slice::sliceTypeToDscpMap = {{Slice::URLLC, 46},
                                                                                 {Slice::eMBB, 40},
                                                                                 {Slice::mMTC, 8}};

const std::unordered_map<uint8_t, Slice::SliceType> Slice::dscpToSliceTypeMap = {{46, Slice::URLLC},
                                                                                 {40, Slice::eMBB},
                                                                                 {8, Slice::mMTC}};

const std::unordered_map<Slice::SliceType, std::string> Slice::sliceTypeToStrMap = {
    {Slice::URLLC, "URLLC"},
    {Slice::eMBB, "eMBB"},
    {Slice::mMTC, "mMTC"}};

uint32_t Slice::_m_sliceId = 0;

TypeId
Slice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Slice")
            .SetParent<Object>()
            .SetGroupName("Applications")
            .AddConstructor<Slice>()
            .AddAttribute("SliceType",
                          "The type of slice (eMBB, URLLC, mMTC)",
                          EnumValue(eMBB),
                          MakeEnumAccessor<SliceType>(&Slice::m_sliceType),
                          MakeEnumChecker<SliceType>(eMBB, "eMBB", URLLC, "URLLC", mMTC, "mMTC"))
            .AddAttribute("SourceNode",
                          "The source node for the slice.",
                          PointerValue(),
                          MakePointerAccessor(&Slice::m_sourceNode),
                          MakePointerChecker<Node>())
            .AddAttribute("SinkNode",
                          "The sink node for the slice.",
                          PointerValue(),
                          MakePointerAccessor(&Slice::m_sinkNode),
                          MakePointerChecker<Node>())
            .AddAttribute("NumApps",
                          "Number of applications in this slice.",
                          UintegerValue(2),
                          MakeUintegerAccessor(&Slice::m_numApps),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("MaxPackets",
                          "Maximum number of packets to send per application. 0 means unlimited.",
                          UintegerValue(1),
                          MakeUintegerAccessor(&Slice::m_maxPackets),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("StartTime",
                          "The start time for the slice.",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&Slice::m_startTime),
                          MakeDoubleChecker<double>())
            .AddAttribute("StopTime",
                          "The stop time for the slice.",
                          DoubleValue(10.0),
                          MakeDoubleAccessor(&Slice::m_stopTime),
                          MakeDoubleChecker<double>());

    return tid;
}

Slice::Slice()
{
}

Slice::~Slice()
{
}

void
Slice::Configure()
{
    _m_sliceId++;
    m_sliceId = _m_sliceId;
    auto it = sliceTypeToDscpMap.find(m_sliceType);
    if (it != sliceTypeToDscpMap.end())
    {
        m_dscp = it->second;
    }
    else
    {
        NS_LOG_WARN("Slice type not found in DSCP map, defaulting dscp to 0");
        m_dscp = 0;
    }

    if (m_sliceType == eMBB)
    {
        m_packetSizeVar = CreateObject<UniformRandomVariable>();
        m_packetSizeVar->SetAttribute("Min", DoubleValue(100));
        m_packetSizeVar->SetAttribute("Max", DoubleValue(1500));

        m_dataRateVar = CreateObject<UniformRandomVariable>();
        m_dataRateVar->SetAttribute("Min", DoubleValue(10));
        m_dataRateVar->SetAttribute("Max", DoubleValue(100));
    }

    else if (m_sliceType == URLLC)
    {
        m_packetSizeVar = CreateObject<UniformRandomVariable>();
        m_packetSizeVar->SetAttribute("Min", DoubleValue(20));
        m_packetSizeVar->SetAttribute("Max", DoubleValue(250));

        m_dataRateVar = CreateObject<UniformRandomVariable>();
        m_dataRateVar->SetAttribute("Min", DoubleValue(1));
        m_dataRateVar->SetAttribute("Max", DoubleValue(10));
    }

    else
    {
        m_packetSizeVar = CreateObject<UniformRandomVariable>();
        m_packetSizeVar->SetAttribute("Min", DoubleValue(20));
        m_packetSizeVar->SetAttribute("Max", DoubleValue(100));

        m_dataRateVar = CreateObject<UniformRandomVariable>();
        m_dataRateVar->SetAttribute("Min", DoubleValue(0.1));
        m_dataRateVar->SetAttribute("Max", DoubleValue(1));
    }
}

void
Slice::InstallApps()
{
    Slice::Configure();

    const std::string& sliceTypeStr = sliceTypeToStrMap.at(m_sliceType);

    std::string sourceNodeName = Names::FindName(m_sourceNode);
    std::string sinkNodeName = Names::FindName(m_sinkNode);

    NS_LOG_INFO("[Slice] ID: " << m_sliceId << " | Type: " << sliceTypeStr << " | "
                               << sourceNodeName << " → " << sinkNodeName
                               << " | StartTime: " << m_startTime << " | StopTime: " << m_stopTime
                               << " | MaxPackets: " << m_maxPackets << " | NumApps: " << m_numApps);

    Ipv4Address destIp = m_sinkNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    uint16_t basePort = 5000 + (m_sliceId * 10);

    double sourceStopTime = std::max(0.0, m_stopTime - 1.0);

    for (uint32_t i = 0; i < m_numApps; ++i)
    {
        uint16_t port = basePort + i;

        double rateMbps =
            std::max(0.1,
                     std::min(m_dataRateVar->GetValue(), 100.0)); // Clamp between 0.1 and 100 Mbps

        // Create traffic generator (source)
        Ptr<CustomTrafficGenerator> trafficGenerator = CreateObject<CustomTrafficGenerator>();
        trafficGenerator->SetAttribute("DestIp", Ipv4AddressValue(destIp));
        trafficGenerator->SetAttribute("DestPort", UintegerValue(port));
        trafficGenerator->SetAttribute("DataRate", DoubleValue(rateMbps));
        trafficGenerator->SetAttribute("PacketSizeVar", PointerValue(m_packetSizeVar));
        trafficGenerator->SetAttribute("Dscp", UintegerValue(m_dscp));
        trafficGenerator->SetAttribute("MaxPackets", UintegerValue(m_maxPackets));
        trafficGenerator->SetStartTime(Seconds(m_startTime));
        trafficGenerator->SetStopTime(Seconds(sourceStopTime));

        ApplicationContainer sourceApp;
        sourceApp.Add(trafficGenerator);
        m_sourceNode->AddApplication(trafficGenerator);
        m_sourceApps.push_back(sourceApp);

        // Create packet sink (destination)
        Ptr<CustomPacketSink> packetSink = CreateObject<CustomPacketSink>();
        packetSink->SetAttribute("Port", UintegerValue(port));
        packetSink->SetStartTime(Seconds(m_startTime));
        packetSink->SetStopTime(Seconds(m_stopTime));

        ApplicationContainer sinkApp;
        sinkApp.Add(packetSink);
        m_sinkNode->AddApplication(packetSink);
        m_sinkApps.push_back(sinkApp);

        NS_LOG_DEBUG("[App] Slice " << m_sliceId << " | "
                                    << "App #" << i << " | Node " << m_sourceNode->GetId()
                                    << " → Node " << m_sinkNode->GetId() << " | Port: " << port
                                    << " | Rate: " << rateMbps << " Mbps"
                                    << " | MaxPackets: " << m_maxPackets);
    }
}

std::vector<ApplicationContainer>
Slice::GetSourceApps()
{
    return m_sourceApps;
}

std::vector<ApplicationContainer>
Slice::GetSinkApps()
{
    return m_sinkApps;
}

uint32_t
Slice::GetSliceId() const
{
    return m_sliceId;
}

Slice::SliceType
Slice::GetSliceType() const
{
    return m_sliceType;
}

} // namespace ns3