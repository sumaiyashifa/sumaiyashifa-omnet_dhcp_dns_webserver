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

#include "include/DnsServer.h"
#include "generated/DnsMessage_m.h"

Define_Module(DnsServer);

void DnsServer::initialize()
{
    EV << "DNS Server initialized\n";
    setupDnsTable();
}

void DnsServer::setupDnsTable()
{
    // Add DNS entries - simplified configuration
    dnsTable["google.com"] = "192.168.10.21";      // Local web server IP (the only external server)
    dnsTable["webserver.local"] = "192.168.10.21"; // Local web server IP
    dnsTable["client1.local"] = "192.168.1.10";
    dnsTable["client2.local"] = "192.168.1.11";
    dnsTable["client3.local"] = "192.168.1.12";
    dnsTable["dhcp.local"] = "192.168.1.1";
    dnsTable["dns.local"] = "192.168.10.20"; // DNS server IP

    EV << "DNS Table initialized with " << dnsTable.size() << " entries\n";
}

void DnsServer::handleMessage(cMessage *msg)
{
    DnsMessage *dnsMsg = check_and_cast<DnsMessage *>(msg);

    EV << "DNS Server received: " << dnsMsg->getType()
       << " for domain: " << dnsMsg->getDomainName() << endl;

    if (strcmp(dnsMsg->getType(), "QUERY") == 0)
    {
        std::string domain = dnsMsg->getDomainName();
        std::string ip = resolveDomain(domain);

        // Create DNS response
        DnsMessage *response = new DnsMessage("DNS_RESPONSE");
        response->setType("RESPONSE");
        response->setDomainName(domain.c_str());
        response->setIpAddress(ip.c_str());
        response->setQueryId(dnsMsg->getQueryId());

        // Preserve client port information for routing
        if (dnsMsg->hasPar("clientPort"))
        {
            response->addPar("clientPort");
            response->par("clientPort").setLongValue(dnsMsg->par("clientPort").longValue());
        }

        EV << "DNS Response: " << domain << " -> " << ip << endl;

        // Send response back to the same gate
        send(response, "port$o");

        queriesProcessed++;
    }

    delete msg;
}

std::string DnsServer::resolveDomain(const std::string &domain)
{
    auto it = dnsTable.find(domain);
    if (it != dnsTable.end())
    {
        return it->second;
    }
    else
    {
        return "0.0.0.0"; // Not found
    }
}

void DnsServer::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "DNS Server\nQueries: %d", queriesProcessed);
    getDisplayString().setTagArg("t", 0, buf);
}
