#include "election_analyzer.h"
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <cmath>

Define_Module(ElectionAnalyzer);

ElectionAnalyzer::ElectionAnalyzer()
{
    totalRuns = 0;
    currentLeader = -1;
    currentRounds = 0;
    currentMessages = 0;
    leaderElected = false;
}

ElectionAnalyzer::~ElectionAnalyzer()
{
}

void ElectionAnalyzer::initialize()
{
    // Register signals
    leaderDistributionSignal = registerSignal("leaderDistribution");
    avgRoundsSignal = registerSignal("avgRounds");
    avgMessagesSignal = registerSignal("avgMessages");
    
    // Initialize tracking
    electionStartTime = simTime();
    leaderElected = false;
    currentLeader = -1;
    currentRounds = 0;
    currentMessages = 0;
    
    // Get output file name from parameter or use default
    outputFile = par("outputFile").stringValue();
    if (outputFile.empty()) {
        outputFile = "election_analysis.txt";
    }
    
    EV_INFO << "[ANALYZER] Election Analyzer initialized\n";
}

void ElectionAnalyzer::handleMessage(cMessage *msg)
{
    // This module doesn't receive messages directly
    // Statistics are collected via method calls
    delete msg;
}

void ElectionAnalyzer::reportLeaderElected(int nodeId, int rounds, int messages)
{
    if (!leaderElected) {
        leaderElected = true;
        currentLeader = nodeId;
        currentRounds = rounds;
        currentMessages = messages;
        
        simtime_t electionTime = simTime() - electionStartTime;
        
        // Update statistics
        leaderCounts[nodeId]++;
        roundsPerRun.push_back(rounds);
        messagesPerRun.push_back(messages);
        timeToElection.push_back(electionTime.dbl());
        totalRuns++;
        
        EV_INFO << "\n╔══════════════════════════════════════════╗\n"
                << "║         ELECTION COMPLETE                 ║\n"
                << "╠══════════════════════════════════════════╣\n"
                << "║  Leader: Node " << std::setw(3) << nodeId << "                        ║\n"
                << "║  Rounds: " << std::setw(5) << rounds << "                          ║\n"
                << "║  Messages: " << std::setw(6) << messages << "                       ║\n"
                << "║  Time: " << std::fixed << std::setprecision(4) << electionTime.dbl() << "s                       ║\n"
                << "╚══════════════════════════════════════════╝\n\n";
        
        emit(leaderDistributionSignal, nodeId);
    }
}

void ElectionAnalyzer::collectStatistics()
{
    if (roundsPerRun.empty()) return;
    
    // Calculate averages
    double avgRounds = std::accumulate(roundsPerRun.begin(), roundsPerRun.end(), 0.0) / roundsPerRun.size();
    double avgMessages = std::accumulate(messagesPerRun.begin(), messagesPerRun.end(), 0.0) / messagesPerRun.size();
    double avgTime = std::accumulate(timeToElection.begin(), timeToElection.end(), 0.0) / timeToElection.size();
    
    // Calculate standard deviations
    double sqSumRounds = 0, sqSumMessages = 0, sqSumTime = 0;
    for (size_t i = 0; i < roundsPerRun.size(); i++) {
        sqSumRounds += pow(roundsPerRun[i] - avgRounds, 2);
        sqSumMessages += pow(messagesPerRun[i] - avgMessages, 2);
        sqSumTime += pow(timeToElection[i] - avgTime, 2);
    }
    double stdRounds = sqrt(sqSumRounds / roundsPerRun.size());
    double stdMessages = sqrt(sqSumMessages / messagesPerRun.size());
    double stdTime = sqrt(sqSumTime / timeToElection.size());
    
    // Record scalars
    recordScalar("avgRoundsToElection", avgRounds);
    recordScalar("stdRoundsToElection", stdRounds);
    recordScalar("avgMessagesToElection", avgMessages);
    recordScalar("stdMessagesToElection", stdMessages);
    recordScalar("avgTimeToElection", avgTime);
    recordScalar("stdTimeToElection", stdTime);
    recordScalar("totalRuns", totalRuns);
    
    // Record leader distribution
    for (const auto& pair : leaderCounts) {
        std::string scalarName = "leaderCount_node" + std::to_string(pair.first);
        recordScalar(scalarName.c_str(), pair.second);
    }
    
    emit(avgRoundsSignal, avgRounds);
    emit(avgMessagesSignal, avgMessages);
}

void ElectionAnalyzer::printSummary()
{
    if (roundsPerRun.empty()) {
        EV_WARN << "[ANALYZER] No election data collected\n";
        return;
    }
    
    double avgRounds = std::accumulate(roundsPerRun.begin(), roundsPerRun.end(), 0.0) / roundsPerRun.size();
    double avgMessages = std::accumulate(messagesPerRun.begin(), messagesPerRun.end(), 0.0) / messagesPerRun.size();
    
    EV_INFO << "\n"
            << "╔═══════════════════════════════════════════════════════════╗\n"
            << "║              ELECTION ANALYSIS SUMMARY                     ║\n"
            << "╠═══════════════════════════════════════════════════════════╣\n"
            << "║  Total Runs: " << std::setw(5) << totalRuns << "                                       ║\n"
            << "║  Avg Rounds: " << std::fixed << std::setprecision(2) << std::setw(8) << avgRounds << "                                  ║\n"
            << "║  Avg Messages: " << std::setw(8) << avgMessages << "                                ║\n"
            << "╠═══════════════════════════════════════════════════════════╣\n"
            << "║  LEADER DISTRIBUTION:                                      ║\n";
    
    for (const auto& pair : leaderCounts) {
        double percentage = (double)pair.second / totalRuns * 100;
        EV_INFO << "║    Node " << std::setw(3) << pair.first << ": " 
                << std::setw(4) << pair.second << " times (" 
                << std::fixed << std::setprecision(1) << std::setw(5) << percentage << "%)                  ║\n";
    }
    
    EV_INFO << "╚═══════════════════════════════════════════════════════════╝\n\n";
}

void ElectionAnalyzer::writeAnalysisReport()
{
    std::ofstream file(outputFile);
    if (!file.is_open()) {
        EV_ERROR << "[ANALYZER] Could not open output file: " << outputFile << "\n";
        return;
    }
    
    file << "=== Leader Election Analysis Report ===\n\n";
    file << "Total Runs: " << totalRuns << "\n\n";
    
    if (!roundsPerRun.empty()) {
        double avgRounds = std::accumulate(roundsPerRun.begin(), roundsPerRun.end(), 0.0) / roundsPerRun.size();
        double avgMessages = std::accumulate(messagesPerRun.begin(), messagesPerRun.end(), 0.0) / messagesPerRun.size();
        double avgTime = std::accumulate(timeToElection.begin(), timeToElection.end(), 0.0) / timeToElection.size();
        
        file << "Performance Metrics:\n";
        file << "  Average Rounds to Election: " << std::fixed << std::setprecision(2) << avgRounds << "\n";
        file << "  Average Messages: " << avgMessages << "\n";
        file << "  Average Time: " << avgTime << "s\n\n";
        
        file << "Leader Distribution:\n";
        for (const auto& pair : leaderCounts) {
            double percentage = (double)pair.second / totalRuns * 100;
            file << "  Node " << pair.first << ": " << pair.second 
                 << " times (" << std::setprecision(1) << percentage << "%)\n";
        }
        
        file << "\nPer-Run Details:\n";
        file << "Run\tRounds\tMessages\tTime(s)\n";
        for (size_t i = 0; i < roundsPerRun.size(); i++) {
            file << (i+1) << "\t" << roundsPerRun[i] << "\t" 
                 << messagesPerRun[i] << "\t\t" 
                 << std::setprecision(4) << timeToElection[i] << "\n";
        }
    }
    
    file.close();
    EV_INFO << "[ANALYZER] Report written to: " << outputFile << "\n";
}

void ElectionAnalyzer::finish()
{
    collectStatistics();
    printSummary();
    writeAnalysisReport();
}
