#ifndef CUSTOM_TRAFFIC_GENERATOR_H
#define CUSTOM_TRAFFIC_GENERATOR_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/inet-socket-address.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"

namespace ns3
{

class CustomTrafficGenerator : public Application
{
  public:
    static TypeId GetTypeId();
    CustomTrafficGenerator();
    virtual ~CustomTrafficGenerator();

    /**
     * \brief Setup the application.
     *
     * \param dest Destination address.
     * \param port Destination port.
     * \param dataRateMbps Data rate in Mbps.
     * \param minSize Minimum packet size.
     * \param maxSize Maximum packet size.
     */
    void Setup(Ipv4Address dest,
               uint16_t port,
               double dataRateMbps,
               uint32_t minSize,
               uint32_t maxSize,
               uint32_t maxPackets);

  protected:
    void StartApplication() override;
    void StopApplication() override;

  private:
    void SendPacket();
    Ptr<Socket> m_socket;
    Ipv4Address m_destIp;
    uint16_t m_destPort;
    EventId m_sendEvent;
    Ptr<ExponentialRandomVariable> m_interPacketTime;
    Ptr<UniformRandomVariable> m_packetSize;

    uint32_t m_minSize;
    uint32_t m_maxSize;
    uint32_t m_maxPackets;
    uint32_t m_packetsSent;
};

} // namespace ns3

#endif
