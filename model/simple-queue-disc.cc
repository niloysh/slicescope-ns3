#include "simple-queue-disc.h"

#include "time-tag.h"

#include "ns3/drop-tail-queue.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SimpleQueueDisc");
NS_OBJECT_ENSURE_REGISTERED(SimpleQueueDisc);

TypeId
SimpleQueueDisc::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SimpleQueueDisc")
                            .SetParent<QueueDisc>()
                            .SetGroupName("TrafficControl")
                            .AddConstructor<SimpleQueueDisc>();
    return tid;
}

SimpleQueueDisc::SimpleQueueDisc()
{
    m_queueDelays.resize(3);
    m_maxQueueSize.resize(3);
    m_weights = {10, 6, 4}; // Weights for URLLC, eMBB, mMTC
    m_deficit = m_weights;
    m_lastServedQueue = 0;
}

SimpleQueueDisc::~SimpleQueueDisc()
{
    // PrintQueueStatistics();
}

uint32_t
SimpleQueueDisc::GetQueueIndexFromDscp(uint8_t dscp) const
{
    if (dscp == 46)
    {
        return 0; // URLLC
    }
    if (dscp == 40)
    {
        return 1; // eMBB
    }
    if (dscp == 8)
    {
        return 2; // mMTC
    }
    return 1; // Default to eMBB
}

bool
SimpleQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    auto ipv4Item = DynamicCast<Ipv4QueueDiscItem>(item);
    if (!ipv4Item)
    {
        NS_LOG_WARN("Non-IPv4 packet received. Dropping.");
        return false;
    }

    TimeTag tag;
    tag.SetTime(Simulator::Now());
    item->GetPacket()->AddPacketTag(tag);

    uint32_t queueIndex = GetQueueIndexFromDscp(ipv4Item->GetHeader().GetDscp());

    m_maxQueueSize[queueIndex] =
        std::max(m_maxQueueSize[queueIndex], GetInternalQueue(queueIndex)->GetNPackets());

    return GetInternalQueue(queueIndex)->Enqueue(item);
}

Ptr<QueueDiscItem>
SimpleQueueDisc::DoDequeue()
{
    for (uint32_t i = 0; i < m_queueDelays.size(); ++i)
    {
        uint32_t queueIndex = (m_lastServedQueue + i) % m_queueDelays.size();

        if (m_deficit[queueIndex] > 0)
        {
            Ptr<QueueDiscItem> item = GetInternalQueue(queueIndex)->Dequeue();
            if (item)
            {
                TimeTag tag;
                item->GetPacket()->RemovePacketTag(tag);
                Time delay = Simulator::Now() - tag.GetTime();
                m_queueDelays[queueIndex].push_back(delay);

                m_deficit[queueIndex]--;
                m_lastServedQueue = queueIndex;
                return item;
            }
        }

        if (GetInternalQueue(queueIndex)->IsEmpty())
        {
            m_deficit[queueIndex] = m_weights[queueIndex];
        }
    }

    return nullptr;
}

Ptr<const QueueDiscItem>
SimpleQueueDisc::DoPeek()
{
    return nullptr;
}

bool
SimpleQueueDisc::CheckConfig()
{
    AddInternalQueue(CreateObject<DropTailQueue<QueueDiscItem>>()); // URLLC
    AddInternalQueue(CreateObject<DropTailQueue<QueueDiscItem>>()); // eMBB
    AddInternalQueue(CreateObject<DropTailQueue<QueueDiscItem>>()); // mMTC
    return true;
}

void
SimpleQueueDisc::InitializeParams()
{
}

void
SimpleQueueDisc::PrintQueueStatistics()
{
    const char* SLICE_TYPES[] = {"URLLC", "eMBB", "mMTC"};
    for (size_t i = 0; i < m_queueDelays.size(); ++i)
    {
        if (!m_queueDelays[i].empty())
        {
            std::sort(m_queueDelays[i].begin(), m_queueDelays[i].end());
            uint32_t maxQueueSize = m_maxQueueSize[i];
            double maxQueueDelay = m_queueDelays[i].back().GetMilliSeconds();
            NS_LOG_INFO("Queue " << SLICE_TYPES[i] << " - Max queue size: " << maxQueueSize
                                 << ", Max queue delay: " << maxQueueDelay << " ms");
        }
    }
}

} // namespace ns3