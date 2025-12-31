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

    // Statistics
    simsignal_t leaderElectedSignal;
    simsignal_t messagesSentSignal;
    simsignal_t roundsCompletedSignal;

    // Initialization
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;

    // Utility methods
    virtual void discoverNeighbors();
    virtual void broadcastToNeighbors(cMessage* msg);
    virtual int getNeighborGateIndex(int neighborId);

public:
    TSNNode();
    virtual ~TSNNode();
};

#endif
