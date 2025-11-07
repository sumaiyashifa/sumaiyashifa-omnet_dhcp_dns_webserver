package areanetwork.simulations;

simple DHCPServer
{
    parameters:
        @display("i=device/server;t=DHCP Server");
    gates:
        inout port;
}

simple RogueDHCPServer
{
    parameters:
        @display("i=device/server;is=r;t=Rogue DHCP");
    gates:
        inout port;
}

simple DnsServer
{
    parameters:
        @display("i=device/server;t=DNS Server");
    gates:
        inout port;
}

simple WebServer
{
    parameters:
        @display("i=device/server;t=Web Server");
    gates:
        inout port;
}

simple Router
{
    parameters:
        int queueCapacity = default(10);         // Maximum queue capacity
        double processingDelay @unit(s) = default(1ms); // Delay between processing
        @display("i=device/router;t=Router (FIFO)");
    gates:
        inout port[7]; // 0=DHCP, 1=DNS, 2-4=clients, 5=Web, 6=Rogue DHCP
}

simple Computer
{
    parameters:
        @display("i=device/pc;t=Client");
    gates:
        inout port;
}

channel SlowChannel extends ned.DatarateChannel
{
    parameters:
        delay = 10ms;
        datarate = 1Mbps;
}

network ComputerNetwork
{
    parameters:
        @display("bgb=800,500");
    submodules:
        dhcpServer: DHCPServer {
            @display("p=225,88;t=DHCP Server");
        }
        rogueDhcp: RogueDHCPServer {
            @display("p=118,172;t=Rogue DHCP");
        }
        dnsServer: DnsServer {
            @display("p=584,105;t=DNS Server");
        }
        webServer: WebServer {
            @display("p=710,184;t=Web Server");
        }
        router: Router {
            @display("p=400,250;t=Router (FIFO)");
        }
        client1: Computer {
            @display("p=150,400;t=Client1");
        }
        client2: Computer {
            @display("p=400,400;t=Client2");
        }
        client3: Computer {
            @display("p=650,400;t=Client3");
        }

    connections:
        dhcpServer.port <--> router.port[0];
        rogueDhcp.port <--> router.port[6];
        dnsServer.port <--> router.port[1];
        webServer.port <--> SlowChannel <--> router.port[5];
        router.port[2] <--> client1.port;
        router.port[3] <--> client2.port;
        router.port[4] <--> client3.port;
}
