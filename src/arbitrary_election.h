#ifndef __ARBITRARYELECTION_H_
#define __ARBITRARYELECTION_H_

#include "election_node.h"
#include "messages_m.h"
#include <map>

/**
 * Leader Election Algorithm for Arbitrary Networks (Section 11.2.3)
 *
 * Uses flooding-based approach where each node maintains L(i) (leader candidate).
 * Initially L(i) = i. In each round, nodes exchange L values with neighbors
 * and update to the maximum. Algorithm terminates after D rounds (network diameter).
 */
class ArbitraryElection : public ElectionNode
{
private:
    int L;              // Current leader candidate (maximum ID seen so far)
    int round;          // Current round number (algorithm runs for D rounds)
    int diameter;       // Network diameter D (number of rounds required for convergence)
    int messagesReceived; // Number of messages received in current round
    int messagesSent;     // Total number of messages sent so far
    bool isLeader;        // True if this node's ID equals final L value

    std::map<int, int> receivedL; // Maps neighbor ID to their L value in current round

    cMessage* roundTimer; // Self-message timer for round synchronization

protected:
    virtual void initialize() override;      // Initialize module, set L = nodeId, schedule first round
    virtual void handleMessage(cMessage* msg) override; // Process incoming L values and timer events
    virtual void finish() override;         // Record statistics at end of simulation

    void startRound();      // Start a new round: broadcast current L value to all neighbors
    void processRound();    // Update L to max{L, L(j) for all neighbors j}, schedule next round
    void completeElection(); // Finalize election after D rounds, determine if this node is leader

public:
    ArbitraryElection();
    virtual ~ArbitraryElection();
};

#endif
