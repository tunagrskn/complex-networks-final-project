#include "anonymous_election.h"
#include "election_analyzer.h"

Define_Module(AnonymousElection);

/**
 * Constructor: Initialize member variables to default values
 */
AnonymousElection::AnonymousElection()
    : rng(), bitDist(0, 1)
{
    state = ACTIVE;
    bit = 0;
    round = 0;
    messagesSent = 0;
    messagesReceived = 0;
    roundTimer = nullptr;
}

/**
 * Destructor: Clean up timer message
 */
AnonymousElection::~AnonymousElection()
{
    cancelAndDelete(roundTimer);
    for (auto msg : futureMessages) {
        delete msg;
    }
    futureMessages.clear();
}

/**
 * Initialize the anonymous election algorithm
 * - Set initial state to ACTIVE
 * - Populate activeNodes set with self and all neighbors
 * - Setup unique random seed per node
 * - Schedule first round after startDelay
 */
void AnonymousElection::initialize()
{
    ElectionNode::initialize();
    
    // Initialize algorithm parameters
    state = ACTIVE;
    bit = 0;
    round = 0;
    
    // CRITICAL: Create truly unique random seed per node
    // Use std::random_device for hardware entropy + node-specific components
    std::random_device rd;
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    
    // Combine: hardware entropy + nanoseconds + nodeId + run number + memory address
    unsigned long seed = rd() ^ 
                         static_cast<unsigned long>(nanos) ^ 
                         (static_cast<unsigned long>(nodeId) * 1000003) ^
                         (static_cast<unsigned long>(getEnvir()->getConfigEx()->getActiveRunNumber()) * 7919) ^
                         reinterpret_cast<unsigned long>(this);
    
    rng.seed(seed);
    
    EV_INFO << "[INIT] Node " << nodeId << " RNG seed: " << seed << "\n";
    
    // Initially all nodes are active
    for (int i = 0; i < numNodes; i++) {
        activeNodes.insert(i);
    }
    
    EV_INFO << "[INIT] Node " << nodeId << " initialized | State: ACTIVE | Neighbors: " 
            << neighbors.size() << " | Total nodes: " << numNodes << "\n";
    
    // Schedule first round with slightly randomized delay to break synchronization
    double baseDelay = par("startDelay").doubleValue();
    double jitter = bitDist(rng) * 0.01;  // Small random jitter
    roundTimer = new cMessage("roundTimer");
    scheduleAt(simTime() + baseDelay + jitter, roundTimer);
}

/**
 * Handle incoming messages:
 * - roundTimer: Trigger to start a new round if ACTIVE
 * - BitMsg: Bit value from neighbor in current round
 * - LeaderAnnouncement: Notification that a leader has been elected
 */
void AnonymousElection::handleMessage(cMessage *msg)
{
    if (msg == roundTimer) {
        // Start a new round
        startRound();
    }
    else if (dynamic_cast<BitMsg*>(msg)) {
        // Receive bit value from neighbor
        BitMsg *bmsg = check_and_cast<BitMsg*>(msg);
        
        // FIX: Handle out-of-order messages (synchronization)
        if (bmsg->getRoundNum() > round) {
            // Message from future round, buffer it
            futureMessages.push_back(msg);
            return; // Do not delete
        }
        else if (bmsg->getRoundNum() < round) {
            // Old message, ignore
            delete msg;
            return;
        }
        
        // Current round processing (bmsg->getRoundNum() == round)
        {
            int originatorId = bmsg->getSenderId();
            
            // Ignore our own messages reflected back
            if (originatorId != nodeId && receivedBits.find(originatorId) == receivedBits.end()) {
                int bitVal = bmsg->getBitValue();
                bool isActiveNode = bmsg->isActive();
                
                receivedBits[originatorId] = bitVal;
                messagesReceived++;
                
                // Update active nodes list based on received info
                if (isActiveNode) activeNodes.insert(originatorId);
                else activeNodes.erase(originatorId);
                
                // FLOODING: Forward to all neighbors to ensure global visibility
                // Exclude the gate the message arrived on to prevent sending it back immediately.
                broadcastToNeighbors(bmsg, msg->getArrivalGate()->getIndex());
                messagesSent += (neighbors.size() - 1); // Update stats for forwarded messages
                
                // Check if we received from all other nodes (global consensus)
                if (receivedBits.size() == numNodes - 1) {
                    processRound();
                }
            }
        }
        
        delete msg;
    }
    else if (dynamic_cast<LeaderAnnouncement*>(msg)) {
        LeaderAnnouncement *lmsg = check_and_cast<LeaderAnnouncement*>(msg);

        // If we are already not active, we have already processed this or a similar announcement.
        if (state == ACTIVE) {
            int leaderId = lmsg->getLeaderId();
            EV << "Node " << nodeId << " received leader announcement: Node "
               << leaderId << " is the Grand Master\n";

            becomePassive();

            // Flood the announcement to other neighbors, excluding the sender.
            broadcastToNeighbors(lmsg, msg->getArrivalGate()->getIndex());
            messagesSent += (neighbors.size() - 1);
        }
        // The original message must be deleted.
        delete lmsg;
    }
    else {
        delete msg;
    }
}

/**
 * Start a new round:
 * 1. Increment round counter and reset received messages
 * 2. Randomly choose a bit (0 or 1)
 * 3. Broadcast chosen bit to all neighbors
 */
void AnonymousElection::startRound()
{
    round++;
    receivedBits.clear();
    messagesReceived = 0;
    
    // Randomly choose a bit (0 or 1) if ACTIVE, else 0
    // CRITICAL: Use C++11 random with hardware-seeded Mersenne Twister
    bit = 0;
    if (state == ACTIVE) {
        bit = bitDist(rng);  // True random bit using mt19937
    }
    
    EV_INFO << "[ROUND " << round << "] Node " << nodeId 
            << " | Bit: " << bit 
            << " | State: " << (state==ACTIVE ? "ACTIVE" : "PASSIVE") << "\n";
    
    // Send bit to all neighbors
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
    
    // FIX: Process buffered messages for this round
    auto it = futureMessages.begin();
    while (it != futureMessages.end()) {
        BitMsg* bmsg = check_and_cast<BitMsg*>(*it);
        if (bmsg->getRoundNum() == round) {
            cMessage* msg = *it;
            it = futureMessages.erase(it);
            handleMessage(msg);
        } else {
            ++it;
        }
    }
    
    // If this node has no neighbors (should not happen in fully connected),
    // process immediately. Also if numNodes=1.
    if (numNodes == 1 || neighbors.empty()) {
        processRound();
    }
}

/**
 * Process round results and decide next action:
 * - Count |S| = number of nodes (including self) that chose bit=1
 * - If |S| = 1 and I chose 1: become LEADER
 * - If |S| = 1 and I chose 0: become PASSIVE
 * - If 1 < |S| < n and I chose 1: advance to next round
 * - If 1 < |S| < n and I chose 0: become PASSIVE
 * - If |S| = 0 or |S| = n: no progress, repeat round
 */
void AnonymousElection::processRound()
{
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
    
    EV_INFO << "[ROUND " << round << " RESULT] Node " << nodeId 
            << " | My bit: " << bit 
            << " | Nodes with bit=1: " << S_size 
            << " | Active nodes: " << activeNodes.size() << "\n";
    
    // Only ACTIVE nodes make decisions
    if (state == ACTIVE) {
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
                EV_INFO << "[ADVANCE] Node " << nodeId << " -> round " << (round + 1) << "\n";
                emit(roundsCompletedSignal, round);
            } else {
                // I chose 0, so I become passive
                becomePassive();
            }
        }
        else {
            // All nodes chose the same bit (no progress), repeat the round
            EV_DEBUG << "[REPEAT] Node " << nodeId << " no progress, repeating\n";
        }
    }

    // Schedule next round for everyone (ACTIVE and PASSIVE) to keep synchronization
    if (state != LEADER) {
        if (roundTimer->isScheduled()) {
            cancelEvent(roundTimer);
        }
        scheduleAt(simTime() + par("roundDelay").doubleValue(), roundTimer);
    }
}

/**
 * Transition to LEADER state:
 * - Update state to LEADER
 * - Emit leader elected signal
 * - Display visual bubble and change node appearance
 * - Announce leadership to all neighbors
 */
void AnonymousElection::becomeLeader()
{
    state = LEADER;
    
    EV_WARN << "\n========================================\n"
            << "  LEADER ELECTED: Node " << nodeId << "\n"
            << "  Round: " << round << " | Messages: " << messagesSent << "\n"
            << "========================================\n\n";
    
    emit(leaderElectedSignal, nodeId);
    
    // Report to analyzer
    cModule *analyzerModule = getParentModule()->getSubmodule("analyzer");
    if (analyzerModule) {
        ElectionAnalyzer *analyzer = check_and_cast<ElectionAnalyzer*>(analyzerModule);
        analyzer->reportLeaderElected(nodeId, round, messagesSent);
    }
    
    // Display prominent message
    bubble("I am the Grand Master!");
    
    // Change display to highlight leader
    getDisplayString().setTagArg("i", 1, "gold");
    getDisplayString().setTagArg("i", 2, "40");
    
    // Announce to all neighbors by flooding
    LeaderAnnouncement *lmsg = new LeaderAnnouncement();
    lmsg->setLeaderId(nodeId);
    broadcastToNeighbors(lmsg); // Send to all neighbors
    delete lmsg; // We created it, so we must delete it.
}

/**
 * Transition to PASSIVE state:
 * - Update state to PASSIVE
 * - Remove self from active nodes set
 * - Change node appearance to gray
 */
void AnonymousElection::becomePassive()
{
    state = PASSIVE;
    activeNodes.erase(nodeId);
    
    EV_DEBUG << "[PASSIVE] Node " << nodeId << " eliminated at round " << round << "\n";
    
    // Change display to show passive state
    getDisplayString().setTagArg("i", 1, "gray");
}

/**
 * Record statistics at end of simulation:
 * - Final state (ACTIVE/PASSIVE/LEADER)
 * - Whether this node is the leader
 * - Total messages sent
 * - Total rounds completed
 */
void AnonymousElection::finish()
{
    // Record statistics
    recordScalar("finalState", state);
    recordScalar("isLeader", (state == LEADER) ? 1 : 0);
    recordScalar("totalMessagesSent", messagesSent);
    recordScalar("totalRounds", round);
    
    const char* stateStr = (state == LEADER) ? "LEADER" : 
                          (state == PASSIVE) ? "PASSIVE" : "ACTIVE";
    
    EV_INFO << "[STATS] Node " << nodeId 
            << " | State: " << stateStr 
            << " | Messages: " << messagesSent 
            << " | Rounds: " << round << "\n";
    
    if (state == LEADER) {
        EV_WARN << "[FINAL] Grand Master: Node " << nodeId 
                << " elected after " << round << " rounds\n";
    }
}
