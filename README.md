# CPU Scheduler Simulation

A C++ implementation of CPU scheduling algorithms including First-Come-First-Served (FCFS) and Round Robin (RR) scheduling with I/O burst simulation.

## Author
- **Name**: Jimmy Ly
- **Date**: October 6, 2025

## Overview

This project simulates CPU scheduling algorithms with the following features:
- **FCFS (First-Come-First-Served)**: Processes are executed in the order they arrive
- **Round Robin**: Processes are executed with a time quantum, allowing for preemption
- **I/O Burst Simulation**: Processes can be blocked for I/O operations
- **Multi-threading**: Uses pthread for concurrent execution
- **Comprehensive Logging**: Detailed execution logs with timing information

## Project Structure

```
Scheduling/
├── schedule.cpp          # Main scheduler implementation
├── log.cpp              # Logging functions implementation
├── log.h                # Logging functions header
├── Makefile             # Build configuration
├── bursts_rr_3.txt      # Sample input file
├── expectedoutput_fcfs.txt    # Expected FCFS output
├── expectedoutput_rr_3.txt    # Expected Round Robin output
└── README.md            # This file
```

## Features

### Scheduling Algorithms
- **FCFS**: Non-preemptive scheduling where processes run to completion
- **Round Robin**: Preemptive scheduling with configurable time quantum

### Process Management
- CPU and I/O burst simulation
- Process state tracking (ready, running, blocked)
- Turnaround time and wait time calculation
- Completion time tracking

### Logging System
- Real-time execution logging
- Process burst execution tracking
- Completion statistics
- Standardized output format

## Building the Project

### Prerequisites
- C++17 compatible compiler (g++)
- pthread library support
- Make utility (optional, for using Makefile)

### Compilation

#### Using Make (Recommended)
```bash
make                    # Build the scheduler
make clean             # Clean build artifacts
make help              # Show available targets
```

#### Manual Compilation
```bash
g++ -std=c++17 -Wall -Wextra -pthread -o schedule schedule.cpp log.cpp
```

## Usage

### Command Line Syntax
```bash
./schedule [-s fcfs|rr] [-q N] <bursts-file>
```

### Parameters
- `-s fcfs|rr`: Scheduling strategy (default: fcfs)
- `-q N`: Time quantum for Round Robin (default: 2)
- `<bursts-file>`: Input file containing process burst information

### Examples

#### FCFS Scheduling (Default)
```bash
./schedule bursts_rr_3.txt
```

#### Round Robin with Quantum 3
```bash
./schedule -s rr -q 3 bursts_rr_3.txt
```

## Input Format

The input file should contain one line per process, with space-separated burst times:
- Odd positions (1st, 3rd, 5th, ...): CPU burst times
- Even positions (2nd, 4th, 6th, ...): I/O burst times
- Each process must have an odd number of bursts (ending with a CPU burst)

### Example Input
```
4 4 2
1 7 3
3 2 4
```

This represents:
- **Process 0**: 4ms CPU → 4ms I/O → 2ms CPU
- **Process 1**: 1ms CPU → 7ms I/O → 3ms CPU
- **Process 2**: 3ms CPU → 2ms I/O → 4ms CPU

## Output Format

The program produces detailed execution logs:

### Execution Logs
```
P0: executed cpu bursts = 4, executed io bursts = 0, time elapsed = 4, enter io
P1: executed cpu bursts = 1, executed io bursts = 0, time elapsed = 5, enter io
P2: executed cpu bursts = 3, executed io bursts = 0, time elapsed = 8, enter io
P0: executed cpu bursts = 6, executed io bursts = 4, time elapsed = 10, completed
```

### Completion Statistics
```
P0: turnaround time = 10, wait time = 0
P2: turnaround time = 14, wait time = 5
P1: turnaround time = 17, wait time = 6
```

### Stop Reasons
- `enter io`: Process completed CPU burst and entered I/O
- `quantum expired`: Process was preempted due to time quantum
- `completed`: Process finished all bursts

## Testing

### Run Tests
```bash
make test-fcfs    # Test FCFS scheduling
make test-rr      # Test Round Robin scheduling
make test         # Run all tests
```

### Manual Testing
```bash
# Test FCFS
./schedule bursts_rr_3.txt > output_fcfs.txt
diff output_fcfs.txt expectedoutput_fcfs.txt

# Test Round Robin
./schedule -s rr -q 3 bursts_rr_3.txt > output_rr_3.txt
diff output_rr_3.txt expectedoutput_rr_3.txt
```

## Algorithm Details

### FCFS (First-Come-First-Served)
- Non-preemptive scheduling
- Processes execute until completion or I/O
- Simple implementation with queue-based ready list

### Round Robin
- Preemptive scheduling with time quantum
- Processes are preempted when quantum expires
- Preempted processes return to ready queue
- Ensures fair CPU time distribution

### I/O Handling
- Processes blocked during I/O operations
- I/O completion handled by separate thread
- Blocked processes return to ready queue when I/O completes

## Technical Implementation

### Key Components
- **Simulation Class**: Main scheduler logic
- **Process Structure**: Tracks process state and timing
- **Blocked Queue**: Manages I/O blocked processes
- **Ready Queue**: Manages processes ready for execution
- **Logging System**: Standardized output formatting

### Threading
- Main thread: Handles user input and output
- Scheduler thread: Executes scheduling simulation
- Atomic operations for thread-safe communication

### Data Structures
- `std::queue<Proc*>`: Ready queue (FIFO)
- `std::deque<BlockedItem>`: Blocked processes with I/O timing
- `std::vector<Proc>`: Process storage and management

## Error Handling

The program includes comprehensive error checking:
- Invalid command line arguments
- File I/O errors
- Invalid burst values (must be positive)
- Invalid process configurations (odd number of bursts required)

## Performance Considerations

- Efficient O(n log n) sorting for blocked processes
- Minimal memory overhead with smart pointer usage
- Thread-safe atomic operations
- Optimized I/O handling with batch processing

## Contributing

This is an academic project. For questions or issues, please contact the author.

---

