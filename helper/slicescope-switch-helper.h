/*
 * Copyright (c) 2024 Your Name
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Your Name <your.email@example.com>
 */
#ifndef SLICESCOPE_SWITCH_HELPER_H
#define SLICESCOPE_SWITCH_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/object-factory.h"

#include <string>

namespace ns3
{

class Node;
class AttributeValue;

/**
 * \brief Add capability to switch multiple LAN segments (IEEE 802.1D bridging)
 */
class SlicescopeSwitchHelper
{
  public:
    /*
     * Construct a SlicescopeSwitchHelper
     */
    SlicescopeSwitchHelper();

    /**
     * Set an attribute on each ns3::SlicescopeSwitchNetDevice created by
     * SlicescopeSwitchHelper::Install
     *
     * \param n1 the name of the attribute to set
     * \param v1 the value of the attribute to set
     */
    void SetDeviceAttribute(std::string n1, const AttributeValue& v1);

    /**
     * This method creates an ns3::SlicescopeSwitchNetDevice with the attributes
     * configured by SlicescopeSwitchHelper::SetDeviceAttribute, adds the device
     * to the node, and attaches the given NetDevices as ports of the
     * switch.
     *
     * \param node The node to install the device in
     * \param c Container of NetDevices to add as switch ports
     * \returns A container holding the added net device.
     */
    NetDeviceContainer Install(Ptr<Node> node, NetDeviceContainer c);

    /**
     * This method creates an ns3::SlicescopeSwitchNetDevice with the attributes
     * configured by SlicescopeSwitchHelper::SetDeviceAttribute, adds the device
     * to the node, and attaches the given NetDevices as ports of the
     * switch.
     *
     * \param nodeName The name of the node to install the device in
     * \param c Container of NetDevices to add as switch ports
     * \returns A container holding the added net device.
     */
    NetDeviceContainer Install(std::string nodeName, NetDeviceContainer c);

  private:
    ObjectFactory m_deviceFactory; //!< Object factory
};

} // namespace ns3

#endif /* SLICESCOPE_SWITCH_HELPER_H */