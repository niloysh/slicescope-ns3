#ifndef SIMPLE_QUEUE_DISC_H
#define SIMPLE_QUEUE_DISC_H

#include "ns3/queue-disc.h"

#include <vector>

namespace ns3
{

class SimpleQueueDisc : public QueueDisc
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    SimpleQueueDisc();
    ~SimpleQueueDisc() override;

    /**
     * \brief Print queue statistics (e.g., delays).
     */
    void PrintQueueStatistics();

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

    std::vector<std::vector<ns3::Time>> m_queueDelays; // Queue delays for each queue
    std::vector<uint32_t> m_maxQueueSize;              // Maximum queue size for each queue
    std::vector<uint32_t> m_weights;                   // Weights for each queue
    std::vector<uint32_t> m_deficit;                   // Deficit counters for each queue
    uint32_t m_lastServedQueue;                        // Index of the last served queue

    static constexpr const char* SLICE_NAMES[3] = {"URLLC", "eMBB", "mMTC"}; // Queue names
};

} // namespace ns3

#endif // SIMPLE_QUEUE_DISC_H