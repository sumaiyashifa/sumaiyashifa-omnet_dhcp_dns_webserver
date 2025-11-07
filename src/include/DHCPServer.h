/*
 * DHCPServer.h
 *
 *  Created on: Oct 2, 2025
 *      Author: Asus
 */

#ifndef __AREANETWORK_DHCPSERVER_H_
#define __AREANETWORK_DHCPSERVER_H_

#include <omnetpp.h>
#include <map>

using namespace omnetpp;

class DHCPServer : public cSimpleModule
{
private:
    int nextIP = 1;                         // Start IP addresses from 192.168.10.1
    std::map<std::string, bool> offeredIPs; // Track which IPs we offered

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void processDHCPDiscover(cMessage *msg);
    virtual void processDHCPRequest(cMessage *msg);
    virtual void refreshDisplay() const override;
};

#endif /* DHCPSERVER_H_ */