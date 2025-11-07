#include "include/RogueDHCPServer.h"

Define_Module(RogueDHCPServer);

void RogueDHCPServer::initialize()
{
    EV << "Rogue DHCP Server initialized\n";
}

void RogueDHCPServer::handleMessage(cMessage *msg)
{
    EV << "Rogue DHCP Server received: " << msg->getName() << endl;

    if (strcmp(msg->getName(), "DHCP_DISCOVER") == 0)
    {
        processDHCPDiscover(msg);
    }
    else if (strcmp(msg->getName(), "DHCP_REQUEST") == 0)
    {
        processDHCPRequest(msg);
    }
    else
    {
        delete msg;
    }
}

void RogueDHCPServer::processDHCPDiscover(cMessage *msg)
{
    // Rogue server responds immediately with same IPs as legitimate but different gateway
    cMessage *offer = new cMessage("DHCP_OFFER");
    offer->addPar("assignedIP");
    offer->addPar("gatewayIP");
    offer->addPar("clientPort");

    // Assign same IP range as legitimate server (192.168.10.1, 2, 3...)
    std::string assignedIP = "192.168.10." + std::to_string(nextIP);
    std::string rogueGateway = "10.0.0.100"; // Rogue gateway
    offer->par("assignedIP").setStringValue(assignedIP.c_str());
    offer->par("gatewayIP").setStringValue(rogueGateway.c_str());

    int clientPort = 2;
    if (msg->hasPar("clientPort"))
        clientPort = msg->par("clientPort").longValue();
    offer->par("clientPort").setLongValue(clientPort);

    EV << "🚨 ROGUE DHCP: Sending malicious offer with IP: " << assignedIP << " Gateway: " << rogueGateway << " to client port " << clientPort << endl;

    // Track that we offered this IP
    offeredIPs[assignedIP] = true;
    nextIP++;

    send(offer, "port$o");
    delete msg;
}

void RogueDHCPServer::processDHCPRequest(cMessage *msg)
{
    // Get the requested IP (should be same as legitimate server but with rogue gateway)
    std::string requestedIP;
    if (msg->hasPar("requestedIP"))
    {
        requestedIP = msg->par("requestedIP").stringValue();
    }
    else
    {
        requestedIP = "192.168.10." + std::to_string(nextIP - 1); // Use last offered IP
    }

    // Only respond if we offered this IP (rogue server wins race)
    if (requestedIP.find("192.168.10.") == 0 && offeredIPs.find(requestedIP) != offeredIPs.end())
    {
        cMessage *ack = new cMessage("DHCP_ACK");
        ack->addPar("assignedIP");
        ack->addPar("gatewayIP");
        ack->addPar("clientPort");

        ack->par("assignedIP").setStringValue(requestedIP.c_str());
        ack->par("gatewayIP").setStringValue("10.0.0.100"); // Rogue gateway

        int clientPort = 2;
        if (msg->hasPar("clientPort"))
            clientPort = msg->par("clientPort").longValue();
        ack->par("clientPort").setLongValue(clientPort);

        send(ack, "port$o");
        EV << "🚨 ROGUE DHCP: Successfully assigned malicious gateway! IP: " << requestedIP << " Gateway: 10.0.0.100 to client port " << clientPort << endl;
    }
    else
    {
        EV << "ROGUE DHCP: Ignoring request for IP we didn't offer: " << requestedIP << endl;
    }
    delete msg;
}

void RogueDHCPServer::refreshDisplay() const
{
    char buf[120];
    sprintf(buf, "🚨 Rogue DHCP\nNext IP: 192.168.10.%d\nGateway: 10.0.0.100", nextIP);
    getDisplayString().setTagArg("t", 0, buf);
}
