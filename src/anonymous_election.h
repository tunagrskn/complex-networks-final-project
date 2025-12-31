#ifndef __ANONYMOUSELECTION_H_
#define __ANONYMOUSELECTION_H_

#include "tsn_node.h"
#include "messages_m.h"
#include <map>

/**
 * Leader Election Algorithm for Anonymous Networks (Section 11.2.4)
 *
 * Randomized algorithm for completely connected networks.
 * Processes randomly choose bits (0 or 1). Those who choose 1 advance to next round.
 * If exactly one process chooses 1, it becomes the leader.
 * Works without using process identifiers (symmetry breaking through randomization).
 */
class AnonymousElection : public TSNNode
{
private:
    enum State { ACTIVE, PASSIVE, LEADER };

    State state;           // Current state of the node
    int bit;               // Random bit chosen in current round
    int round;             // Current round number
    int messagesSent;      // Total messages sent
    int messagesReceived;  // Messages received in current round

    std::map<int, int> receivedBits; // Bits received from active neighbors
    std::set<int> activeNodes;       // Set of currently active node IDs

    cMessage* roundTimer;  // Self-message for synchronization

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;

    void startRound();
    void processRound();
    void becomeLeader();
    void becomePassive();

public:
    AnonymousElection();
    virtual ~AnonymousElection();
};

#endif
