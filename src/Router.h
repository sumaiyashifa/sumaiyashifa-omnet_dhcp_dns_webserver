/*
 * Router.h
 *
 *  Created on: Oct 2, 2025
 *      Author: Asus
 */

#ifndef __AREANETWORK_ROUTER_H_
#define __AREANETWORK_ROUTER_H_

#include <omnetpp.h>
#include <queue>
#include <string>
#include "messages/DhcpMessage_m.h"
#include "messages/DnsMessage_m.h"
#include "messages/DataPacket_m.h"

using namespace omnetpp;

class Router : public cSimpleModule
{
private:
    // Packet Queue (FIFO)
    std::queue<cMessage *> packetQueue;
    bool isProcessing = false;
    simtime_t serviceTime = 0.01; // Processing delay between packets

    // Statistics
    int packetsProcessed = 0;
    int dhcpPackets = 0;
    int dnsPackets = 0;
    int httpPackets = 0;
    int httpRequests = 0;
    int httpResponses = 0;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;

    // Helper functions
    virtual void processPacket(cMessage *msg);
    virtual void updatePacketStats(const std::string &packetType);
};

#endif
