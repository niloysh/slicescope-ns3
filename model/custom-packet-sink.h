#ifndef CUSTOM_PACKET_SINK_H
#define CUSTOM_PACKET_SINK_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"

#include <cstdint>
#include <map>

namespace ns3
{

struct FlowStats
{
    uint64_t totalBytes;
    uint32_t totalPackets;
};

class CustomPacketSink : public Application
{
  public:
    static TypeId GetTypeId();

    CustomPacketSink();
    ~CustomPacketSink() override;
    uint32_t GetTotalRxPackets() const;
    uint32_t GetTotalRx() const;
    std::map<std::pair<Ipv4Address, uint16_t>, FlowStats> GetFlowStats() const;
    std::vector<double> GetOwd() const;
    std::vector<std::pair<Time, double>> GetOwdRecords() const;

  private:
    void StartApplication() override;
    void StopApplication() override;
    void HandleRead(Ptr<Socket> socket);

    Ptr<Socket> m_socket;
    Address m_localAddress;
    uint16_t m_port;
    uint64_t m_totalRxBytes;
    uint32_t m_totalRxPackets;
    std::map<std::pair<Ipv4Address, uint16_t>, FlowStats> m_flowStats;
    std::vector<double> m_owd;
    std::vector<std::pair<Time, double>> m_owdRecords;

    double m_firstPacketTime = 0.0;
    double m_lastPacketTime = 0.0;
    EventId m_dataRateEvent;
    void ComputeDataRate();
    bool m_computeDataRate = false;
};

} // namespace ns3

#endif /* CUSTOM_PACKET_SINK */
