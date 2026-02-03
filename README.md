# Leader Election Algorithms Simulation

A comprehensive OMNeT++ simulation framework for distributed leader election algorithms in various network topologies.

## Overview

This project implements and compares two fundamental leader election algorithms from distributed systems theory:

1. **Flooding-based Election (Arbitrary Networks)** - Deterministic algorithm using unique process IDs
2. **Randomized Election (Anonymous Networks)** - Probabilistic algorithm for networks without process identifiers

The simulations allow analysis of message complexity, round complexity, and algorithm behavior across different network topologies.

## Algorithms

### Flooding-based Election (Arbitrary Networks)

Based on Section 11.2.3 of distributed algorithms theory:

- Each node maintains a leader candidate `L(i)`, initially set to its own ID
- In each round, nodes exchange `L` values with all neighbors
- Each node updates `L` to the maximum value received
- Algorithm terminates after `D` rounds (network diameter)
- **Message Complexity**: `O(Δ · D)` where Δ is max degree and D is diameter

### Randomized Election (Anonymous Networks)

Based on Section 11.2.4 of distributed algorithms theory:

- Designed for fully connected networks without unique identifiers
- Active nodes randomly choose a bit (0 or 1) each round
- Nodes choosing 1 continue; nodes choosing 0 become passive
- If exactly one node chooses 1, it becomes the leader
- **Message Complexity**: `O(n² log n)` where n is the number of nodes

## Network Topologies

The simulation supports the following topologies:

| Topology | Description | Parameters |
|----------|-------------|------------|
| **Ring** | Circular connection | n nodes, diameter ≈ n/2 |
| **Mesh** | Grid-based layout | k×k grid, diameter = 2(k-1) |
| **Star** | Central hub with spokes | n nodes, diameter = 2 |
| **Fully Connected** | Complete graph | n nodes, diameter = 1 |
| **Random** | Probabilistic connections | configurable connectivity |

## Project Structure

```
leader_election_algorithms/
├── src/
│   ├── election_node.{h,cpp}      # Base class for election nodes
│   ├── arbitrary_election.{h,cpp} # Flooding-based algorithm
│   ├── anonymous_election.{h,cpp} # Randomized algorithm
│   ├── election_analyzer.{h,cpp}  # Statistics collection
│   ├── messages.msg               # Message definitions
│   └── package.ned                # NED package definition
├── simulations/
│   ├── Election.ned               # Network topology definitions
│   ├── omnetpp.ini                # Simulation configurations
│   └── results/                   # Simulation output files
├── report/                        # LaTeX project report
├── notes/                         # Algorithm notes
└── Makefile                       # Build configuration
```

## Prerequisites

- **OMNeT++ 6.0+** - Discrete event simulator
- **C++14** compatible compiler
- **Make** build system

## Building

1. Generate the makefiles:
   ```bash
   make makefiles
   ```

2. Build the project:
   ```bash
   make
   ```

3. Clean build artifacts:
   ```bash
   make clean
   ```

## Running Simulations

Navigate to the `simulations/` directory and run with OMNeT++:

```bash
cd simulations

# Run with GUI
opp_run -m -n .:../src -l ../src/leader_election_algorithms omnetpp.ini

# Run in command-line mode
opp_run -m -u Cmdenv -n .:../src -l ../src/leader_election_algorithms omnetpp.ini -c RingNetwork_Arbitrary
```

### Available Configurations

| Configuration | Description |
|---------------|-------------|
| `RingNetwork_Arbitrary` | Ring topology with flooding algorithm |
| `RingNetwork_Anonymous` | Ring topology with randomized algorithm |
| `MeshNetwork_Arbitrary` | 3×3 mesh with flooding algorithm |
| `MeshNetwork_Anonymous` | 3×3 mesh with randomized algorithm |
| `StarNetwork_Arbitrary` | Star topology with flooding algorithm |
| `StarNetwork_Anonymous` | Star topology with randomized algorithm |
| `FullyConnectedNetwork_Arbitrary` | Complete graph with flooding algorithm |
| `FullyConnectedNetwork_Anonymous` | Complete graph with randomized algorithm |
| `RandomNetwork` | Random topology with flooding algorithm |

## Results and Analysis

Simulation results are stored in `simulations/results/` in the following formats:

- `.vec` - Vector data (time series)
- `.sca` - Scalar statistics
- `.vci` - Vector index files

Key metrics collected:
- **Leader elected** - Which node became leader
- **Messages sent** - Total message count
- **Rounds completed** - Number of rounds until termination

## Complexity Analysis

### Arbitrary Networks (Flooding)

| Metric | Complexity |
|--------|------------|
| Message complexity | O(Δ · D) |
| Round complexity | O(D) |
| Deterministic | Yes |

### Anonymous Networks (Randomized)

| Metric | Complexity |
|--------|------------|
| Message complexity | O(n² log n) |
| Round complexity | O(log n) expected |
| Deterministic | No (probabilistic) |

## References

- Distributed Algorithm Design and Analysis - Chapter 11: Leader Election
- OMNeT++ Simulation Manual: https://omnetpp.org/doc/omnetpp/manual/

## License

See [LICENSE](LICENSE) for details.
