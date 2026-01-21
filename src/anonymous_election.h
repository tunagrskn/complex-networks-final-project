#ifndef __ANONYMOUSELECTION_H_
#define __ANONYMOUSELECTION_H_

#include "election_node.h"
#include "messages_m.h"
#include <map>
#include <list>
#include <random>
#include <chrono>

/**
 * Leader Election Algorithm for Anonymous Networks (Section 11.2.4)
 *
 * Randomized algorithm for completely connected networks.
 * Processes randomly choose bits (0 or 1). Those who choose 1 advance to next round.
 * If exactly one process chooses 1, it becomes the leader.
 * Works without using process identifiers (symmetry breaking through randomization).
 */
class AnonymousElection : public ElectionNode
{
private:
    /**
     * Node state in the anonymous election algorithm
     * ACTIVE: Actively participating in the election
     * PASSIVE: Eliminated from the election, waiting for leader announcement
     * LEADER: Successfully elected as the Grand Master
     */
    enum State { ACTIVE, PASSIVE, LEADER };

    State state;           // Current state of the node (ACTIVE/PASSIVE/LEADER)
    int bit;               // Random bit (0 or 1) chosen in current round
    int round;             // Current round number
    int messagesSent;      // Total number of messages sent so far
    int messagesReceived;  // Number of messages received in current round

    std::map<int, int> receivedBits; // Maps neighbor ID to their chosen bit value
    std::set<int> activeNodes;       // IDs of nodes currently active in the election

    std::list<cMessage*> futureMessages; // Buffer for messages from future rounds
    cMessage* roundTimer;  // Self-message timer for round synchronization
    
    // True randomness using C++11 random
    std::mt19937 rng;      // Mersenne Twister random number generator
    std::uniform_int_distribution<int> bitDist;  // Distribution for 0 or 1

protected:
    virtual void initialize() override;       // Initialize module and schedule first round
    virtual void handleMessage(cMessage* msg) override; // Process incoming messages and timer events
    virtual void finish() override;          // Record statistics at end of simulation

    void startRound();      // Start a new round: choose random bit and broadcast to neighbors
    void processRound();    // Analyze round results and update state (continue/become leader/become passive)
    void becomeLeader();    // Transition to LEADER state and announce to network
    void becomePassive();   // Transition to PASSIVE state (eliminated from election)

public:
    AnonymousElection();
    virtual ~AnonymousElection();
};

#endif
