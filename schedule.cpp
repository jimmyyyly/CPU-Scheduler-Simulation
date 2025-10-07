// File: schedule.cpp
// Build: make (produces ./schedule)
// Run examples:
// ./schedule bursts.txt               # FCFS (default)
// ./schedule -s rr -q 3 bursts.txt    # RR with quantum 3

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <map>
#include <numeric>
#include <queue>
#include <sstream>
#include <string>
#include <tuple>
#include <thread>
#include <utility>
#include <vector>
#include "log.h"

struct BurstLine {
    // Original bursts
    std::vector<int> bursts; // odd count, CPU/IO/CPU/...
};

struct Proc {
    int pid;
    std::deque<int> bursts; // remaining bursts (front is the current burst)
    int executed_cpu{0};
    int executed_io{0};
    int total_cpu{0};
    int total_io{0};
    int completion_time{-1};
};

enum class Strategy { FCFS, RR };

struct Options {
    Strategy strategy{Strategy::FCFS};
    int quantum{2};
    std::string file;
};

struct Shared {
    std::atomic<bool> done{false};
};

// -- Utility printing --
static std:: string join_line_readable(const std::vector<int>& v) {
    std:: ostringstream out;
    for (size_t i = 0; i < v.size(); ++ i) {
        int x = v[i];
        if (i) out << ", ";
        out << x << "ms (" << (i % 2 == 0 ? "CPU" : "IO") << ")";
    }
    return out.str();
}

// -- Parsing --
static void exit_ok() {
    // Use exit 0 to exit the program
    std::exit(0);
}

static Options parse_args(int argc, char** argv) {
    Options opt;
    opterr = 0; // Handle errors
    int c;
    while ((c = getopt(argc, argv, "s:q:")) != -1) {
        switch (c) {
            case 's': {
                std::string v (optarg ? optarg: "");
                if (v == "fcfs") opt.strategy = Strategy::FCFS;
                else if (v == "rr") opt.strategy = Strategy::RR;
                else {
                    // Make the invalid strategy -> default to FCFS
                    opt.strategy = Strategy::FCFS;
                }
                break;
            }
            case 'q': {
                // Only relevant for RR but allow it regardless
                char *end = nullptr; long val = std::strtol(optarg, &end, 10);
                if (end == optarg || val <= 0) {
                    std::cout << "Time quantum must be a number and bigger than 0\n";
                    exit_ok();
                }
                opt.quantum = (int)val;
                break;
        }
        default:
            break;
    }
}
if (optind >= argc) {
    std::cout << "Usage: " << argv[0] << " [-s fcfs|rr] [-q N] <bursts-file>\n";
    exit_ok();
}
opt.file = argv[optind];
return opt;
}

static std::vector<BurstLine> read_bursts(const std::string& path) {
    std::ifstream fin(path);
    if (!fin) {
        std::cout << "Unable to open <" << path << ">\n";
        exit_ok();
    }
    std::vector<BurstLine> lines;
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        int x; BurstLine bl;
        while (iss >> x) {
            if (x <= 0) {
                std::cout << "A burst number must be bigger than 0\n";
                exit_ok();
            }
            bl.bursts.push_back(x);
        }
        if (!bl.bursts.empty() && (bl.bursts.size() % 2 == 0)) {
            std::cout << "There must be an odd number of bursts for each process\n";
            exit_ok();
        }
        if (!bl.bursts.empty()) lines.push_back(std::move(bl));
}
return lines;
}

// -- Scheduler Core --
struct BlockedItem { Proc* p; int remaining_io; size_t order; }; // Order for stability

static void stable_sort_blocked(std::deque<BlockedItem>& blocked) {
    std::stable_sort(blocked.begin(), blocked.end(), [](const BlockedItem& a, const BlockedItem& b) {
        if (a.remaining_io != b.remaining_io) return a.remaining_io < b.remaining_io;
        return a.order < b.order;
    });
}

struct Simulation {
    Options opt;
    Shared* shared;
    int time_elapsed{0};
    std::queue<Proc*> ready;
    std::deque<BlockedItem> blocked;
    std::vector<Proc> procs;
    std::vector<std::pair<int, int>> completed; // (completion_time, pid)
    size_t order_counter{0};

    explicit Simulation(const Options& o, Shared* s): opt(o), shared(s) {}

    void init_from_lines(const std::vector<BurstLine>& lines) {
        procs.clear();
        for (size_t i = 0; i < lines.size(); ++i) {
            Proc p; p.pid = (int)i; p.bursts = std::deque<int>(lines[i].bursts.begin(), lines[i].bursts.end());
            // sum cpu/io totals for stats
            for (size_t k = 0; k < lines[i].bursts.size(); ++k) {
                if (k % 2 == 0) p.total_cpu += lines[i].bursts[k];
                else p.total_io += lines[i].bursts[k];
            }
            procs.push_back(std::move(p));
        }
        for (auto& p: procs) ready.push(&p);
    }

    void print_input_readback(const std::vector<BurstLine>& lines) {
        for (size_t i = 0; i < lines.size(); ++ i) {
            std::cout << "P" << i << ": " << join_line_readable(lines[i].bursts) << "\n";
        }
    }

    void enqueue_ready(Proc* p) { ready.push(p); }

    void move_to_blocked(Proc* p) {
        // Pop finished CPU burst
        if (!p -> bursts.empty() && p -> bursts.front() == 0) p -> bursts.pop_front();
        if (!p -> bursts.empty()) {
            // now front is IO burst
            int io = p -> bursts.front();
            blocked.push_back(BlockedItem{p, io, order_counter++});
            stable_sort_blocked(blocked);
        }
    }

    // Advance all blocked by dt; move any finished (in ascending remaining order) to ready immeediately
    void advance_blocked(int dt) {
        int t = dt;
        while (t > 0 && !blocked.empty()) {
            stable_sort_blocked(blocked);
            int next_finish = blocked.front().remaining_io;
            int step = std::min(next_finish, t);
            for (auto &bi : blocked) {
                int dec = std::min(step, bi.remaining_io);
                bi.remaining_io -= dec;
                bi.p->executed_io += dec;
            }
            t -= step;
            // move all that hit 0 in the order of the current ordering
            std::deque<BlockedItem> remaining;
            for (auto &bi : blocked) {
                if (bi.remaining_io == 0) {
                    // consume IO burst and push to ready
                    if (!bi.p -> bursts.empty()) {
                        // remove the IO burst front
                        bi.p -> bursts.pop_front();
                    }
                    enqueue_ready(bi.p);
                } else {
                    remaining.push_back(bi);
                }
            }
            blocked.swap(remaining);
        }
        // if t > 0 and not blocked, nothing to do
        if (t > 0) {
            // no blocked to progress; nothing else to update
        }
}

void run() {
    while(true) {
        if (!ready.empty()) {
            Proc* p = ready.front(); ready.pop();
            // Amount this CPU segment can run
            int cpu_remaining = p -> bursts.front();
            int segment = (opt.strategy == Strategy::FCFS) ? cpu_remaining : std::min(cpu_remaining, opt.quantum);

            // Execute in multiple sub-steps due to blocked finishes
            int remaining = segment;
            while (remaining > 0) {
                int step = remaining;
                if (!blocked.empty()) {
                    stable_sort_blocked(blocked);
                    int soonest = blocked.front().remaining_io;
                    if (soonest > 0) step = std::min(step, soonest);
                }
                // Apply step
                p -> executed_cpu += step;
                time_elapsed += step;
                p -> bursts.front() -= step;
                // Progress blocked by step
                advance_blocked(step);
                remaining -= step;
            }

            // Determine the reason we stopped and take actions
            if (p -> bursts.front() == 0) {
                // Finished CPU burst
                p -> bursts.pop_front();
                if (p -> bursts.empty()) {
                    // Completed all bursts
                    p -> completion_time = time_elapsed;
                    completed.push_back({time_elapsed, p -> pid});
                    log_cpuburst_execution(p -> pid, p -> executed_cpu, p -> executed_io, time_elapsed, COMPLETED);
                } else {
                    // Enter IO
                    log_cpuburst_execution(p -> pid, p -> executed_cpu, p -> executed_io, time_elapsed, ENTER_IO);
                    move_to_blocked(p);
                }
            } else {
                // Quantum expired
                log_cpuburst_execution(p -> pid, p -> executed_cpu, p -> executed_io, time_elapsed, QUANTUM_EXPIRED);
                enqueue_ready(p);
            }
        } else if (!blocked.empty()) {
            // No ready tasks; jump time until the earliest IO completes
            stable_sort_blocked(blocked);
            int step = blocked.front().remaining_io;
            // Progress all blocked by 'step' and move those that finish
            advance_blocked(step);
            time_elapsed += step; // Advancing wall time while CPU idle
        } else {
            // Both empty -> done
            break;
        }
    }
}

    void print_stats_and_finish() {
        // Order by completion time (already appended in order of time_elapsed increases)
        std::stable_sort(completed.begin(), completed.end());
        for (auto [t, pid] : completed) {
            const Proc& p = procs[pid];
            int turnaround = p.completion_time; // Admitted at 0
            int wait = turnaround - (p.total_cpu + p.total_io);
            log_process_completion(pid, turnaround, wait);
        }
    }
};

// -- Worker thread --
#include <pthread.h>

struct ThreadArgs { Simulation* sim; };

static void* scheduler_thread(void* vp) {
    ThreadArgs* args = reinterpret_cast<ThreadArgs*>(vp);
    args -> sim -> run();
    args -> sim -> print_stats_and_finish();
    args -> sim -> shared -> done.store(true);
    return nullptr;
}

int main(int argc, char** argv) {
    Options opt = parse_args(argc, argv);
    auto lines = read_bursts(opt.file);

    // Echo input
    for (size_t i = 0; i < lines.size(); ++ i) {
        log_process_bursts((unsigned int*)lines[i].bursts.data(), lines[i].bursts.size());
    }

    Shared shared; Simulation sim(opt, &shared);
    sim.init_from_lines(lines);

    pthread_t th;
    ThreadArgs ta{ &sim };
    int rc = pthread_create(&th, NULL, scheduler_thread, &ta);
    if (rc != 0) {
        std::perror("pthread_create");
        return 1;
    }

    // Busy wait (explicitly required by the spec). No pthread_join
    while (!shared.done.load()) {
        // Small sleep to avoid burning CPU in real environment
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Main exits
    return 0;
}
