//
// Minimal rogue DHCP server that responds with rogue IPs quickly
//

#ifndef __AREANETWORK_ROGUEDHCPSERVER_H_
#define __AREANETWORK_ROGUEDHCPSERVER_H_

#include <omnetpp.h>
#include <map>

using namespace omnetpp;

class RogueDHCPServer : public cSimpleModule
{
private:
    int nextIP = 1;                       // start at 100 so IPs begin at 192.168.10.100
    std::map<std::string, bool> offeredIPs; // Track which IPs we offered

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    void processDHCPDiscover(cMessage *msg);
    void processDHCPRequest(cMessage *msg);
    virtual void refreshDisplay() const override;
};

#endif
