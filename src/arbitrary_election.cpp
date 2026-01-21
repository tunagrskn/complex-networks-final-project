#include "arbitrary_election.h"
#include "election_analyzer.h"

Define_Module(ArbitraryElection);

/**
 * Constructor: Initialize member variables to default values
 */
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

/**
 * Destructor: Clean up timer message
 */
ArbitraryElection::~ArbitraryElection()
{
    cancelAndDelete(roundTimer);
}

/**
 * Initialize the arbitrary network election algorithm:
 * - Set L(i) = i (each node initially considers itself as leader)
 * - Read network diameter parameter
 * - Schedule first round after startDelay
 */
void ArbitraryElection::initialize()
{
    ElectionNode::initialize();
    
    L = nodeId;
    round = 0;
    diameter = par("diameter");
    isLeader = false;
    
    EV_INFO << "[INIT] Node " << nodeId << " | L=" << L << " | Diameter=" << diameter << "\n";
    
    roundTimer = new cMessage("roundTimer");
    scheduleAt(simTime() + par("startDelay").doubleValue(), roundTimer);
}

/**
 * Handle incoming messages:
 * - roundTimer: Trigger to start a new round
 * - LeaderMsg: L value from neighbor in current round
 */
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
            
            EV_DEBUG << "[RECV] Node " << nodeId << " <- L=" << neighborL 
                     << " from Node " << neighborId << " (Round " << round << ")\n";
            
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

/**
 * Start a new round:
 * 1. Check if D rounds completed; if so, complete election
 * 2. Increment round counter and reset received messages
 * 3. Broadcast current L(i) value to all neighbors
 */
void ArbitraryElection::startRound()
{
    if (round >= diameter) {
        completeElection();
        return;
    }
    
    round++;
    receivedL.clear();
    messagesReceived = 0;
    
    EV_INFO << "[ROUND " << round << "] Node " << nodeId << " | L=" << L << "\n";
    
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

/**
 * Process round results:
 * - Update L(i) to max{L(i), L(j) for all neighbors j}
 * - Emit round completed signal
 * - Schedule next round if not finished (round < D)
 * - Otherwise, complete election
 */
void ArbitraryElection::processRound()
{
    int oldL = L;
    
    for (const auto& pair : receivedL) {
        if (pair.second > L) {
            L = pair.second;
        }
    }
    
    if (L != oldL) {
        EV_DEBUG << "[UPDATE] Node " << nodeId << " | L: " << oldL << " -> " << L << "\n";
    }
    
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

/**
 * Complete the election after D rounds:
 * - Check if L(i) == i (this node is the leader)
 * - If leader, emit signal and display visual indicators
 */
void ArbitraryElection::completeElection()
{
    isLeader = (L == nodeId);
    
    EV_INFO << "[ELECTION COMPLETE] Node " << nodeId << " | Leader: " << L 
            << (isLeader ? " (I AM THE LEADER)" : "") << "\n";
    
    if (isLeader) {
        emit(leaderElectedSignal, nodeId);
        
        // Report to analyzer
        cModule *analyzerModule = getParentModule()->getSubmodule("analyzer");
        if (analyzerModule) {
            ElectionAnalyzer *analyzer = check_and_cast<ElectionAnalyzer*>(analyzerModule);
            analyzer->reportLeaderElected(nodeId, round, messagesSent);
        }
        
        EV_WARN << "\n========================================\n"
                << "  LEADER ELECTED: Node " << nodeId << "\n"
                << "  Rounds: " << round << " | Messages: " << messagesSent << "\n"
                << "========================================\n\n";
        
        // Display prominent message
        bubble("I am the Grand Master!");
        
        // Change display to highlight leader
        getDisplayString().setTagArg("i", 1, "gold");
        getDisplayString().setTagArg("i", 2, "40");
    }
}

/**
 * Record statistics at end of simulation:
 * - Final L value (elected leader ID)
 * - Whether this node is the leader
 * - Total messages sent
 * - Total rounds completed
 */
void ArbitraryElection::finish()
{
    recordScalar("finalLeader", L);
    recordScalar("isLeader", isLeader ? 1 : 0);
    recordScalar("totalMessagesSent", messagesSent);
    recordScalar("totalRounds", round);
    
    EV_INFO << "[STATS] Node " << nodeId 
            << " | Leader: " << L 
            << " | IsLeader: " << (isLeader ? "YES" : "NO")
            << " | Messages: " << messagesSent << "\n";
}
