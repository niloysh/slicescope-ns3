#ifndef CUSTOM_QUEUE_DISC_H
#define CUSTOM_QUEUE_DISC_H

#include "ns3/net-device.h"
#include "ns3/queue-disc.h"
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
    static const std::unordered_map<Slice::SliceType, uint32_t> queueIndexMap;

  private:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;
    void InitializeParams() override;

    /**
     * \brief Get the queue index based on the DSCP value.
     * \param dscp The DSCP value of the packet.
     * \return The queue index (0 for URLLC, 1 for eMBB, 2 for mMTC).
     */
    uint32_t GetQueueIndexFromDscp(uint8_t dscp) const;

    std::vector<std::vector<ns3::Time>> m_queueDelays;
    std::vector<uint32_t> m_maxQueueSize; // Max queue size for each slice type
    std::vector<uint32_t> m_weights;      // Weights for each queue
    uint32_t m_lastServedQueue;           // Index of the last served queue
    Ptr<NetDevice> m_netDevice;
    Ptr<Node> m_node;
    uint32_t m_port;

    static constexpr const char* SLICE_TYPES[3] = {"URLLC", "eMBB", "mMTC"}; // Queue names
};

} // namespace ns3

#endif // SIMPLE_QUEUE_DISC_H