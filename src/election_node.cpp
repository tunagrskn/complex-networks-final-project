#include "election_node.h"

Define_Module(ElectionNode);

/**
 * Constructor: Initialize member variables to default values
 */
ElectionNode::ElectionNode()
{
    nodeId = -1;
    numNodes = 0;
}

/**
 * Destructor: Clean up (nothing to clean in base class)
 */
ElectionNode::~ElectionNode()
{
}

/**
 * Initialize the election node:
 * - Read node parameters (nodeId, numNodes)
 * - Register statistical signals
 * - Discover connected neighbors
 */
void ElectionNode::initialize()
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

/**
 * Handle incoming messages (to be overridden by derived classes)
 * Default implementation just deletes the message
 */
void ElectionNode::handleMessage(cMessage *msg)
{
    // To be overridden by derived classes
    delete msg;
}

/**
 * Discover neighbors by scanning all connected gates:
 * - Iterate through all port gates
 * - For each connected gate, extract neighbor's nodeId
 * - Add to neighbors set
 */
void ElectionNode::discoverNeighbors()
{
    neighbors.clear();
    
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
    
    EV_DEBUG << "[TOPOLOGY] Node " << nodeId << " | Neighbors: " << neighbors.size() << "\n";
}

/**
 * Broadcast a message to all neighbors:
 * - For each neighbor, duplicate the message and send.
 * - Optionally exclude the gate the message originally arrived on.
 * - The caller retains ownership of the original message.
 */
void ElectionNode::broadcastToNeighbors(cMessage *msg, int excludeGateIndex)
{
    for (int i = 0; i < gateSize("port"); i++) {
        if (i == excludeGateIndex) {
            continue;
        }
        cGate *outGate = gate("port$o", i);
        if (outGate->isConnected()) {
            cMessage *copy = msg->dup();
            send(copy, "port$o", i);
        }
    }
}

/**
 * Get the gate index for a specific neighbor:
 * - Iterate through all port gates
 * - Find the gate connected to the neighbor with given ID
 * - Return gate index, or -1 if not found
 */
int ElectionNode::getNeighborGateIndex(int neighborId)
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
