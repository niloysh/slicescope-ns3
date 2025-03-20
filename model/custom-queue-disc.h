#ifndef CUSTOM_QUEUE_DISC_H
#define CUSTOM_QUEUE_DISC_H

#include "ns3/net-device.h"
#include "ns3/queue-disc.h"
#include <ns3/drop-tail-queue.h>
#include <ns3/node.h>
#include <ns3/slice.h>

#include <vector>

namespace ns3
{

class CustomQueueDisc : public QueueDisc
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    CustomQueueDisc();
    ~CustomQueueDisc() override;

    /**
     * \brief Print queue statistics (e.g., delays).
     */
    void PrintQueueStatistics();
    Ptr<NetDevice> GetNetDevice() const;
    static const std::unordered_map<Slice::SliceType, uint32_t> sliceTypeToQueueIndexMap;
    static const std::unordered_map<uint32_t, Slice::SliceType> queueIndexToSliceTypeMap;
    void SetQueueWeights(std::map<Slice::SliceType, uint32_t> queueWeights);

  private:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;
    void InitializeParams() override;
    uint32_t GetQueueIndexFromDscp(uint8_t dscp) const;

    std::vector<std::vector<ns3::Time>> m_queueDelays;
    std::vector<uint32_t> m_maxPacketsinQueue;
    std::vector<uint32_t> m_queueWeights;
    uint32_t m_lastServedQueueIndex;
    Ptr<NetDevice> m_netDevice;
    Ptr<Node> m_node;
    uint32_t m_port;
    std::vector<Ptr<DropTailQueue<QueueDiscItem>>> m_internalQueues;
};

} // namespace ns3

#endif // SIMPLE_QUEUE_DISC_H