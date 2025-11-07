#include "include/DHCPServer.h"

Define_Module(DHCPServer);

void DHCPServer::initialize()
{
    EV << "DHCP Server initialized\n";
}

void DHCPServer::handleMessage(cMessage *msg)
{
    EV << "DHCP Server received: " << msg->getName() << endl;

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

void DHCPServer::processDHCPDiscover(cMessage *msg)
{
    EV << "Processing DHCP Discover from client\n";

    // Add a small delay to legitimate server to simulate slower response
    // In real scenario, rogue server would be closer/have better connection
    cMessage *offer = new cMessage("DHCP_OFFER");
    offer->addPar("assignedIP");
    offer->addPar("gatewayIP");
    offer->addPar("clientPort");

    // Assign IP in 192.168.10.x range and increment for next client
    std::string assignedIP = "192.168.10." + std::to_string(nextIP);
    std::string gatewayIP = "192.168.10.100"; // Legitimate gateway
    offer->par("assignedIP").setStringValue(assignedIP.c_str());
    offer->par("gatewayIP").setStringValue(gatewayIP.c_str());

    // Extract client port from the router
    int clientPort = 1; // Default to client1
    if (msg->hasPar("clientPort"))
    {
        clientPort = msg->par("clientPort").longValue();
    }
    offer->par("clientPort").setLongValue(clientPort);

    EV << "LEGITIMATE DHCP: Preparing to send offer with IP: " << assignedIP << " Gateway: " << gatewayIP << endl;

    // Introduce a delay here (for example, 1 to 2 seconds)
    scheduleAt(simTime() + uniform(1, 2), offer);  // Adds a random delay between 1 and 2 seconds

    // Track that we offered this IP
    offeredIPs[assignedIP] = true;
    nextIP++; // Increment for next client

    delete msg;
}

void DHCPServer::processDHCPRequest(cMessage *msg)
{
    EV << "Processing DHCP Request from client\n";

    // Extract the requested IP from the request
    std::string requestedIP;
    if (msg->hasPar("requestedIP"))
    {
        requestedIP = msg->par("requestedIP").stringValue();
    }
    else
    {
        requestedIP = "192.168.10.1"; // Fallback
    }

    // Only respond if we offered this IP (we lost the race to rogue server)
    if (requestedIP.find("192.168.10.") == 0 && offeredIPs.find(requestedIP) != offeredIPs.end())
    {
        cMessage *ack = new cMessage("DHCP_ACK");
        ack->addPar("assignedIP");
        ack->addPar("gatewayIP");
        ack->addPar("clientPort");

        ack->par("assignedIP").setStringValue(requestedIP.c_str());
        ack->par("gatewayIP").setStringValue("192.168.10.100"); // Legitimate gateway

        // Extract client port from the request message
        int clientPort = 1; // Default to client1
        if (msg->hasPar("clientPort"))
        {
            clientPort = msg->par("clientPort").longValue();
        }
        ack->par("clientPort").setLongValue(clientPort);

        // Send acknowledgment back to router (but rogue server will likely respond first)
        send(ack, "port$o");
        EV << "LEGITIMATE DHCP: Sent ACK with IP: " << requestedIP << " Gateway: 192.168.10.100 (may be too late - rogue won race)" << endl;
    }
    else
    {
        // Ignore requests for IPs we didn't offer (rogue server won this client)
        EV << "LEGITIMATE DHCP: Ignoring request for IP we didn't offer: " << requestedIP << " (rogue server won this client)" << endl;
    }
    delete msg;
}

void DHCPServer::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "DHCP Server\nNext IP: 192.168.10.%d\nGateway: 192.168.10.100", nextIP);
    getDisplayString().setTagArg("t", 0, buf);
}
