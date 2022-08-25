

package rdp.simulations.SimpleSimulation;

@namespace(inet);
@namespace(ndp);


import rdp.node.StandardHostRdp;
import ecmp.networklayer.configurator.ipv4.Ipv4NetworkConfiguratorEcmp;
import inet.node.inet.Router;
import rdp.base.RouterRdp;
import ned.DatarateChannel;
import ned.IBidirectionalChannel;

network simplenetwork2
{
    parameters:
        @display("bgb=512,395");
        int numberOfClients = default(2);
        int numberOfNormalFlows = default(1);
        int numberOfLongerFlows = default(0);
        int numberOfRouters = default(1);
    types:
        channel ethernetline extends DatarateChannel
        {
            delay = 25ms;
            datarate = 10Mbps;
        }
    submodules:
        client[numberOfClients]: StandardHostRdp {
            @display("p=162,225,m,n,$numberOfClients,150");
        }
        configurator: Ipv4NetworkConfiguratorEcmp {
            @display("p=450,350");
        }
        server: StandardHostRdp {
            @display("p=464,117");
        }
        router: RouterRdp {
            @display("p=183,50,m,n,$numberOfRouters,150");
        }
    connections:
        client[0].pppg++ <--> ethernetline <--> router.pppg++;
        client[1].pppg++ <--> ethernetline <--> router.pppg++;
        router.pppg++ <--> ethernetline <--> server.pppg++;
}
