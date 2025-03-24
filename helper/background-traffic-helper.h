#ifndef BACKGROUND_TRAFFIC_HELPER_H
#define BACKGROUND_TRAFFIC_HELPER_H

#include "ns3/application-container.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/packet.h"

#include <sys/types.h>

namespace ns3
{

class BackgroundTrafficHelper
{
  public:
    enum TrafficType
    {
        UDP,
        ONOFF,
        BULK
    };

    BackgroundTrafficHelper()
    {
    }

    void Install(TrafficType type,
                 Ptr<Node> source,
                 Ptr<Node> sink,
                 Ipv4Address sinkAddr,
                 uint16_t port,
                 double startTime,
                 double stopTime,
                 std::string dataRate,
                 uint32_t packetSize,
                 uint32_t maxPackets,
                 uint32_t maxBytes);

    void InstallSaturatingTraffic(NodeContainer sources,
                                  NodeContainer sinks,
                                  double startTime,
                                  double stopTime,
                                  uint32_t packetSize,
                                  uint16_t basePort = 5000);

    void ScheduleRandomBurstsSrcDst(Ptr<Node> src,
                                    Ptr<Node> dst,
                                    Ipv4Address dstAddr,
                                    uint16_t basePort,
                                    double simulationEndTime,
                                    uint32_t numBursts,
                                    std::string minRate,
                                    std::string maxRate,
                                    double minDuration,
                                    double maxDuration);

    void ScheduleRandomBursts(NodeContainer sources,
                              NodeContainer sinks,
                              double simulationEndTime,
                              uint32_t numBursts,
                              std::string minRate,
                              std::string maxRate,
                              double minDuration,
                              double maxDuration);

    uint64_t GetTotalBytesSent() const;
    uint64_t GetTotalBytesReceived() const;
    uint32_t GetTotalPacketsSent() const;
    uint32_t GetTotalPacketsReceived() const;

  private:
    void TxTrace(Ptr<const Packet> packet);
    void RxTrace(Ptr<const Packet> packet, const Address& addr);

  private:
    uint64_t m_bytesSent = 0;
    uint64_t m_bytesReceived = 0;
    uint32_t m_packetsSent = 0;
    uint32_t m_packetsReceived = 0;
    Ptr<PacketSink> m_sinkApp;
};

} // namespace ns3

#endif // BACKGROUND_TRAFFIC_HELPER_H