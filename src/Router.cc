/*
 * Router.cc
 *
 *  Created on: Oct 2, 2025
 *      Author: Asus
 */

#include "Router.h"

Define_Module(Router);

void Router::initialize()
{
    EV << "Router initialized with FIFO queue\n";
}

void Router::handleMessage(cMessage *msg)
{
    // Check if message is self-message (for scheduled processing)
    if (msg->isSelfMessage())
    {
        if (!packetQueue.empty())
        {
            cMessage *nextMsg = packetQueue.front();
            packetQueue.pop();

            EV << "Processing packet from FIFO: " << nextMsg->getName() << endl;
            processPacket(nextMsg);

            // Schedule next packet if queue not empty
            if (!packetQueue.empty())
            {
                scheduleAt(simTime() + serviceTime, msg);
            }
            else
            {
                isProcessing = false;
                delete msg; // no more packets to process
            }
        }
        return;
    }

    // Incoming message: enqueue into FIFO
    packetQueue.push(msg);
    EV << "Packet enqueued: " << msg->getName()
       << " | Queue length: " << packetQueue.size() << endl;

    // If not currently processing, start now
    if (!isProcessing)
    {
        isProcessing = true;
        cMessage *timer = new cMessage("processNext");
        scheduleAt(simTime() + serviceTime, timer);
    }

    refreshDisplay();
}

void Router::processPacket(cMessage *msg)
{
    EV << "Router processing: " << msg->getName() << endl;

    int arrivalGate = msg->getArrivalGate() ? msg->getArrivalGate()->getIndex() : -1;

    if (strcmp(msg->getName(), "DHCP_DISCOVER") == 0 || strcmp(msg->getName(), "DHCP_REQUEST") == 0)
    {
        int clientPort = arrivalGate;

        // Send to Rogue DHCP first
        cMessage *copyRogue = (cMessage *)msg->dup();
        copyRogue->addPar("clientPort").setLongValue(clientPort);
        send(copyRogue, "port$o", 6);

        // Send to legitimate DHCP
        msg->addPar("clientPort").setLongValue(clientPort);
        send(msg, "port$o", 0);

        updatePacketStats("DHCP");
    }
    else if (strcmp(msg->getName(), "DHCP_OFFER") == 0 || strcmp(msg->getName(), "DHCP_ACK") == 0)
    {
        int targetPort = msg->hasPar("clientPort") ? msg->par("clientPort").longValue() : 2;
        if (targetPort >= 2 && targetPort <= 4)
            send(msg, "port$o", targetPort);
        else
            delete msg;
    }
    else if (strcmp(msg->getName(), "DNS_QUERY") == 0)
    {
        msg->addPar("clientPort").setLongValue(arrivalGate);
        send(msg, "port$o", 1);
        updatePacketStats("DNS");
    }
    else if (strcmp(msg->getName(), "DNS_RESPONSE") == 0)
    {
        int targetPort = msg->hasPar("clientPort") ? msg->par("clientPort").longValue() : 2;
        if (targetPort >= 2 && targetPort <= 4)
            send(msg, "port$o", targetPort);
        else
            delete msg;
    }
    else if (strcmp(msg->getName(), "HTTP_REQUEST") == 0)
    {
        EV << "Router: HTTP_REQUEST received from gate " << arrivalGate << " name=" << msg->getName() << "\n";
        msg->addPar("clientPort").setLongValue(arrivalGate);
        send(msg, "port$o", 5);
        updatePacketStats("HTTP");
        httpRequests++; // Track individual request count
        EV << "HTTP counters => packets:" << httpPackets << " req:" << httpRequests << " resp:" << httpResponses << "\n";
    }
    else if (strcmp(msg->getName(), "HTTP_RESPONSE") == 0)
    {
        int targetPort = msg->hasPar("clientPort") ? msg->par("clientPort").longValue() : 2;
        if (targetPort >= 2 && targetPort <= 4)
        {
            EV << "Router: HTTP_RESPONSE routed to client port " << targetPort << " name=" << msg->getName() << "\n";
            send(msg, "port$o", targetPort);
            updatePacketStats("HTTP");
            httpResponses++; // Track individual response count
            EV << "HTTP counters => packets:" << httpPackets << " req:" << httpRequests << " resp:" << httpResponses << "\n";
        }
        else
            delete msg;
    }
    else
    {
        // Unknown packet type
        send(msg, "port$o", 2);
    }

    packetsProcessed++;
    refreshDisplay();
}

void Router::updatePacketStats(const std::string &packetType)
{
    if (packetType == "DHCP")
        dhcpPackets++;
    else if (packetType == "DNS")
        dnsPackets++;
    else if (packetType == "HTTP")
        httpPackets++;
}

void Router::refreshDisplay() const
{
    std::string display = "Router (FIFO)\nProcessed: " + std::to_string(packetsProcessed) +
                          "\nIn Queue: " + std::to_string(packetQueue.size()) +
                          "\nDHCP: " + std::to_string(dhcpPackets) +
                          "  DNS: " + std::to_string(dnsPackets) +
                          "\nHTTP Req: " + std::to_string(httpRequests) +
                          "  Resp: " + std::to_string(httpResponses);

    getDisplayString().setTagArg("t", 0, display.c_str());
}
