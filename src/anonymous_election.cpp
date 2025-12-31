#include "anonymous_election.h"

Define_Module(AnonymousElection);

AnonymousElection::AnonymousElection()
{
    state = ACTIVE;
    bit = 0;
    round = 0;
    messagesSent = 0;
    messagesReceived = 0;
    roundTimer = nullptr;
}

AnonymousElection::~AnonymousElection()
{
    cancelAndDelete(roundTimer);
}

void AnonymousElection::initialize()
{
    TSNNode::initialize();
    
    // Initialize algorithm parameters
    state = ACTIVE;
    bit = 0;
    round = 0;
    
    // Initially all nodes are active
    activeNodes.insert(nodeId);
    for (int neighId : neighbors) {
        activeNodes.insert(neighId);
    }
    
    EV << "Node " << nodeId << " initialized as ACTIVE with " 
       << activeNodes.size() << " total active nodes\n";
    
    // Schedule first round
    roundTimer = new cMessage("roundTimer");
    scheduleAt(simTime() + par("startDelay").doubleValue(), roundTimer);
}

void AnonymousElection::handleMessage(cMessage *msg)
{
    if (msg == roundTimer) {
        // Start a new round
        if (state == ACTIVE) {
            startRound();
        }
    }
    else if (dynamic_cast<BitMsg*>(msg)) {
        // Receive bit value from neighbor
        BitMsg *bmsg = check_and_cast<BitMsg*>(msg);
        
        if (bmsg->getRoundNum() == round && state == ACTIVE) {
            int neighborId = bmsg->getSenderId();
            int neighborBit = bmsg->getBitValue();
            bool neighborActive = bmsg->isActive();
            
            receivedBits[neighborId] = neighborBit;
            messagesReceived++;
            
            // Update active nodes list
            if (neighborActive) {
                activeNodes.insert(neighborId);
            } else {
                activeNodes.erase(neighborId);
            }
            
            EV << "Node " << nodeId << " received bit=" << neighborBit 
               << " from node " << neighborId 
               << " (active=" << neighborActive << ", round " << round << ")\n";
            
            // Check if we received from all neighbors
            if (receivedBits.size() == neighbors.size()) {
                processRound();
            }
        }
        
        delete msg;
    }
    else if (dynamic_cast<LeaderAnnouncement*>(msg)) {
        LeaderAnnouncement *lmsg = check_and_cast<LeaderAnnouncement*>(msg);
        int leaderId = lmsg->getLeaderId();
        
        EV << "Node " << nodeId << " received leader announcement: Node " 
           << leaderId << " is the Grand Master\n";
        
        delete msg;
    }
    else {
        delete msg;
    }
}

void AnonymousElection::startRound()
{
    if (state != ACTIVE) {
        return;
    }
    
    round++;
    receivedBits.clear();
    messagesReceived = 0;
    
    // Randomly choose a bit (0 or 1)
    bit = intuniform(0, 1);
    
    EV << "Node " << nodeId << " starting round " << round 
       << " with bit=" << bit << "\n";
    
    // Send bit to all neighbors (including passive ones for coordination)
    for (int neighId : neighbors) {
        BitMsg *bmsg = new BitMsg();
        bmsg->setSenderId(nodeId);
        bmsg->setBitValue(bit);
        bmsg->setRoundNum(round);
        bmsg->setIsActive(state == ACTIVE);
        
        int gateIndex = getNeighborGateIndex(neighId);
        if (gateIndex >= 0) {
            send(bmsg, "port$o", gateIndex);
            messagesSent++;
        } else {
            delete bmsg;
        }
    }
    
    emit(messagesSentSignal, messagesSent);
    
    // If this node has no neighbors (should not happen in fully connected),
    // process immediately
    if (neighbors.empty()) {
        processRound();
    }
}

void AnonymousElection::processRound()
{
    if (state != ACTIVE) {
        return;
    }
    
    // Count how many nodes (including self) chose bit=1
    int S_size = 0;
    bool iChoose1 = (bit == 1);
    
    if (iChoose1) {
        S_size++;
    }
    
    for (const auto& pair : receivedBits) {
        if (pair.second == 1) {
            S_size++;
        }
    }
    
    EV << "Node " << nodeId << " round " << round << " analysis:\n";
    EV << "  My bit: " << bit << "\n";
    EV << "  Nodes with bit=1 (|S|): " << S_size << "\n";
    EV << "  Total active nodes: " << activeNodes.size() << "\n";
    
    // Decision logic
    if (S_size == 1 && iChoose1) {
        // I am the only one with bit=1, I become the leader
        becomeLeader();
    }
    else if (S_size == 1 && !iChoose1) {
        // Someone else is the leader, I become passive
        becomePassive();
    }
    else if (S_size > 1 && S_size < (int)activeNodes.size()) {
        // Multiple nodes chose 1, but not all
        if (iChoose1) {
            // I chose 1, so I advance to next round
            EV << "Node " << nodeId << " advances to round " << (round + 1) << "\n";
            emit(roundsCompletedSignal, round);
            
            // Schedule next round
            scheduleAt(simTime() + par("roundDelay").doubleValue(), roundTimer);
        } else {
            // I chose 0, so I become passive
            becomePassive();
        }
    }
    else {
        // All nodes chose the same bit (no progress), repeat the round
        EV << "Node " << nodeId << " no progress made, repeating selection\n";
        
        // Schedule next round immediately
        scheduleAt(simTime() + par("roundDelay").doubleValue(), roundTimer);
    }
}

void AnonymousElection::becomeLeader()
{
    state = LEADER;
    
    EV << "Node " << nodeId << " ELECTED AS GRAND MASTER (round " << round << ")\n";
    
    emit(leaderElectedSignal, nodeId);
    
    // Display prominent message
    bubble("I am the Grand Master!");
    
    // Change display to highlight leader
    getDisplayString().setTagArg("i", 1, "gold");
    getDisplayString().setTagArg("i", 2, "40");
    
    // Announce to all neighbors
    for (int neighId : neighbors) {
        LeaderAnnouncement *lmsg = new LeaderAnnouncement();
        lmsg->setLeaderId(nodeId);
        
        int gateIndex = getNeighborGateIndex(neighId);
        if (gateIndex >= 0) {
            send(lmsg, "port$o", gateIndex);
        } else {
            delete lmsg;
        }
    }
}

void AnonymousElection::becomePassive()
{
    state = PASSIVE;
    activeNodes.erase(nodeId);
    
    EV << "Node " << nodeId << " became PASSIVE (round " << round << ")\n";
    
    // Change display to show passive state
    getDisplayString().setTagArg("i", 1, "gray");
}

void AnonymousElection::finish()
{
    // Record statistics
    recordScalar("finalState", state);
    recordScalar("isLeader", (state == LEADER) ? 1 : 0);
    recordScalar("totalMessagesSent", messagesSent);
    recordScalar("totalRounds", round);
    
    const char* stateStr = (state == LEADER) ? "LEADER" : 
                          (state == PASSIVE) ? "PASSIVE" : "ACTIVE";
    
    EV << "Node " << nodeId << " statistics:\n";
    EV << "  Final State: " << stateStr << "\n";
    EV << "  Messages Sent: " << messagesSent << "\n";
    EV << "  Rounds Completed: " << round << "\n";
}
