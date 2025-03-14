#ifndef SLICESCOPE_SWITCH_NET_DEVICE_H
#define SLICESCOPE_SWITCH_NET_DEVICE_H

#include "ns3/bridge-channel.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/nstime.h"

#include <map>
#include <stdint.h>

/**
 * \file
 * \ingroup bridge
 * ns3::SlicescopeSwitchNetDevice declaration.
 */

namespace ns3
{

class Node;

/**
 * \defgroup bridge Bridge Network Device
 *
 * \brief a virtual net device that bridges multiple LAN segments
 *
 * The SlicescopeSwitchNetDevice object is a "virtual" netdevice that aggregates
 * multiple "real" netdevices and implements the data plane forwarding
 * part of IEEE 802.1D.  By adding a SlicescopeSwitchNetDevice to a Node, it
 * will act as a "bridge", or "switch", to multiple LAN segments.
 *
 * By default the bridge netdevice implements a "learning bridge"
 * algorithm (see 802.1D), where incoming unicast frames from one port
 * may occasionally be forwarded throughout all other ports, but
 * usually they are forwarded only to a single correct output port.
 *
 * \attention The Spanning Tree Protocol part of 802.1D is not
 * implemented.  Therefore, you have to be careful not to create
 * bridging loops, or else the network will collapse.
 *
 * \attention Bridging is designed to work only with NetDevices
 * modelling IEEE 802-style technologies, such as CsmaNetDevice and
 * WifiNetDevice.
 *
 * \attention If including a WifiNetDevice in a bridge, the wifi
 * device must be in Access Point mode.  Adhoc mode is not supported
 * with bridging.
 */

/**
 * \ingroup bridge
 * \brief a virtual net device that bridges multiple LAN segments
 */
class SlicescopeSwitchNetDevice : public NetDevice
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    SlicescopeSwitchNetDevice();
    ~SlicescopeSwitchNetDevice() override;

    // Delete copy constructor and assignment operator to avoid misuse
    SlicescopeSwitchNetDevice(const SlicescopeSwitchNetDevice&) = delete;
    SlicescopeSwitchNetDevice& operator=(const SlicescopeSwitchNetDevice&) = delete;

    /**
     * \brief Add a 'port' to a bridge device
     * \param bridgePort the NetDevice to add
     *
     * This method adds a new bridge port to a SlicescopeSwitchNetDevice, so that
     * the new bridge port NetDevice becomes part of the bridge and L2
     * frames start being forwarded to/from this NetDevice.
     *
     * \attention The netdevice that is being added as bridge port must
     * _not_ have an IP address.  In order to add IP connectivity to a
     * bridging node you must enable IP on the SlicescopeSwitchNetDevice itself,
     * never on its port netdevices.
     */
    void AddBridgePort(Ptr<NetDevice> bridgePort);

    /**
     * \brief Gets the number of bridged 'ports', i.e., the NetDevices currently bridged.
     *
     * \return the number of bridged ports.
     */
    uint32_t GetNBridgePorts() const;

    /**
     * \brief Gets the n-th bridged port.
     * \param n the port index
     * \return the n-th bridged NetDevice
     */
    Ptr<NetDevice> GetBridgePort(uint32_t n) const;

    // inherited from NetDevice base class.
    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;
    Ptr<Channel> GetChannel() const override;
    void SetAddress(Address address) override;
    Address GetAddress() const override;
    bool SetMtu(const uint16_t mtu) override;
    uint16_t GetMtu() const override;
    bool IsLinkUp() const override;
    void AddLinkChangeCallback(Callback<void> callback) override;
    bool IsBroadcast() const override;
    Address GetBroadcast() const override;
    bool IsMulticast() const override;
    Address GetMulticast(Ipv4Address multicastGroup) const override;
    bool IsPointToPoint() const override;
    bool IsBridge() const override;
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;
    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;
    bool NeedsArp() const override;
    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;
    void SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;
    Address GetMulticast(Ipv6Address addr) const override;
    bool m_enableLayer3; //!< true if the bridge will forward IP packets

  protected:
    void DoDispose() override;

    /**
     * \brief Receives a packet from one bridged port.
     * \param device the originating port
     * \param packet the received packet
     * \param protocol the packet protocol (e.g., Ethertype)
     * \param source the packet source
     * \param destination the packet destination
     * \param packetType the packet type (e.g., host, broadcast, etc.)
     */
    void ReceiveFromDevice(Ptr<NetDevice> device,
                           Ptr<const Packet> packet,
                           uint16_t protocol,
                           const Address& source,
                           const Address& destination,
                           PacketType packetType);

    /**
     * \brief Forwards a unicast packet
     * \param incomingPort the packet incoming port
     * \param packet the packet
     * \param protocol the packet protocol (e.g., Ethertype)
     * \param src the packet source
     * \param dst the packet destination
     */
    void ForwardUnicast(Ptr<NetDevice> incomingPort,
                        Ptr<const Packet> packet,
                        uint16_t protocol,
                        Mac48Address src,
                        Mac48Address dst);

    /**
     * \brief Forwards a broadcast or a multicast packet
     * \param incomingPort the packet incoming port
     * \param packet the packet
     * \param protocol the packet protocol (e.g., Ethertype)
     * \param src the packet source
     * \param dst the packet destination
     */
    void ForwardBroadcast(Ptr<NetDevice> incomingPort,
                          Ptr<const Packet> packet,
                          uint16_t protocol,
                          Mac48Address src,
                          Mac48Address dst);

    /**
     * \brief Learns the port a MAC address is sending from
     * \param source source address
     * \param port the port the source is sending from
     */
    void Learn(Mac48Address source, Ptr<NetDevice> port);

    /**
     * \brief Gets the port associated to a source address
     * \param source the source address
     * \returns the port the source is associated to, or NULL if no association is known.
     */
    Ptr<NetDevice> GetLearnedState(Mac48Address source);

  private:
    NetDevice::ReceiveCallback m_rxCallback;               //!< receive callback
    NetDevice::PromiscReceiveCallback m_promiscRxCallback; //!< promiscuous receive callback

    Mac48Address m_address; //!< MAC address of the NetDevice
    Time m_expirationTime;  //!< time it takes for learned MAC state to expire

    /**
     * \ingroup bridge
     * Structure holding the status of an address
     */
    struct LearnedState
    {
        Ptr<NetDevice> associatedPort; //!< port associated with the address
        Time expirationTime;           //!< time it takes for learned MAC state to expire
    };

    std::map<Mac48Address, LearnedState> m_learnState; //!< Container for known address statuses
    Ptr<Node> m_node;                                  //!< node owning this NetDevice
    Ptr<BridgeChannel> m_channel;                      //!< virtual bridged channel
    std::vector<Ptr<NetDevice>> m_ports;               //!< bridged ports
    uint32_t m_ifIndex;                                //!< Interface index
    uint16_t m_mtu;                                    //!< MTU of the bridged NetDevice
    bool m_enableLearning; //!< true if the bridge will learn the node status
};

} // namespace ns3

#endif /* SLICESCOPE_SWITCH_NET_DEVICE_H */
