//
// Network topology definitions for TSN Leader Election Algorithms
//

//
// Base module for TSN nodes
//
simple TSNNode
{
    parameters:
        int nodeId;
        @display("i=device/device");
        @signal[leaderElected](type=long);
        @signal[messagesSent](type=long);
        @signal[roundsCompleted](type=long);
        @statistic[leaderElected](record=vector,last);
        @statistic[messagesSent](record=vector,sum);
        @statistic[roundsCompleted](record=vector,last);
        
    gates:
        inout port[];
}

//
// Leader Election for Arbitrary Networks (flooding-based)
//
simple ArbitraryElection extends TSNNode
{
    parameters:
        @class(ArbitraryElection);
        int diameter;           // Network diameter D
        double startDelay @unit(s) = default(0.1s);
        double roundDelay @unit(s) = default(0.5s);
}

//
// Leader Election for Anonymous Networks (randomized)
//
simple AnonymousElection extends TSNNode
{
    parameters:
        @class(AnonymousElection);
        double startDelay @unit(s) = default(0.1s);
        double roundDelay @unit(s) = default(0.5s);
}

//
// Network topology: Ring (suitable for arbitrary network algorithm)
//
network RingNetwork
{
    parameters:
        int numNodes = default(8);
        int diameter = default(4); // For ring: diameter ≈ n/2
        
    submodules:
        node[numNodes]: ArbitraryElection {
            parameters:
                nodeId = index;
                diameter = parent.diameter;
                @display("p=,,ring");
        }
        
    connections:
        // Create ring topology
        for i=0..numNodes-2 {
            node[i].port++ <--> {delay = 10ms;} <--> node[i+1].port++;
        }
        // Close the ring
        node[numNodes-1].port++ <--> {delay = 10ms;} <--> node[0].port++;
}

//
// Network topology: Mesh (arbitrary network)
//
network MeshNetwork
{
    parameters:
        int numNodes = default(9);
        int gridSize = default(3); // 3x3 grid
        int diameter = default(4);
        
    submodules:
        node[numNodes]: ArbitraryElection {
            parameters:
                nodeId = index;
                diameter = parent.diameter;
                @display("p=,,matrix,$gridSize");
        }
        
    connections allowunconnected:
        // Create 2D mesh topology
        for i=0..numNodes-1, for j=0..numNodes-1, if i<j {
            // Connect neighbors in grid
            node[i].port++ <--> {delay = 10ms;} <--> node[j].port++ 
                if (j == i+1 && (i+1) % gridSize != 0) ||  // right neighbor
                   (j == i+gridSize);                       // down neighbor
        }
}

//
// Network topology: Star (arbitrary network)
//
network StarNetwork
{
    parameters:
        int numNodes = default(7);
        int diameter = default(2); // Star diameter is 2
        
    submodules:
        node[numNodes]: ArbitraryElection {
            parameters:
                nodeId = index;
                diameter = parent.diameter;
                @display("p=,,ring");
        }
        
    connections:
        // Node 0 is the center, connect all others to it
        for i=1..numNodes-1 {
            node[0].port++ <--> {delay = 10ms;} <--> node[i].port++;
        }
}

//
// Network topology: Fully Connected (for anonymous algorithm)
//
network FullyConnectedNetwork
{
    parameters:
        int numNodes = default(6);
        
    submodules:
        node[numNodes]: AnonymousElection {
            parameters:
                nodeId = index;
                @display("p=,,ring");
        }
        
    connections:
        // Connect every node to every other node
        for i=0..numNodes-1, for j=i+1..numNodes-1 {
            node[i].port++ <--> {delay = 10ms;} <--> node[j].port++;
        }
}

//
// Network topology: Random (for arbitrary network algorithm)
//
network RandomNetwork
{
    parameters:
        int numNodes = default(10);
        int diameter = default(5);
        double connectivity = default(0.3); // 30% connection probability
        
    submodules:
        node[numNodes]: ArbitraryElection {
            parameters:
                nodeId = index;
                diameter = parent.diameter;
                @display("p=,,circle");
        }
        
    connections allowunconnected:
        // Create random connections
        for i=0..numNodes-1, for j=i+1..numNodes-1 {
            node[i].port++ <--> {delay = 10ms;} <--> node[j].port++ 
                if uniform(0,1) < connectivity;
        }
}
