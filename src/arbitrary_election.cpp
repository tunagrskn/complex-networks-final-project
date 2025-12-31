#include "arbitrary_election.h"

Define_Module(ArbitraryElection);

ArbitraryElection::ArbitraryElection()
{
    L = -1;
    round = 0;
    diameter = 0;
    messagesReceived = 0;
    messagesSent = 0;
    isLeader = false;
    roundTimer = nullptr;
}

ArbitraryElection::~ArbitraryElection()
{
    cancelAndDelete(roundTimer);
}

void ArbitraryElection::initialize()
{
    TSNNode::initialize();
    
    // Initialize algorithm parameters
    L = nodeId;  // Initially, each node considers itself as leader
    round = 0;
    diameter = par("diameter");
    isLeader = false;
    
    EV << "Node " << nodeId << " initialized with L=" << L << ", diameter=" << diameter << "\n";
    
    // Schedule first round
    roundTimer = new cMessage("roundTimer");
    scheduleAt(simTime() + par("startDelay").doubleValue(), roundTimer);
}

void ArbitraryElection::handleMessage(cMessage *msg)
{
    if (msg == roundTimer) {
        // Start a new round
        startRound();
    }
    else if (dynamic_cast<LeaderMsg*>(msg)) {
        // Receive leader value from neighbor
        LeaderMsg *lmsg = check_and_cast<LeaderMsg*>(msg);
        
        if (lmsg->getRoundNum() == round) {
            int neighborId = lmsg->getSenderId();
            int neighborL = lmsg->getLeaderValue();
            
            receivedL[neighborId] = neighborL;
            messagesReceived++;
            
            EV << "Node " << nodeId << " received L=" << neighborL 
               << " from node " << neighborId << " (round " << round << ")\n";
            
            // Check if we received from all neighbors
            if (receivedL.size() == neighbors.size()) {
                processRound();
            }
        }
        
        delete msg;
    }
    else {
        delete msg;
    }
}

void ArbitraryElection::startRound()
{
    if (round >= diameter) {
        completeElection();
        return;
    }
    
    round++;
    receivedL.clear();
    messagesReceived = 0;
    
    EV << "Node " << nodeId << " starting round " << round << " with L=" << L << "\n";
    
    // Send L(i) to all neighbors
    for (int neighId : neighbors) {
        LeaderMsg *lmsg = new LeaderMsg();
        lmsg->setSenderId(nodeId);
        lmsg->setLeaderValue(L);
        lmsg->setRoundNum(round);
        
        int gateIndex = getNeighborGateIndex(neighId);
        if (gateIndex >= 0) {
            send(lmsg, "port$o", gateIndex);
            messagesSent++;
        } else {
            delete lmsg;
        }
    }
    
    emit(messagesSentSignal, messagesSent);
    
    // If this node has no neighbors, complete election
    if (neighbors.empty()) {
        completeElection();
    }
}

void ArbitraryElection::processRound()
{
    // Update L(i) to max{L(i) ∪ L(j): j ∈ N(i)}
    int oldL = L;
    
    for (const auto& pair : receivedL) {
        if (pair.second > L) {
            L = pair.second;
        }
    }
    
    EV << "Node " << nodeId << " updated L from " << oldL << " to " << L 
       << " (round " << round << ")\n";
    
    emit(roundsCompletedSignal, round);
    
    // Schedule next round if not finished
    if (round < diameter) {
        if (roundTimer->isScheduled()) {
            cancelEvent(roundTimer);
        }
        scheduleAt(simTime() + par("roundDelay").doubleValue(), roundTimer);
    } else {
        completeElection();
    }
}

void ArbitraryElection::completeElection()
{
    isLeader = (L == nodeId);
    
    EV << "Node " << nodeId << " completed election: L=" << L 
       << (isLeader ? " (I AM THE LEADER)" : "") << "\n";
    
    if (isLeader) {
        emit(leaderElectedSignal, nodeId);
        
        // Display prominent message
        bubble("I am the Grand Master!");
        
        // Change display to highlight leader
        getDisplayString().setTagArg("i", 1, "gold");
        getDisplayString().setTagArg("i", 2, "40");
    }
}

void ArbitraryElection::finish()
{
    // Record statistics
    recordScalar("finalLeader", L);
    recordScalar("isLeader", isLeader ? 1 : 0);
    recordScalar("totalMessagesSent", messagesSent);
    recordScalar("totalRounds", round);
    
    EV << "Node " << nodeId << " statistics:\n";
    EV << "  Final Leader: " << L << "\n";
    EV << "  Is Leader: " << (isLeader ? "YES" : "NO") << "\n";
    EV << "  Messages Sent: " << messagesSent << "\n";
    EV << "  Rounds Completed: " << round << "\n";
}
