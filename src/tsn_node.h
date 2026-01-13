#ifndef __TSNNODE_H_
#define __TSNNODE_H_

#include <omnetpp.h>
#include <vector>
#include <set>

using namespace omnetpp;

/**
 * Base class for TSN (Time-Sensitive Networking) nodes
 * implementing Grand Master election algorithms
 */
class TSNNode : public cSimpleModule
{
protected:
    int nodeId;              // Unique node identifier
    int numNodes;            // Total number of nodes in the network
    std::set<int> neighbors; // Set of neighbor node IDs

    // Statistics signals for OMNeT++ result collection
    simsignal_t leaderElectedSignal;   // Emitted when a node becomes the leader
    simsignal_t messagesSentSignal;    // Emitted when messages are sent
    simsignal_t roundsCompletedSignal; // Emitted when a round completes

    // Initialization methods
    virtual void initialize() override;         // Initialize node, discover neighbors, register signals
    virtual void handleMessage(cMessage* msg) override; // Handle incoming messages (to be overridden by subclasses)

    // Utility methods for network topology and communication
    virtual void discoverNeighbors();           // Scan connected gates to find neighbor node IDs
    virtual void broadcastToNeighbors(cMessage* msg, int excludeGateIndex = -1); // Send message copy to all neighbors, optionally excluding one gate
    virtual int getNeighborGateIndex(int neighborId);  // Get gate index for a specific neighbor ID

public:
    TSNNode();
    virtual ~TSNNode();
};

#endif
