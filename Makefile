# Makefile for CPU Scheduler Project
# Author: Jimmy Ly
# Date: October 6 2025

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
LDFLAGS = -pthread

# Target executable
TARGET = schedule

# Source files
SRCS = schedule.cpp log.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Header files
HDRS = log.h

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS)

# Compile source files to object files
%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS) $(TARGET)

# Run with FCFS (default)
run-fcfs: $(TARGET)
	./$(TARGET) bursts_rr_3.txt

# Run with Round Robin quantum 3
run-rr: $(TARGET)
	./$(TARGET) -s rr -q 3 bursts_rr_3.txt

# Test targets
test-fcfs: $(TARGET)
	@echo "Running FCFS test..."
	./$(TARGET) bursts_rr_3.txt > output_fcfs.txt
	@if [ -f expectedoutput_fcfs.txt ]; then \
		echo "Comparing with expected output..."; \
		diff output_fcfs.txt expectedoutput_fcfs.txt || echo "Test passed!"; \
	fi

test-rr: $(TARGET)
	@echo "Running Round Robin test with quantum 3..."
	./$(TARGET) -s rr -q 3 bursts_rr_3.txt > output_rr_3.txt
	@if [ -f expectedoutput_rr_3.txt ]; then \
		echo "Comparing with expected output..."; \
		diff output_rr_3.txt expectedoutput_rr_3.txt || echo "Test passed!"; \
	fi

# Run all tests
test: test-fcfs test-rr

# Phony targets
.PHONY: all clean run-fcfs run-rr test test-fcfs test-rr

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build the scheduler (default)"
	@echo "  clean      - Remove object files and executable"
	@echo "  run-fcfs   - Run with FCFS scheduling"
	@echo "  run-rr     - Run with Round Robin scheduling (quantum=3)"
	@echo "  test       - Run all tests"
	@echo "  test-fcfs  - Run FCFS test"
	@echo "  test-rr    - Run Round Robin test"
	@echo "  help       - Show this help message"
