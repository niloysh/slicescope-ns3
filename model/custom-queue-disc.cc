#include "custom-queue-disc.h"

#include "metadata-tag.h"
#include "slice.h"
#include "time-tag.h"

#include "ns3/drop-tail-queue.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <ns3/names.h>
#include <ns3/pointer.h>
#include <ns3/slice.h>

#include <sys/types.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CustomQueueDisc");
NS_OBJECT_ENSURE_REGISTERED(CustomQueueDisc);

const std::unordered_map<Slice::SliceType, uint32_t> CustomQueueDisc::sliceTypeToQueueIndexMap = {
    {Slice::URLLC, 0},
    {Slice::eMBB, 1},
    {Slice::mMTC, 2}};

const std::unordered_map<uint32_t, Slice::SliceType> CustomQueueDisc::queueIndexToSliceTypeMap = {
    {0, Slice::URLLC},
    {1, Slice::eMBB},
    {2, Slice::mMTC}};

TypeId
CustomQueueDisc::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CustomQueueDisc")
                            .SetParent<QueueDisc>()
                            .SetGroupName("TrafficControl")
                            .AddConstructor<CustomQueueDisc>()
                            .AddAttribute("Node",
                                          "The node this queue disc is attached to",
                                          PointerValue(),
                                          MakePointerAccessor(&CustomQueueDisc::m_node),
                                          MakePointerChecker<Node>())
                            .AddAttribute("NetDevice",
                                          "The net device this queue disc is attached to",
                                          PointerValue(),
                                          MakePointerAccessor(&CustomQueueDisc::m_netDevice),
                                          MakePointerChecker<NetDevice>())
                            .AddAttribute("Port",
                                          "The port this queue disc is attached to",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&CustomQueueDisc::m_port),
                                          MakeUintegerChecker<uint32_t>());
    return tid;
}

CustomQueueDisc::CustomQueueDisc()
{
    m_queueDelays.resize(3);
    m_maxPacketsinQueue.resize(3);
    m_queueWeights.resize(3);
    m_queueWeights = {80, 15, 5}; // URLLC, eMBB, mMTC
    m_lastServedQueueIndex = 0;
    m_node = nullptr;
    m_netDevice = nullptr;
    m_port = 0;
}

CustomQueueDisc::~CustomQueueDisc()
{
}

uint32_t
CustomQueueDisc::GetQueueIndexFromDscp(uint8_t dscp) const
{
    if (dscp == 0)
    {
        return 1; // Default to eMBB
    }
    auto it = sliceTypeToQueueIndexMap.find(Slice::dscpToSliceTypeMap.at(dscp));
    if (it != sliceTypeToQueueIndexMap.end())
    {
        return it->second;
    }
    return 1; // Default to eMBB
}

bool
CustomQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    auto ipv4Item = DynamicCast<Ipv4QueueDiscItem>(item);
    if (!ipv4Item)
    {
        NS_LOG_WARN("Non-IPv4 packet received. Dropping.");
        return false;
    }

    std::string nodeName = Names::FindName(m_node);

    MetadataTag metadataTag;
    metadataTag.SetIngressTimestamp(Simulator::Now());
    item->GetPacket()->AddPacketTag(metadataTag);

    uint32_t queueIndex = GetQueueIndexFromDscp(ipv4Item->GetHeader().GetDscp());

    m_maxPacketsinQueue[queueIndex] =
        std::max(m_maxPacketsinQueue[queueIndex], GetInternalQueue(queueIndex)->GetNPackets());

    NS_LOG_DEBUG("[QueueDisc] Enqueueing packet on "
                 << nodeName << " port " << m_port << " | DSCP "
                 << static_cast<uint32_t>(ipv4Item->GetHeader().GetDscp()) << " | Queue "
                 << Slice::sliceTypeToStrMap.at(queueIndexToSliceTypeMap.at(queueIndex))
                 << " | Queue size: " << GetInternalQueue(queueIndex)->GetNPackets()
                 << " | Max queue size: " << m_maxPacketsinQueue[queueIndex]);

    return GetInternalQueue(queueIndex)->Enqueue(item);
}

Ptr<QueueDiscItem>
CustomQueueDisc::DoDequeue()
{
    static uint32_t currentQueueIndex = 0;
    // Packets served from each queue
    static std::vector<uint32_t> packetsServed(m_queueWeights.size(), 0);

    uint32_t numQueues = m_queueWeights.size();

    for (uint32_t i = 0; i < numQueues; i++)
    {
        uint32_t queueIndex = (currentQueueIndex + i) % numQueues;
        if (!GetInternalQueue(queueIndex)->IsEmpty())
        {
            Ptr<QueueDiscItem> item = GetInternalQueue(queueIndex)->Dequeue();
            MetadataTag metadataTag;
            item->GetPacket()->RemovePacketTag(metadataTag);
            Time ingressTimestamp = metadataTag.GetIngressTimestamp();
            Time queueDelay = Simulator::Now() - ingressTimestamp;
            m_queueDelays[queueIndex].push_back(queueDelay);

            packetsServed[queueIndex]++;
            if (packetsServed[queueIndex] >= m_queueWeights[queueIndex])
            {
                packetsServed[queueIndex] = 0;
                currentQueueIndex = (queueIndex + 1) % numQueues; // Move to the next queue
            }

            return item;
        }
    }

    return nullptr; // No packets in any queue
}

Ptr<const QueueDiscItem>
CustomQueueDisc::DoPeek()
{
    for (uint32_t i = 0; i < m_queueWeights.size(); i++)
    {
        if (!GetInternalQueue(i)->IsEmpty())
        {
            return GetInternalQueue(i)->Peek();
        }
    }
    return nullptr; // No packets in any queue
}

bool
CustomQueueDisc::CheckConfig()
{
    AddInternalQueue(CreateObject<DropTailQueue<QueueDiscItem>>()); // URLLC
    AddInternalQueue(CreateObject<DropTailQueue<QueueDiscItem>>()); // eMBB
    AddInternalQueue(CreateObject<DropTailQueue<QueueDiscItem>>()); // mMTC

    GetInternalQueue(0)->SetMaxSize(QueueSize("20KB"));  // URLLC
    GetInternalQueue(1)->SetMaxSize(QueueSize("500KB")); // eMBB
    GetInternalQueue(2)->SetMaxSize(QueueSize("200KB")); // mMTC

    return true;
}

void
CustomQueueDisc::InitializeParams()
{
}

void
CustomQueueDisc::PrintQueueStatistics()
{
    for (size_t i = 0; i < m_queueDelays.size(); ++i)
    {
        if (!m_queueDelays[i].empty())
        {
            std::sort(m_queueDelays[i].begin(), m_queueDelays[i].end());
            uint32_t maxQueueSize = m_maxPacketsinQueue[i];
            double maxQueueDelay = m_queueDelays[i].back().GetMilliSeconds();

            double averageQueueDelay = 0;
            for (const auto& delay : m_queueDelays[i])
            {
                averageQueueDelay += delay.GetMilliSeconds();
            }
            averageQueueDelay /= m_queueDelays[i].size();

            NS_LOG_INFO("[QueueDisc] Node: "
                        << Names::FindName(m_node) << " | Port: " << m_port << " | Queue: "
                        << Slice::sliceTypeToStrMap.at(queueIndexToSliceTypeMap.at(i))
                        << " | Max size: " << maxQueueSize << " | Max delay: " << maxQueueDelay
                        << " ms"
                        << " | Average delay: " << averageQueueDelay << " ms");
        }
    }
}

void
CustomQueueDisc::SetQueueWeights(std::map<Slice::SliceType, uint32_t> queueWeights)
{
    for (const auto& it : queueWeights)
    {
        auto sliceType = it.first;
        auto weight = it.second;
        auto it2 = sliceTypeToQueueIndexMap.find(sliceType);
        if (it2 != sliceTypeToQueueIndexMap.end())
        {
            m_queueWeights[it2->second] = weight;
        }
    }
}

} // namespace ns3