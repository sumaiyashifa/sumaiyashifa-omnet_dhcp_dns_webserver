//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "include/Computer.h"
#include "generated/DataPacket_m.h"
#include "generated/DnsMessage_m.h"

Define_Module(Computer);

void Computer::initialize()
{
    EV << "Computer " << getName() << " starting\n";
    dhcpTimer = new cMessage("DHCP_Timer");
    packetTimer = new cMessage("Packet_Timer");
    dhcpTimeStats.setName("DHCP Time");
    packetsSent.setName("Packets Sent");
    packetsReceived.setName("Packets Received");
    scheduleAt(simTime() + uniform(0, 2), dhcpTimer);
}

void Computer::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (msg == dhcpTimer)
        {
            startDHCP();
        }
        else if (msg == packetTimer)
        {
            startWebAccess();
        }
    }
    else
    {
        if (strcmp(msg->getName(), "DHCP_OFFER") == 0)
        {
            // Only process if we don't already have an IP and haven't already responded to an offer
            if (!hasIP && !waitingForDHCPResponse)
            {
                // Extract assigned IP and gateway from the offer
                if (msg->hasPar("assignedIP"))
                {
                    assignedIP = msg->par("assignedIP").stringValue();
                    if (msg->hasPar("gatewayIP"))
                    {
                        gatewayIP = msg->par("gatewayIP").stringValue();
                    }

                    EV << "Received DHCP Offer with IP: " << assignedIP << " Gateway: " << gatewayIP << "\n";

                    // Check if this is a rogue gateway (10.0.0.100)
                    if (gatewayIP == "10.0.0.100")
                    {
                        EV << "🚨 WARNING: Received offer with ROGUE GATEWAY: " << gatewayIP << " (should reject this!)" << "\n";
                    }
                    else
                    {
                        EV << "Received legitimate DHCP offer for IP: " << assignedIP << " Gateway: " << gatewayIP << "\n";
                    }

                    waitingForDHCPResponse = true;
                    EV << "Sending DHCP Request for IP: " << assignedIP << "\n";
                    cMessage *request = new cMessage("DHCP_REQUEST");
                    request->addPar("requestedIP");
                    request->par("requestedIP").setStringValue(assignedIP.c_str());
                    send(request, "port$o");
                }
            }
            else if (hasIP)
            {
                EV << "Already have IP, ignoring DHCP offer\n";
            }
            else
            {
                EV << "Already waiting for DHCP response, ignoring additional offer\n";
            }
        }
        else if (strcmp(msg->getName(), "DHCP_ACK") == 0)
        {
            // Only process if we don't already have an IP and we were waiting for a response
            if (!hasIP && waitingForDHCPResponse)
            {
                simtime_t dhcpTime = simTime() - dhcpTimer->getSendingTime();
                dhcpTimeStats.record(dhcpTime);

                // Extract IP and gateway from ACK message
                if (msg->hasPar("assignedIP"))
                {
                    assignedIP = msg->par("assignedIP").stringValue();
                }
                if (msg->hasPar("gatewayIP"))
                {
                    gatewayIP = msg->par("gatewayIP").stringValue();
                }

                // Check if we got a rogue gateway and show warning
                if (gatewayIP == "10.0.0.100")
                {
                    EV << "🚨 ATTACK SUCCESSFUL: ROGUE DHCP SERVER ASSIGNED MALICIOUS GATEWAY: " << gatewayIP << " - Client compromised!" << "\n";
                    EV << "   IP: " << assignedIP << " Gateway: " << gatewayIP << " (ALL TRAFFIC WILL GO TO ROGUE!)" << "\n";
                }
                else
                {
                    EV << "DHCP complete with legitimate IP: " << assignedIP << " Gateway: " << gatewayIP << "\n";
                }

                hasIP = true;
                waitingForDHCPResponse = false;
                packetCounter = 6;

                // Cancel any pending retry timers since we got an IP
                if (dhcpTimer && dhcpTimer->isScheduled())
                {
                    cancelEvent(dhcpTimer);
                }

                // Start web access after getting IP
                if (packetTimer->isScheduled()) // <-- added
                    cancelEvent(packetTimer);   // <-- added
                scheduleAt(simTime() + uniform(5, 10), packetTimer);
            }
            else if (hasIP)
            {
                EV << "Already have IP, ignoring DHCP ACK\n";
            }
            else
            {
                EV << "Not waiting for DHCP response, ignoring unexpected ACK\n";
            }
        }
        else if (strcmp(msg->getName(), "DATA_PACKET") == 0)
        {
            // Handle received data packet
            DataPacket *packet = check_and_cast<DataPacket *>(msg);
            EV << "Received data packet from " << packet->getSourceIP()
               << " to " << packet->getDestIP()
               << ": " << packet->getPayload() << endl;
            packetsReceived.record(1);
        }
        else if (strcmp(msg->getName(), "DNS_RESPONSE") == 0)
        {
            // Handle DNS response
            DnsMessage *dnsResp = check_and_cast<DnsMessage *>(msg);
            EV << "DNS Response: " << dnsResp->getDomainName()
               << " -> " << dnsResp->getIpAddress() << endl;

            // Debug output
            EV << "DEBUG: waitingForDNS=" << waitingForDNS
               << ", pendingQueryId=" << pendingQueryId
               << ", receivedQueryId=" << dnsResp->getQueryId() << endl;

            // Check if this is our pending query
            if (waitingForDNS && dnsResp->getQueryId() == pendingQueryId)
            {
                resolvedWebServerIP = dnsResp->getIpAddress();
                waitingForDNS = false;

                EV << "DNS resolved! Now sending HTTP request to " << resolvedWebServerIP << endl;

                // Send HTTP request to the resolved IP
                // Count the DNS response as a packet for display
                packetCounter++; // DNS response received
                sendHttpRequest(resolvedWebServerIP);

                // Schedule next web access
                if (packetTimer->isScheduled()) // <-- added
                    cancelEvent(packetTimer);   // <-- added
                scheduleAt(simTime() + uniform(10, 20), packetTimer);
            }
            else
            {
                EV << "DEBUG: DNS response not for us - waitingForDNS=" << waitingForDNS
                   << ", queryId mismatch: " << pendingQueryId << " vs " << dnsResp->getQueryId() << endl;
            }
        }
        else if (strcmp(msg->getName(), "HTTP_RESPONSE") == 0)
        {
            // Handle HTTP response from web server
            DataPacket *response = check_and_cast<DataPacket *>(msg);
            EV << "Received HTTP response from web server: " << response->getPayload() << endl;
            packetsReceived.record(1);
            // Count the HTTP response as a packet for display
            packetCounter++; // HTTP response received
        }
        delete msg;
    }
}

void Computer::startDHCP()
{
    if (!hasIP && retryCount < 3)
    {
        EV << "Sending DHCP Discover\n";
        sendDHCPDiscover();
        retryCount++;

        if (!hasIP && retryCount < 3)
        {
            scheduleAt(simTime() + 2, dhcpTimer);
        }
    }
    else if (hasIP)
    {
        // DHCP complete - do nothing
        EV << "DHCP complete, no more actions needed\n";
    }
}

void Computer::sendDHCPDiscover()
{
    waitingForDHCPResponse = false; // Reset flag when starting new discovery
    cMessage *discover = new cMessage("DHCP_DISCOVER");
    send(discover, "port$o");
}

void Computer::startWebAccess()
{
    if (hasIP && !waitingForDNS && webAccessCount < maxWebAccess)
    {
        webAccessCount++; // Increment counter

        // Special behavior for client2 - request google.com (local web server)
        std::string domainToQuery;
        if (getName() == "client2")
        {
            domainToQuery = "google.com";
            EV << "Starting web access #" << webAccessCount << " - sending DNS query for google.com (local web server)" << endl;
        }
        else
        {
            domainToQuery = "google.com";
            EV << "Starting web access #" << webAccessCount << " - sending DNS query for google.com" << endl;
        }

        // Send DNS query to resolve web server domain
        sendDnsQuery(domainToQuery);
        waitingForDNS = true;
        pendingQueryId = packetCounter; // Use current value
        packetCounter++;                // Increment after setting pendingQueryId

        EV << "DEBUG: Sent DNS query with ID=" << pendingQueryId << ", waitingForDNS=" << waitingForDNS << endl;

        // Special behavior for client3 (attacker) - send packets faster
        if (getName() == "client3")
        {
            // Attack mode: send packets every 0.5 seconds (DDoS simulation)
            if (packetTimer->isScheduled()) // <-- added
                cancelEvent(packetTimer);   // <-- added
            scheduleAt(simTime() + 0.5, packetTimer);
            EV << "🔥 ATTACKER MODE: Rapid web requests!" << endl;
        }
        else
        {
            // Normal mode: send packets every 10-20 seconds
            if (packetTimer->isScheduled()) // <-- added
                cancelEvent(packetTimer);   // <-- added
            scheduleAt(simTime() + uniform(10, 20), packetTimer);
        }
    }
    else if (webAccessCount >= maxWebAccess)
    {
        EV << "Maximum web access attempts (" << maxWebAccess << ") reached for " << getName() << ". Stopping web access." << endl;
    }
}

void Computer::sendHttpRequest(const std::string &serverIP)
{
    EV << "DEBUG: sendHttpRequest called with serverIP=" << serverIP << ", hasIP=" << hasIP << endl;

    if (hasIP)
    {
        DataPacket *packet = new DataPacket("HTTP_REQUEST");
        packet->setSourceIP(assignedIP.c_str());
        packet->setDestIP(serverIP.c_str());
        packet->setPayload("GET /index.html HTTP/1.1");
        packet->setPacketId(packetCounter++);

        EV << "Sending HTTP request from " << assignedIP
           << " to web server " << serverIP << endl;

        send(packet, "port$o");
        packetsSent.record(1);

        EV << "DEBUG: HTTP request sent successfully" << endl;
    }
    else
    {
        EV << "DEBUG: Cannot send HTTP request - no IP assigned" << endl;
    }
}

void Computer::sendDnsQuery(const std::string &domain)
{
    if (hasIP)
    {
        DnsMessage *query = new DnsMessage("DNS_QUERY");
        query->setType("QUERY");
        query->setDomainName(domain.c_str());
        query->setQueryId(packetCounter); // Use current value, don't increment here

        EV << "Sending DNS query for: " << domain << " with ID=" << packetCounter << endl;
        send(query, "port$o");
    }
}

void Computer::refreshDisplay() const
{
    char buf[150];
    if (hasIP)
    {
        // Check if this client has rogue gateway (indicates compromise)
        bool isCompromised = (gatewayIP == "10.0.0.100");

        if (isCompromised)
        {
            sprintf(buf, "🚨 COMPROMISED\nIP: %s\nGateway: %s\nPkts: %d", assignedIP.c_str(), gatewayIP.c_str(), packetCounter);
            getDisplayString().setTagArg("i", 1, "red");
        }
        else if (getName() == "client3")
        {
            sprintf(buf, "🔥 ATTACKER\nIP: %s\nGateway: %s\nPkts: %d", assignedIP.c_str(), gatewayIP.c_str(), packetCounter);
            getDisplayString().setTagArg("i", 1, "red");
        }
        else
        {
            sprintf(buf, "IP: %s\nGateway: %s\nPkts: %d", assignedIP.c_str(), gatewayIP.c_str(), packetCounter);
            getDisplayString().setTagArg("i", 1, "green");
        }
    }
    else
    {
        sprintf(buf, "No IP\nRetry: %d", retryCount);
        getDisplayString().setTagArg("i", 1, "yellow");
    }
    getDisplayString().setTagArg("t", 0, buf);
}
