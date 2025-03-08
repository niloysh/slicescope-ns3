#ifndef SIMPLE_PACKET_SINK_H
#define SIMPLE_PACKET_SINK_H

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

class SimplePacketSink : public Application
{
  public:
    static TypeId GetTypeId();

    SimplePacketSink();
    ~SimplePacketSink() override;

    void Setup(uint16_t port);
    void PrintStats() const;

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
    uint64_t m_totalBytes;
    uint32_t m_totalPackets;
    std::map<std::pair<Ipv4Address, uint16_t>, FlowStats> m_flowStats;
};

} // namespace ns3

#endif /* SIMPLE_PACKET_SINK_H */
