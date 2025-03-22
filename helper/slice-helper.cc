#include "slice-helper.h"

#include "ns3/custom-packet-sink.h"
#include "ns3/custom-traffic-generator.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"

#include <sys/types.h>

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

    Ptr<UniformRandomVariable> randVarSourceSink = CreateObject<UniformRandomVariable>();
    randVarSourceSink->SetStream(8);

    Ptr<UniformRandomVariable> randVarStartTime = CreateObject<UniformRandomVariable>();
    randVarStartTime->SetStream(9);

    Ptr<UniformRandomVariable> randVarSliceType = CreateObject<UniformRandomVariable>();
    randVarSliceType->SetStream(10);

    for (uint32_t i = 0; i < numSlices; ++i)
    {
        // Randomly pick source and sink

        uint32_t srcIdx = randVarSourceSink->GetInteger(0, sources.GetN() - 1);
        uint32_t sinkIdx = randVarSourceSink->GetInteger(0, sinks.GetN() - 1);

        Ptr<Node> sourceNode = sources.Get(srcIdx);
        Ptr<Node> sinkNode = sinks.Get(sinkIdx);

        // Ensure source and sink are not the same
        while (sourceNode == sinkNode && sources.GetN() > 1)
        {
            sinkIdx = randVarSourceSink->GetInteger(0, sinks.GetN() - 1);
            sinkNode = sinks.Get(sinkIdx);
        }

        double startTime = randVarStartTime->GetValue(0.0, m_simulationDuration / 2.0);
        double stopTime = m_simulationDuration;

        auto sliceType = static_cast<Slice::SliceType>(randVarSliceType->GetInteger(0, 2));

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
        uint32_t totalRxPackets = 0;
        uint32_t totalTxPackets = 0;
        double minOwd = std::numeric_limits<double>::max();
        double maxOwd = 0;
        double sumOwd = 0;
        uint32_t countOwd = 0;

        auto sinks = slice->GetSinkApps();
        auto sources = slice->GetSourceApps();
        for (size_t i = 0; i < sources.size(); ++i)
        {
            Ptr<CustomPacketSink> sink = DynamicCast<CustomPacketSink>(sinks[i].Get(0));
            Ptr<CustomTrafficGenerator> source =
                DynamicCast<CustomTrafficGenerator>(sources[i].Get(0));
            if (!sink)
            {
                continue;
            }

            totalRxPackets += sink->GetTotalRxPackets();
            totalTxPackets += source->GetTotalPacketsSent();
            auto owdValues = sink->GetOwd();

            for (double owd : owdValues)
            {
                if (owd < minOwd)
                {
                    minOwd = owd;
                }
                if (owd > maxOwd)
                {
                    maxOwd = owd;
                }
                sumOwd += owd;
                countOwd++;
            }
        }

        double avgRtt = (countOwd > 0) ? sumOwd / countOwd : 0.0;
        if (minOwd == std::numeric_limits<double>::max())
        {
            minOwd = 0.0;
        }

        NS_LOG_INFO("[Slice " << slice->GetSliceId() << "]"
                              << " | Type: " << Slice::sliceTypeToStrMap.at(slice->GetSliceType())
                              << " |"
                              << " Rx Packets: " << totalRxPackets
                              << " | Dropped: " << (totalTxPackets - totalRxPackets)
                              << " | Min OWD: " << (minOwd * 1000) << " ms"
                              << " | Max OWD: " << (maxOwd * 1000) << " ms"
                              << " | Avg OWD: " << (avgRtt * 1000) << " ms");
    }
}

void
SliceHelper::ExportOwdRecords(std::string filename)
{
    NS_LOG_INFO("Exporting OWD records to " << filename);

    std::vector<std::tuple<Time, double, uint32_t, std::string>> sliceOwdRecords;
    for (auto slice : m_slices)
    {
        uint32_t sliceId = slice->GetSliceId();
        std::string sliceTypeStr = Slice::sliceTypeToStrMap.at(slice->GetSliceType());
        auto sinks = slice->GetSinkApps();
        for (const auto& sinkApp : sinks)
        {
            Ptr<CustomPacketSink> sink = DynamicCast<CustomPacketSink>(sinkApp.Get(0));
            if (!sink)
            {
                continue;
            }

            auto owdRecords = sink->GetOwdRecords();
            for (const auto& record : owdRecords)
            {
                sliceOwdRecords.emplace_back(record.first, record.second, sliceId, sliceTypeStr);
            }
        }
    }

    // Sort by arrival time (first element in the tuple)
    std::sort(sliceOwdRecords.begin(),
              sliceOwdRecords.end(),
              [](const std::tuple<Time, double, uint32_t, std::string>& a,
                 const std::tuple<Time, double, uint32_t, std::string>& b) {
                  return std::get<0>(a) < std::get<0>(b); // Sort by time
              });

    std::ofstream outFile(filename, std::ios::out);
    outFile << "PacketArrivalTime(s),OWD(ms),SliceId,SliceType\n"; // CSV header

    for (const auto& record : sliceOwdRecords)
    {
        double arrivalTimeSec = std::get<0>(record).GetSeconds();
        double owdMs = std::get<1>(record) * 1000; // Convert to ms
        uint32_t sliceId = std::get<2>(record);
        std::string sliceType = std::get<3>(record);

        outFile << arrivalTimeSec << "," << owdMs << "," << sliceId << "," << sliceType << "\n";
    }

    outFile.close();
    NS_LOG_INFO("Successfully exported OWD records to " << filename);
}

} // namespace ns3
