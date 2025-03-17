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
    std::vector<double> timestamps;
};

class CustomPacketSink : public Application
{
  public:
    static TypeId GetTypeId();

    CustomPacketSink();
    ~CustomPacketSink() override;
    void PrintStats() const;
    uint32_t GetTotalPackets() const;
    uint32_t GetTotalBytes() const;
    std::map<std::pair<Ipv4Address, uint16_t>, FlowStats> GetFlowStats() const;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /*
     * \brief Handle a packet received by the application
     * \param socket the receiving socket
     */
    void HandleRead(Ptr<Socket> socket);

    Ptr<Socket> m_socket;
    Address m_localAddress;
    uint16_t m_port;
    uint64_t m_totalBytes;
    uint32_t m_totalPackets;
    std::map<std::pair<Ipv4Address, uint16_t>, FlowStats> m_flowStats;
};

} // namespace ns3

#endif /* CUSTOM_PACKET_SINK */
