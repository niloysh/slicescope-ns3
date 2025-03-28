#ifndef CUSTOM_TRAFFIC_GENERATOR_H
#define CUSTOM_TRAFFIC_GENERATOR_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/inet-socket-address.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"

#include <cstdint>
#include <sys/types.h>

namespace ns3
{

class CustomTrafficGenerator : public Application
{
  public:
    static TypeId GetTypeId();
    CustomTrafficGenerator();
    ~CustomTrafficGenerator() override;

    uint32_t GetTotalPacketsSent() const;
    uint32_t GetTotalBytesSent() const;

  protected:
    void StartApplication() override;
    void StopApplication() override;

  private:
    void SendPacket();
    Ptr<Socket> m_socket;
    Ipv4Address m_destIp;
    uint16_t m_destPort;
    uint32_t m_maxPackets;
    EventId m_sendEvent;
    uint32_t m_packetsSent;
    double m_bytesSent;
    double m_dataRate;
    uint8_t m_dscp;
    bool m_running;
    Ptr<RandomVariableStream> m_packetSizeVar;
    Ptr<RandomVariableStream> m_jitterVar;

    // Precompute inter-arrival times for faster packet generation
    void PrecomputeInterarrivalTimes();
    std::queue<double> m_precomputedInterarrival;
};

} // namespace ns3

#endif
