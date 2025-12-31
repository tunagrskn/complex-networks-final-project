#include "tsn_node.h"

Define_Module(TSNNode);

TSNNode::TSNNode()
{
    nodeId = -1;
    numNodes = 0;
}

TSNNode::~TSNNode()
{
}

void TSNNode::initialize()
{
    nodeId = par("nodeId");
    numNodes = getParentModule()->par("numNodes");
    
    // Register signals for statistics collection
    leaderElectedSignal = registerSignal("leaderElected");
    messagesSentSignal = registerSignal("messagesSent");
    roundsCompletedSignal = registerSignal("roundsCompleted");
    
    // Discover neighbors
    discoverNeighbors();
}

void TSNNode::handleMessage(cMessage *msg)
{
    // To be overridden by derived classes
    delete msg;
}

void TSNNode::discoverNeighbors()
{
    neighbors.clear();
    
    // Iterate through all gates to find connected neighbors
    for (int i = 0; i < gateSize("port"); i++) {
        cGate *outGate = gate("port$o", i);
        if (outGate->isConnected()) {
            cGate *connectedGate = outGate->getPathEndGate();
            cModule *neighborModule = connectedGate->getOwnerModule();
            if (neighborModule != nullptr) {
                int neighId = neighborModule->par("nodeId");
                neighbors.insert(neighId);
            }
        }
    }
    
    EV << "Node " << nodeId << " discovered " << neighbors.size() << " neighbors: ";
    for (int n : neighbors) {
        EV << n << " ";
    }
    EV << "\n";
}

void TSNNode::broadcastToNeighbors(cMessage *msg)
{
    for (int neighId : neighbors) {
        int gateIndex = getNeighborGateIndex(neighId);
        if (gateIndex >= 0) {
            cMessage *copy = msg->dup();
            send(copy, "port$o", gateIndex);
        }
    }
    delete msg;
}

int TSNNode::getNeighborGateIndex(int neighborId)
{
    for (int i = 0; i < gateSize("port"); i++) {
        cGate *outGate = gate("port$o", i);
        if (outGate->isConnected()) {
            cGate *connectedGate = outGate->getPathEndGate();
            cModule *neighborModule = connectedGate->getOwnerModule();
            if (neighborModule != nullptr && neighborModule->par("nodeId").intValue() == neighborId) {
                return i;
            }
        }
    }
    return -1;
}
