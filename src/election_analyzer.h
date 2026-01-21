#ifndef __ELECTIONANALYZER_H_
#define __ELECTIONANALYZER_H_

#include <omnetpp.h>
#include <map>
#include <vector>
#include <fstream>

using namespace omnetpp;

/**
 * Election Analyzer Module
 * 
 * Collects and analyzes statistics from leader election simulations:
 * - Leader distribution across runs
 * - Message complexity analysis
 * - Round/time complexity analysis
 * - Convergence metrics
 */
class ElectionAnalyzer : public cSimpleModule
{
private:
    // Statistics collection
    int totalRuns;
    std::map<int, int> leaderCounts;      // How many times each node was elected
    std::vector<int> roundsPerRun;        // Rounds needed per run
    std::vector<int> messagesPerRun;      // Total messages per run
    std::vector<double> timeToElection;   // Simulation time to elect leader
    
    // Current run tracking
    int currentLeader;
    int currentRounds;
    int currentMessages;
    simtime_t electionStartTime;
    bool leaderElected;
    
    // Signals for statistics
    simsignal_t leaderDistributionSignal;
    simsignal_t avgRoundsSignal;
    simsignal_t avgMessagesSignal;
    
    // Output file
    std::string outputFile;
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    
    void collectStatistics();
    void writeAnalysisReport();
    void printSummary();
    
public:
    ElectionAnalyzer();
    virtual ~ElectionAnalyzer();
    
    // Called by election nodes to report leader election
    void reportLeaderElected(int nodeId, int rounds, int messages);
};

#endif
