

package rdp.simulations.SimpleSimulation;

@namespace(inet);
@namespace(ndp);


import rdp.node.StandardHostRdp;
import ecmp.networklayer.configurator.ipv4.Ipv4NetworkConfiguratorEcmp;
import inet.node.inet.Router;
import rdp.base.RouterRdp;
import ned.DatarateChannel;
import ned.IBidirectionalChannel;

network simplenetwork
{
    parameters:
        @display("bgb=512,395");
        int numberOfClients = default(1);
        int numberOfServers = default(1);
        int numberOfNormalFlows = default(1);
        int numberOfLongerFlows = default(0);
        int numberOfRouters = default(1);
    types:
        channel ethernetline extends DatarateChannel
        {
            delay = 0.1us;
            datarate = 100Gbps;
        }
        channel bottleneckline extends DatarateChannel
        {
            delay = 50us;
            datarate = 10Gbps;
        }
    submodules:
        client[numberOfClients]: StandardHostRdp {
            @display("p=68,71,m,n,$numberOfClients,150");
        }
        configurator: Ipv4NetworkConfiguratorEcmp {
            @display("p=450,350");
        }
        server[numberOfServers]: StandardHostRdp {
            @display("p=431,50,m,n,$numberOfServers,150");
        }
        router1: RouterRdp {
            @display("p=183,50,m,n,$numberOfRouters,150");
        }
        router2: RouterRdp {
            @display("p=300,50,m,n,$numberOfRouters,150");
        }
    connections:
        for i=0..sizeof(client)-1 {
            client[i].pppg++ <--> ethernetline <--> router1.pppg++;
        }

        for n=0..sizeof(server)-1 {
            server[n].pppg++ <--> ethernetline <--> router2.pppg++;
        }

        router1.pppg++ <--> bottleneckline <--> router2.pppg++;
}
