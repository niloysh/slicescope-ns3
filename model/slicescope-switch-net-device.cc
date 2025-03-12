#include "slicescope-switch-net-device.h"

#include "slicescope-header.h"

#include "ns3/boolean.h"
#include "ns3/channel.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/udp-header.h"
#include "ns3/uinteger.h"

/**
 * \file
 * \ingroup bridge
 * ns3::SlicescopeSwitchNetDevice implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SlicescopeSwitchNetDevice");

NS_OBJECT_ENSURE_REGISTERED(SlicescopeSwitchNetDevice);

TypeId
SlicescopeSwitchNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SlicescopeSwitchNetDevice")
            .SetParent<NetDevice>()
            .SetGroupName("Bridge")
            .AddConstructor<SlicescopeSwitchNetDevice>()
            .AddAttribute("Mtu",
                          "The MAC-level Maximum Transmission Unit",
                          UintegerValue(1500),
                          MakeUintegerAccessor(&SlicescopeSwitchNetDevice::SetMtu,
                                               &SlicescopeSwitchNetDevice::GetMtu),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("EnableLearning",
                          "Enable the learning mode of the Learning Bridge",
                          BooleanValue(true),
                          MakeBooleanAccessor(&SlicescopeSwitchNetDevice::m_enableLearning),
                          MakeBooleanChecker())
            .AddAttribute("ExpirationTime",
                          "Time it takes for learned MAC state entry to expire.",
                          TimeValue(Seconds(300)),
                          MakeTimeAccessor(&SlicescopeSwitchNetDevice::m_expirationTime),
                          MakeTimeChecker())
            .AddAttribute("EnableLayer3",
                          "Enable processing at Layer 3",
                          BooleanValue(true),
                          MakeBooleanAccessor(&SlicescopeSwitchNetDevice::m_enableLayer3),
                          MakeBooleanChecker());
    return tid;
}

SlicescopeSwitchNetDevice::SlicescopeSwitchNetDevice()
    : m_node(nullptr),
      m_ifIndex(0)
{
    NS_LOG_FUNCTION_NOARGS();
    m_channel = CreateObject<BridgeChannel>();
}

SlicescopeSwitchNetDevice::~SlicescopeSwitchNetDevice()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
SlicescopeSwitchNetDevice::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();
    for (auto iter = m_ports.begin(); iter != m_ports.end(); iter++)
    {
        *iter = nullptr;
    }
    m_ports.clear();
    m_channel = nullptr;
    m_node = nullptr;
    NetDevice::DoDispose();
}

void
SlicescopeSwitchNetDevice::ReceiveFromDevice(Ptr<NetDevice> incomingPort,
                                             Ptr<const Packet> packet,
                                             uint16_t protocol,
                                             const Address& src,
                                             const Address& dst,
                                             PacketType packetType)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_DEBUG("UID is " << packet->GetUid());

    Mac48Address src48 = Mac48Address::ConvertFrom(src);
    Mac48Address dst48 = Mac48Address::ConvertFrom(dst);

    if (!m_promiscRxCallback.IsNull())
    {
        m_promiscRxCallback(this, packet, protocol, src, dst, packetType);
    }

    Ptr<Packet> copy = packet->Copy();
    if (m_enableLayer3)
    {
        Ipv4Header ipv4Header;
        if (copy->PeekHeader(ipv4Header))
        {
            copy->RemoveHeader(ipv4Header);
            Ipv4Address ipv4Src = ipv4Header.GetSource();
            Ipv4Address ipv4Dst = ipv4Header.GetDestination();
            uint8_t protocol = ipv4Header.GetProtocol();

            NS_LOG_INFO("*** Node" << m_node->GetId() << " ***");
            NS_LOG_INFO("IPv4 Source: " << ipv4Src << " Destination: " << ipv4Dst
                                        << " Protocol: " << (uint32_t)protocol);

            if (protocol == 17)
            {
                UdpHeader udpHeader;
                copy->RemoveHeader(udpHeader);
                // uint16_t srcPort = udpHeader.GetSourcePort();
                // uint16_t dstPort = udpHeader.GetDestinationPort();
                // NS_LOG_INFO("Original UDP Source Port: " << srcPort
                //                                          << " Destination Port: " << dstPort);

                // // Modify the source port
                // uint16_t newSrcPort = srcPort; // New source port value
                // udpHeader.SetSourcePort(newSrcPort);
                // NS_LOG_INFO("New UDP Source Port: " << udpHeader.GetSourcePort());

                SlicescopeHeader slicescopeHeader;
                slicescopeHeader.SetDscp(42);     // New DSCP value
                slicescopeHeader.SetBitmap(0xFF); // New bitmap value
                uint32_t slicescopeHeaderSize = slicescopeHeader.GetSerializedSize();
                NS_LOG_INFO("Adding slicescope header. Previous size: "
                            << copy->GetSize()
                            << " New size: " << copy->GetSize() + slicescopeHeaderSize);
                copy->AddHeader(slicescopeHeader);

                // Update UDP length
                uint16_t udpLength = udpHeader.GetSerializedSize() + slicescopeHeaderSize;
                udpHeader.ForcePayloadSize(udpLength);

                copy->AddHeader(udpHeader);

                // Update IPv4 length
                uint16_t ipv4Length = ipv4Header.GetPayloadSize() + slicescopeHeaderSize;
                ipv4Header.SetPayloadSize(ipv4Length);
            }

            copy->AddHeader(ipv4Header);
        }
    }

    switch (packetType)
    {
    case PACKET_HOST:
        if (dst48 == m_address)
        {
            Learn(src48, incomingPort);
            m_rxCallback(this, copy, protocol, src);
        }
        break;

    case PACKET_BROADCAST:
    case PACKET_MULTICAST:
        m_rxCallback(this, copy, protocol, src);
        ForwardBroadcast(incomingPort, copy, protocol, src48, dst48);
        break;

    case PACKET_OTHERHOST:
        if (dst48 == m_address)
        {
            Learn(src48, incomingPort);
            m_rxCallback(this, copy, protocol, src);
        }
        else
        {
            ForwardUnicast(incomingPort, copy, protocol, src48, dst48);
        }
        break;
    }
}

void
SlicescopeSwitchNetDevice::ForwardUnicast(Ptr<NetDevice> incomingPort,
                                          Ptr<const Packet> packet,
                                          uint16_t protocol,
                                          Mac48Address src,
                                          Mac48Address dst)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_DEBUG("LearningBridgeForward (incomingPort="
                 << incomingPort->GetInstanceTypeId().GetName() << ", packet=" << packet
                 << ", protocol=" << protocol << ", src=" << src << ", dst=" << dst << ")");

    Learn(src, incomingPort);
    Ptr<NetDevice> outPort = GetLearnedState(dst);
    if (outPort && outPort != incomingPort)
    {
        NS_LOG_LOGIC("Learning bridge state says to use port `"
                     << outPort->GetInstanceTypeId().GetName() << "'");
        outPort->SendFrom(packet->Copy(), src, dst, protocol);
    }
    else
    {
        NS_LOG_LOGIC("No learned state: send through all ports");
        for (auto iter = m_ports.begin(); iter != m_ports.end(); iter++)
        {
            Ptr<NetDevice> port = *iter;
            if (port != incomingPort)
            {
                NS_LOG_LOGIC("LearningBridgeForward ("
                             << src << " => " << dst
                             << "): " << incomingPort->GetInstanceTypeId().GetName() << " --> "
                             << port->GetInstanceTypeId().GetName() << " (UID " << packet->GetUid()
                             << ").");
                port->SendFrom(packet->Copy(), src, dst, protocol);
            }
        }
    }
}

void
SlicescopeSwitchNetDevice::ForwardBroadcast(Ptr<NetDevice> incomingPort,
                                            Ptr<const Packet> packet,
                                            uint16_t protocol,
                                            Mac48Address src,
                                            Mac48Address dst)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_DEBUG("LearningBridgeForward (incomingPort="
                 << incomingPort->GetInstanceTypeId().GetName() << ", packet=" << packet
                 << ", protocol=" << protocol << ", src=" << src << ", dst=" << dst << ")");
    Learn(src, incomingPort);

    for (auto iter = m_ports.begin(); iter != m_ports.end(); iter++)
    {
        Ptr<NetDevice> port = *iter;
        if (port != incomingPort)
        {
            NS_LOG_LOGIC("LearningBridgeForward (" << src << " => " << dst << "): "
                                                   << incomingPort->GetInstanceTypeId().GetName()
                                                   << " --> " << port->GetInstanceTypeId().GetName()
                                                   << " (UID " << packet->GetUid() << ").");
            port->SendFrom(packet->Copy(), src, dst, protocol);
        }
    }
}

void
SlicescopeSwitchNetDevice::Learn(Mac48Address source, Ptr<NetDevice> port)
{
    NS_LOG_FUNCTION_NOARGS();
    if (m_enableLearning)
    {
        LearnedState& state = m_learnState[source];
        state.associatedPort = port;
        state.expirationTime = Simulator::Now() + m_expirationTime;
    }
}

Ptr<NetDevice>
SlicescopeSwitchNetDevice::GetLearnedState(Mac48Address source)
{
    NS_LOG_FUNCTION_NOARGS();
    if (m_enableLearning)
    {
        Time now = Simulator::Now();
        auto iter = m_learnState.find(source);
        if (iter != m_learnState.end())
        {
            LearnedState& state = iter->second;
            if (state.expirationTime > now)
            {
                return state.associatedPort;
            }
            else
            {
                m_learnState.erase(iter);
            }
        }
    }
    return nullptr;
}

uint32_t
SlicescopeSwitchNetDevice::GetNBridgePorts() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_ports.size();
}

Ptr<NetDevice>
SlicescopeSwitchNetDevice::GetBridgePort(uint32_t n) const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_ports[n];
}

void
SlicescopeSwitchNetDevice::AddBridgePort(Ptr<NetDevice> bridgePort)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_ASSERT(bridgePort != this);
    if (!Mac48Address::IsMatchingType(bridgePort->GetAddress()))
    {
        NS_FATAL_ERROR("Device does not support eui 48 addresses: cannot be added to bridge.");
    }
    if (!bridgePort->SupportsSendFrom())
    {
        NS_FATAL_ERROR("Device does not support SendFrom: cannot be added to bridge.");
    }
    if (m_address == Mac48Address())
    {
        m_address = Mac48Address::ConvertFrom(bridgePort->GetAddress());
    }

    NS_LOG_DEBUG("RegisterProtocolHandler for " << bridgePort->GetInstanceTypeId().GetName());
    m_node->RegisterProtocolHandler(
        MakeCallback(&SlicescopeSwitchNetDevice::ReceiveFromDevice, this),
        0,
        bridgePort,
        true);
    m_ports.push_back(bridgePort);
    m_channel->AddChannel(bridgePort->GetChannel());
}

void
SlicescopeSwitchNetDevice::SetIfIndex(const uint32_t index)
{
    NS_LOG_FUNCTION_NOARGS();
    m_ifIndex = index;
}

uint32_t
SlicescopeSwitchNetDevice::GetIfIndex() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_ifIndex;
}

Ptr<Channel>
SlicescopeSwitchNetDevice::GetChannel() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_channel;
}

void
SlicescopeSwitchNetDevice::SetAddress(Address address)
{
    NS_LOG_FUNCTION_NOARGS();
    m_address = Mac48Address::ConvertFrom(address);
}

Address
SlicescopeSwitchNetDevice::GetAddress() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_address;
}

bool
SlicescopeSwitchNetDevice::SetMtu(const uint16_t mtu)
{
    NS_LOG_FUNCTION_NOARGS();
    m_mtu = mtu;
    return true;
}

uint16_t
SlicescopeSwitchNetDevice::GetMtu() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_mtu;
}

bool
SlicescopeSwitchNetDevice::IsLinkUp() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

void
SlicescopeSwitchNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
}

bool
SlicescopeSwitchNetDevice::IsBroadcast() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

Address
SlicescopeSwitchNetDevice::GetBroadcast() const
{
    NS_LOG_FUNCTION_NOARGS();
    return Mac48Address::GetBroadcast();
}

bool
SlicescopeSwitchNetDevice::IsMulticast() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

Address
SlicescopeSwitchNetDevice::GetMulticast(Ipv4Address multicastGroup) const
{
    NS_LOG_FUNCTION(this << multicastGroup);
    Mac48Address multicast = Mac48Address::GetMulticast(multicastGroup);
    return multicast;
}

bool
SlicescopeSwitchNetDevice::IsPointToPoint() const
{
    NS_LOG_FUNCTION_NOARGS();
    return false;
}

bool
SlicescopeSwitchNetDevice::IsBridge() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

bool
SlicescopeSwitchNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION_NOARGS();
    return SendFrom(packet, m_address, dest, protocolNumber);
}

bool
SlicescopeSwitchNetDevice::SendFrom(Ptr<Packet> packet,
                                    const Address& src,
                                    const Address& dest,
                                    uint16_t protocolNumber)
{
    NS_LOG_FUNCTION_NOARGS();
    Mac48Address dst = Mac48Address::ConvertFrom(dest);

    // try to use the learned state if data is unicast
    if (!dst.IsGroup())
    {
        Ptr<NetDevice> outPort = GetLearnedState(dst);
        if (outPort)
        {
            outPort->SendFrom(packet, src, dest, protocolNumber);
            return true;
        }
    }

    // data was not unicast or no state has been learned for that mac
    // address => flood through all ports.
    Ptr<Packet> pktCopy;
    for (auto iter = m_ports.begin(); iter != m_ports.end(); iter++)
    {
        pktCopy = packet->Copy();
        Ptr<NetDevice> port = *iter;
        port->SendFrom(pktCopy, src, dest, protocolNumber);
    }

    return true;
}

Ptr<Node>
SlicescopeSwitchNetDevice::GetNode() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_node;
}

void
SlicescopeSwitchNetDevice::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION_NOARGS();
    m_node = node;
}

bool
SlicescopeSwitchNetDevice::NeedsArp() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

void
SlicescopeSwitchNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
    NS_LOG_FUNCTION_NOARGS();
    m_rxCallback = cb;
}

void
SlicescopeSwitchNetDevice::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb)
{
    NS_LOG_FUNCTION_NOARGS();
    m_promiscRxCallback = cb;
}

bool
SlicescopeSwitchNetDevice::SupportsSendFrom() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

Address
SlicescopeSwitchNetDevice::GetMulticast(Ipv6Address addr) const
{
    NS_LOG_FUNCTION(this << addr);
    return Mac48Address::GetMulticast(addr);
}

} // namespace ns3
