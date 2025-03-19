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

const std::unordered_map<Slice::SliceType, uint32_t> CustomQueueDisc::queueIndexMap = {
    {Slice::URLLC, 0},
    {Slice::eMBB, 1},
    {Slice::mMTC, 2}};

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
    m_maxQueueSize.resize(3);
    m_weights = {6, 0, 0};
    m_lastServedQueue = 0;
    m_node = nullptr;
    m_netDevice = nullptr;
    m_port = 0;
}

CustomQueueDisc::~CustomQueueDisc()
{
    // PrintQueueStatistics();
}

uint32_t
CustomQueueDisc::GetQueueIndexFromDscp(uint8_t dscp) const
{
    if (dscp == Slice::dscpMap.at(Slice::URLLC))
    {
        return queueIndexMap.at(Slice::URLLC);
    }
    if (dscp == Slice::dscpMap.at(Slice::eMBB))
    {
        return queueIndexMap.at(Slice::eMBB);
    }
    if (dscp == Slice::dscpMap.at(Slice::mMTC))
    {
        return queueIndexMap.at(Slice::mMTC);
    }
    return queueIndexMap.at(Slice::eMBB); // Default to eMBB
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

    m_maxQueueSize[queueIndex] =
        std::max(m_maxQueueSize[queueIndex], GetInternalQueue(queueIndex)->GetNPackets());

    NS_LOG_DEBUG("[QueueDisc] Enqueueing packet on "
                 << nodeName << " port " << m_port << " | DSCP "
                 << static_cast<uint32_t>(ipv4Item->GetHeader().GetDscp()) << " | Queue "
                 << SLICE_TYPES[queueIndex]
                 << " | Queue size: " << GetInternalQueue(queueIndex)->GetNPackets()
                 << " | Max queue size: " << m_maxQueueSize[queueIndex]);

    return GetInternalQueue(queueIndex)->Enqueue(item);
}

Ptr<QueueDiscItem>
CustomQueueDisc::DoDequeue()
{
    for (uint32_t i = 0; i < m_weights.size(); i++)
    {
        uint32_t queueIndex = (m_lastServedQueue + i) % m_weights.size();
        if (!GetInternalQueue(queueIndex)->IsEmpty())
        {
            Ptr<QueueDiscItem> item = GetInternalQueue(queueIndex)->Dequeue();

            MetadataTag metadataTag;
            item->GetPacket()->RemovePacketTag(metadataTag);
            Time ingressTimestamp = metadataTag.GetIngressTimestamp();
            Time queueDelay = Simulator::Now() - ingressTimestamp;
            m_queueDelays[queueIndex].push_back(queueDelay);

            NS_LOG_DEBUG("[QueueDisc] Dequeueing packet from "
                         << Names::FindName(m_node) << " port " << m_port << " | DSCP "
                         << static_cast<uint32_t>(
                                DynamicCast<Ipv4QueueDiscItem>(item)->GetHeader().GetDscp())
                         << " | Queue " << SLICE_TYPES[queueIndex]
                         << " | Queue size: " << GetInternalQueue(queueIndex)->GetNPackets()
                         << " | Queue delay: " << queueDelay.GetMilliSeconds() << " ms");

            m_lastServedQueue = queueIndex;
            return item;
        }
    }
    return nullptr; // No packets in any queue
}

Ptr<const QueueDiscItem>
CustomQueueDisc::DoPeek()
{
    return nullptr;
}

bool
CustomQueueDisc::CheckConfig()
{
    AddInternalQueue(CreateObject<DropTailQueue<QueueDiscItem>>()); // URLLC
    AddInternalQueue(CreateObject<DropTailQueue<QueueDiscItem>>()); // eMBB
    AddInternalQueue(CreateObject<DropTailQueue<QueueDiscItem>>()); // mMTC

    for (uint32_t i = 0; i < GetNInternalQueues(); ++i)
    {
        GetInternalQueue(i)->SetMaxSize(QueueSize("200p")); // Set max size to 100 packets
    }

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
            uint32_t maxQueueSize = m_maxQueueSize[i];
            double maxQueueDelay = m_queueDelays[i].back().GetMilliSeconds();

            double averageQueueDelay = 0;
            for (const auto& delay : m_queueDelays[i])
            {
                averageQueueDelay += delay.GetMilliSeconds();
            }
            averageQueueDelay /= m_queueDelays[i].size();

            NS_LOG_INFO("[QueueDisc] Node: " << Names::FindName(m_node) << " | Port: " << m_port
                                             << " | Queue: " << SLICE_TYPES[i]
                                             << " | Max size: " << maxQueueSize
                                             << " | Max delay: " << maxQueueDelay << " ms"
                                             << " | Average delay: " << averageQueueDelay << " ms");
        }
    }
}

} // namespace ns3