#ifndef __ARBITRARYELECTION_H_
#define __ARBITRARYELECTION_H_

#include "tsn_node.h"
#include "messages_m.h"
#include <map>

/**
 * Leader Election Algorithm for Arbitrary Networks (Section 11.2.3)
 *
 * Uses flooding-based approach where each node maintains L(i) (leader candidate).
 * Initially L(i) = i. In each round, nodes exchange L values with neighbors
 * and update to the maximum. Algorithm terminates after D rounds (network diameter).
 */
class ArbitraryElection : public TSNNode
{
private:
    int L;              // Current leader candidate (maximum ID seen)
    int round;          // Current round number
    int diameter;       // Network diameter D
    int messagesReceived; // Messages received in current round
    int messagesSent;     // Total messages sent
    bool isLeader;        // Flag indicating if this node is the leader

    std::map<int, int> receivedL; // L values received from neighbors in current round

    cMessage* roundTimer; // Self-message for synchronization

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;

    void startRound();
    void processRound();
    void completeElection();

public:
    ArbitraryElection();
    virtual ~ArbitraryElection();
};

#endif
