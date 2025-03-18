#include "slice-helper.h"

#include "ns3/custom-packet-sink.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SliceHelper");

TypeId
SliceHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SliceHelper")
                            .SetParent<Object>()
                            .SetGroupName("Helper")
                            .AddConstructor<SliceHelper>()
                            .AddAttribute("SimulationDuration",
                                          "Total simulation time in seconds.",
                                          DoubleValue(10.0),
                                          MakeDoubleAccessor(&SliceHelper::m_simulationDuration),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("MaxPackets",
                                          "Maximum number of packets per slice application.",
                                          UintegerValue(100),
                                          MakeUintegerAccessor(&SliceHelper::m_maxPackets),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("NumApps",
                                          "Number of applications per slice.",
                                          UintegerValue(1),
                                          MakeUintegerAccessor(&SliceHelper::m_numApps),
                                          MakeUintegerChecker<uint32_t>());
    return tid;
}

SliceHelper::SliceHelper()
    : m_simulationDuration(10.0),
      m_maxPackets(2),
      m_numApps(1)
{
}

void
SliceHelper::SetAttribute(std::string name, const AttributeValue& value)
{
    Object::SetAttribute(name, value);
}

std::vector<Ptr<Slice>>
SliceHelper::CreateSlices(NodeContainer sources, NodeContainer sinks, uint32_t numSlices)
{
    m_slices.clear();
    m_slices.reserve(numSlices);
    NS_LOG_INFO("[SliceHelper] Creating " << numSlices << " slices...");

    for (uint32_t i = 0; i < numSlices; ++i)
    {
        // Randomly pick source and sink
        uint32_t srcIdx = rand() % sources.GetN();
        uint32_t sinkIdx = rand() % sinks.GetN();
        Ptr<Node> sourceNode = sources.Get(srcIdx);
        Ptr<Node> sinkNode = sinks.Get(sinkIdx);

        // Ensure source and sink are not the same
        while (sourceNode == sinkNode && sources.GetN() > 1)
        {
            sinkIdx = rand() % sinks.GetN();
            sinkNode = sinks.Get(sinkIdx);
        }

        double startTime = (double)rand() / RAND_MAX * (m_simulationDuration / 2.0);
        double stopTime = m_simulationDuration;

        auto sliceType = static_cast<Slice::SliceType>(rand() % 3);

        Ptr<Slice> slice = CreateObject<Slice>();
        slice->SetAttribute("SliceType", EnumValue(sliceType));
        slice->SetAttribute("SourceNode", PointerValue(sourceNode));
        slice->SetAttribute("SinkNode", PointerValue(sinkNode));
        slice->SetAttribute("StartTime", DoubleValue(startTime));
        slice->SetAttribute("StopTime", DoubleValue(stopTime));
        slice->SetAttribute("MaxPackets", UintegerValue(m_maxPackets));
        slice->SetAttribute("NumApps", UintegerValue(m_numApps));
        slice->InstallApps();

        m_slices.push_back(slice);

        NS_LOG_INFO(
            "[SliceHelper] Created Slice #"
            << i << " | Type: "
            << (sliceType == Slice::eMBB ? "eMBB" : (sliceType == Slice::URLLC ? "URLLC" : "mMTC"))
            << " | Node " << sourceNode->GetId() << " → Node " << sinkNode->GetId()
            << " | Start: " << startTime << "s"
            << " | Stop: " << stopTime << "s"
            << " | MaxPackets: " << m_maxPackets << " | NumApps: " << m_numApps);
    }

    return m_slices;
}

std::vector<Ptr<Slice>>
SliceHelper::GetSlices() const
{
    return m_slices;
}

void
SliceHelper::ReportSliceStats()
{
    NS_LOG_INFO("====== Slice Statistics ======");

    for (auto& slice : m_slices)
    {
        uint32_t totalPackets = 0;
        double minRtt = std::numeric_limits<double>::max();
        double maxRtt = 0;
        double sumRtt = 0;
        uint32_t countRtt = 0;

        auto sinks = slice->GetSinkApps();
        for (auto& sinkApp : sinks)
        {
            Ptr<CustomPacketSink> sink = DynamicCast<CustomPacketSink>(sinkApp.Get(0));
            if (!sink)
            {
                continue;
            }

            totalPackets += sink->GetTotalPacketsReceived();
            auto rttValues = sink->GetRtt();

            for (double rtt : rttValues)
            {
                if (rtt < minRtt)
                {
                    minRtt = rtt;
                }
                if (rtt > maxRtt)
                {
                    maxRtt = rtt;
                }
                sumRtt += rtt;
                countRtt++;
            }
        }

        double avgRtt = (countRtt > 0) ? sumRtt / countRtt : 0.0;
        if (minRtt == std::numeric_limits<double>::max())
        {
            minRtt = 0.0;
        }

        NS_LOG_INFO("[Slice " << slice->GetSliceId() << "]"
                              << " | Type: "
                              << (slice->GetSliceType() == Slice::eMBB
                                      ? "eMBB"
                                      : (slice->GetSliceType() == Slice::URLLC ? "URLLC" : "mMTC"))
                              << " |"
                              << " Packets: " << totalPackets << " | Min RTT: " << (minRtt * 1000)
                              << " ms"
                              << " | Max RTT: " << (maxRtt * 1000) << " ms"
                              << " | Avg RTT: " << (avgRtt * 1000) << " ms");
    }
}

} // namespace ns3
