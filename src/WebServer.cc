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

#include "include/WebServer.h"
#include "generated/DataPacket_m.h"

Define_Module(WebServer);

void WebServer::initialize()
{
    EV << "Web Server initialized\n";
}

void WebServer::handleMessage(cMessage *msg)
{
    EV << "DEBUG: WebServer received message: " << msg->getName() << endl;

    if (strcmp(msg->getName(), "HTTP_REQUEST") == 0)
    {
        DataPacket *request = check_and_cast<DataPacket *>(msg);

        EV << "Web Server received HTTP request from " << request->getSourceIP()
           << " for: " << request->getPayload() << endl;

        // Create HTTP response
        DataPacket *response = new DataPacket("HTTP_RESPONSE");
        response->setSourceIP("192.168.10.21"); // Local web server IP
        response->setDestIP(request->getSourceIP());
        response->setPayload("HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>Welcome to Google!</h1><p>This is a local Google server simulation.</p><p>You are accessing google.com from our local network.</p></body></html>");
        response->setPacketId(request->getPacketId());

        // Preserve client port information for routing
        if (request->hasPar("clientPort"))
        {
            response->addPar("clientPort");
            response->par("clientPort").setLongValue(request->par("clientPort").longValue());
        }

        EV << "Sending HTTP response to " << request->getDestIP() << endl;

        // Send response back to the same gate
        send(response, "port$o");

        requestsProcessed++;
        EV << "DEBUG: HTTP response sent, requestsProcessed=" << requestsProcessed << endl;
        delete request;
    }
    else
    {
        EV << "Web Server received unknown message: " << msg->getName() << endl;
        delete msg;
    }
}

void WebServer::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "Web Server\nRequests: %d", requestsProcessed);
    getDisplayString().setTagArg("t", 0, buf);
}
