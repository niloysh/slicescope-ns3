#include "slicescope-switch-helper.h"

#include "ns3/bridge-net-device.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/slicescope-switch-net-device.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SlicescopeSwitchHelper");

SlicescopeSwitchHelper::SlicescopeSwitchHelper()
{
    NS_LOG_FUNCTION_NOARGS();
    m_deviceFactory.SetTypeId("ns3::SlicescopeSwitchNetDevice");
}

void
SlicescopeSwitchHelper::SetDeviceAttribute(std::string n1, const AttributeValue& v1)
{
    NS_LOG_FUNCTION_NOARGS();
    m_deviceFactory.Set(n1, v1);
}

NetDeviceContainer
SlicescopeSwitchHelper::Install(Ptr<Node> node, NetDeviceContainer c)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_INFO("**** Install slicescope switch device on node " << node->GetId());

    NetDeviceContainer devs;
    Ptr<SlicescopeSwitchNetDevice> dev = m_deviceFactory.Create<SlicescopeSwitchNetDevice>();
    devs.Add(dev);
    node->AddDevice(dev);

    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        NS_LOG_INFO("**** Add SwitchPort " << *i);
        // dev->AddSwitchPort(*i);
        dev->AddBridgePort(*i);
    }
    return devs;
}

NetDeviceContainer
SlicescopeSwitchHelper::Install(std::string nodeName, NetDeviceContainer c)
{
    NS_LOG_FUNCTION_NOARGS();
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return Install(node, c);
}

} // namespace ns3