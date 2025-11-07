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

#ifndef __AREANETWORK_COMPUTER_H_
#define __AREANETWORK_COMPUTER_H_

#include <omnetpp.h>

using namespace omnetpp;

class Computer : public cSimpleModule
{
private:
    cMessage *dhcpTimer = nullptr;
    cMessage *packetTimer = nullptr;
    bool hasIP = false;
    bool waitingForDHCPResponse = false; // Prevent multiple DHCP requests
    int retryCount = 0;
    std::string assignedIP = "";
    std::string gatewayIP = "";
    int packetCounter = 0;

    // DNS and Web Access
    std::string resolvedWebServerIP = "";
    bool waitingForDNS = false;
    int pendingQueryId = 0;
    int webAccessCount = 0; // Track number of web access attempts
    int maxWebAccess = 2;   // Maximum number of web access attempts per client

    // Statistics
    cOutVector dhcpTimeStats;
    cOutVector packetsSent;
    cOutVector packetsReceived;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual void startDHCP();
    virtual void sendDHCPDiscover();
    virtual void startWebAccess();
    virtual void sendDnsQuery(const std::string &domain);
    virtual void sendHttpRequest(const std::string &serverIP);
};

#endif