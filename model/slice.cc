#include "slice.h"

#include "custom-packet-sink.h"
#include "custom-traffic-generator.h"

#include "ns3/double.h"
#include "ns3/ipv4.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Slice");

const std::unordered_map<Slice::SliceType, uint8_t> Slice::dscpMap = {{Slice::eMBB, 40},
                                                                      {Slice::URLLC, 46},
                                                                      {Slice::mMTC, 8}};

Slice::Slice(SliceType sliceType,
             uint32_t sliceId,
             Ptr<Node> sourceNode,
             Ptr<Node> sinkNode,
             const SliceParams& params)
    : m_sliceId(sliceId),
      m_sliceType(sliceType),
      m_sourceNode(sourceNode),
      m_sinkNode(sinkNode),
      m_params(params)
{
    NS_LOG_INFO("Creating slice " << m_sliceId << " of type " << m_sliceType);
    auto it = dscpMap.find(sliceType);
    if (it != dscpMap.end())
    {
        m_dscp = it->second;
    }
    else
    {
        NS_LOG_WARN("Slice type not found in DSCP map, defaulting dscp to 0");
        m_dscp = 0;
    }
}

Slice::~Slice()
{
}

void
Slice::InstallApps()
{
    NS_LOG_INFO("Installing apps for slice " << m_sliceId);

    Ptr<UniformRandomVariable> rateVar = CreateObject<UniformRandomVariable>();
    rateVar->SetAttribute("Min", DoubleValue(m_params.minRateMbps));
    rateVar->SetAttribute("Max", DoubleValue(m_params.maxRateMbps));

    Ptr<UniformRandomVariable> sizeVar = CreateObject<UniformRandomVariable>();
    sizeVar->SetAttribute("Min", UintegerValue(m_params.minPacketSize));
    sizeVar->SetAttribute("Max", UintegerValue(m_params.maxPacketSize));

    Ptr<UniformRandomVariable> numAppsVar = CreateObject<UniformRandomVariable>();
    numAppsVar->SetAttribute("Min", UintegerValue(m_params.minApps));
    numAppsVar->SetAttribute("Max", UintegerValue(m_params.maxApps));

    uint32_t numApps = numAppsVar->GetInteger();
    NS_LOG_INFO("Slice " << m_sliceId << " has " << numApps << " applications");

    Ipv4Address destIp = m_sinkNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    uint16_t basePort = 5000 + (m_sliceId * 10);

    for (uint32_t i = 0; i < numApps; ++i)
    {
        uint16_t port = basePort + i;

        double rateMbps = rateVar->GetValue();
        uint32_t packetSize = sizeVar->GetInteger();

        NS_LOG_INFO("App " << i << " - DataRate: " << rateMbps
                           << " Mbps, PacketSize: " << packetSize << " bytes, Port: " << port);

        // Create traffic generator (source)
        Ptr<CustomTrafficGenerator> trafficGenerator = CreateObject<CustomTrafficGenerator>();
        trafficGenerator->SetAttribute("DestIp", Ipv4AddressValue(destIp));
        trafficGenerator->SetAttribute("DestPort", UintegerValue(port));
        trafficGenerator->SetAttribute("DataRate", DoubleValue(rateMbps));
        trafficGenerator->SetAttribute("PacketSize", UintegerValue(packetSize));
        trafficGenerator->SetAttribute("Dscp", UintegerValue(m_dscp));

        ApplicationContainer sourceApp;
        sourceApp.Add(trafficGenerator);
        m_sourceNode->AddApplication(trafficGenerator);
        m_sourceApps.push_back(sourceApp);

        // Create packet sink (destination)
        Ptr<CustomPacketSink> packetSink = CreateObject<CustomPacketSink>();
        packetSink->SetAttribute("Port", UintegerValue(port));

        ApplicationContainer sinkApp;
        sinkApp.Add(packetSink);
        m_sinkNode->AddApplication(packetSink);
        m_sinkApps.push_back(sinkApp);

        NS_LOG_INFO("Installed app " << i << " from Node " << m_sourceNode->GetId() << " to Node "
                                     << m_sinkNode->GetId());
    }
}

} // namespace ns3