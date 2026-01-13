# Time-Sensitive Networking (TSN) Grand Master Election Algorithms

**Complex Networks Final Project** - Distributed Leader Election for TSN Networks

## Project Overview

This project implements and compares **distributed leader election algorithms** for selecting a Grand Master (GM) in Time-Sensitive Networking (TSN) environments. TSN networks require precise time synchronization, and the Grand Master acts as the authoritative clock source for the entire network.

## Motivation

Current TSN standards (IEEE 802.1AS) use the **Best Master Clock Algorithm (BMCA)** which relies on comparing node properties (priority, clock quality, MAC address). However, in distributed networks, classical leader election algorithms offer interesting alternatives that don't require pre-configured priorities or centralized coordination.

## Implemented Algorithms

### 1. **Arbitrary Election Algorithm** (Flooding-based)
- **Source**: Distributed Algorithms, Section 11.2.3
- **Approach**: Each node maintains L(i) (leader candidate), initially set to its own ID
- **Process**: Nodes exchange L values with neighbors for D rounds (network diameter)
- **Result**: Node with maximum ID becomes the leader
- **Complexity**: O(D) rounds, O(|E| × D) messages
- **Networks**: Ring, Mesh, Star topologies

### 2. **Anonymous Election Algorithm** (Randomized)
- **Source**: Distributed Algorithms, Section 11.2.4
- **Approach**: Randomized symmetry-breaking without using node identifiers
- **Process**: Active nodes randomly choose bit (0 or 1); those choosing 1 advance
- **Result**: When exactly one node chooses 1, it becomes the leader
- **Complexity**: Expected O(log n) rounds
- **Networks**: Fully connected topology

## Network Topologies Tested

1. **Ring Network** (8 nodes, diameter=4)
2. **2D Mesh Network** (3×3 grid, 9 nodes, diameter=4)
3. **Star Network** (7 nodes, diameter=2)
4. **Fully Connected Network** (6 nodes)

## Performance Metrics

- **Total Messages Sent**: Communication overhead
- **Number of Rounds**: Time to convergence
- **Leader Election Success Rate**: Algorithm reliability
- **Network Diameter Impact**: Scalability analysis

## Key Findings & Contributions

1. **Topology Impact**: Star networks converge fastest (diameter=2), while ring networks require more rounds
2. **Message Complexity**: Arbitrary algorithm has predictable message count (deterministic), while anonymous algorithm varies (randomized)
3. **TSN Applicability**: Both algorithms successfully elect a unique Grand Master in different network structures
4. **Trade-offs**: 
   - Arbitrary: Deterministic but requires node IDs and diameter knowledge
   - Anonymous: Works without IDs but requires full connectivity and randomization

## Technology Stack

- **Simulation Platform**: OMNeT++ 6.3.0
- **Language**: C++17
- **Network Description**: NED (Network Description Language)
- **Build System**: Make / opp_makemake

## Project Structure

```
├── src/                          # C++ implementation
│   ├── tsn_node.{h,cpp}         # Base TSN node class
│   ├── arbitrary_election.{h,cpp} # Flooding-based algorithm
│   ├── anonymous_election.{h,cpp} # Randomized algorithm
│   └── messages.msg             # OMNeT++ message definitions
├── simulations/                 # Simulation configurations
│   ├── TSNElection.ned         # Network topologies
│   ├── omnetpp.ini             # Simulation parameters
│   └── results/                # Simulation output
└── out/                        # Build artifacts
