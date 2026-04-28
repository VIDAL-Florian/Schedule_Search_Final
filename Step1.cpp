// ============================================================
// STEP 1 : Measuring the WCET of tau1
// ============================================================


// ============================================================
// Goal: determine the Worst-Case Execution Time of task tau1,
// which multiplies two random 64-bit integers.
//
// Problem: std::chrono on Windows has ~100 ns resolution, but a
// single 64-bit multiplication takes only ~4-5 ns.
//
// Solution: measure a *batch* of BATCH=1000 multiplications at
// once, then divide the elapsed time by BATCH. This brings the
// effective uncertainty down to ~0.1 ns.
//
// We repeat this for N_RUNS=10,000 runs and keep the maximum
// observed time as a conservative WCET estimate (worst case).
// ============================================================



#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#ifndef _BYTE_DEFINED
#define _BYTE_DEFINED
#endif

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <iomanip>
#include <fstream>
#include <cstdint>
#include <chrono>

// Number of independent measurement runs
static constexpr int N_RUNS = 10000;

// Number of multiplications per batch (amortises clock granularity)
static constexpr int BATCH  = 1000;

// Upper bound for the random operands: values in [2^61, 2^62)
// Using large numbers exercises the full 64-bit multiplier
static constexpr uint64_t LARGE_MAX = (1ULL << 62);


// Returns the current wall-clock time in nanoseconds
// std::chrono::high_resolution_clock is the finest clock available
// in the C++ standard library
static double get_time_ns() {
    auto tp = std::chrono::high_resolution_clock::now();
    return (double)std::chrono::duration_cast<std::chrono::nanoseconds>(
               tp.time_since_epoch()).count();
}

void run_step1() {
    std::cout << "Batch size : " << BATCH << " multiplications by measure\n\n";


    // Fixed seed (42) ensures the same operand pairs across runs (reproducibility)
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<uint64_t> dist(LARGE_MAX / 2, LARGE_MAX);


    // Pre-generate operand arrays so array filling is not included in the timing
    std::vector<uint64_t> A(BATCH), B(BATCH);
    for (int i = 0; i < BATCH; ++i) { A[i] = dist(rng); B[i] = dist(rng); }


    std::vector<double> times;
    times.reserve(N_RUNS);


    // 'volatile' prevents the compiler from optimising away the multiplication loop,
    // which would give artificially zero execution times
    volatile uint64_t sink = 0;


    // --- Main measurement loop ---
    for (int run = 0; run < N_RUNS; ++run) {

        double t0 = get_time_ns();   // start timestamp
        for (int i = 0; i < BATCH; ++i) sink = A[i] * B[i];  // timed work
        double t1 = get_time_ns();   // stop timestamp

        // Per-multiplication time (ns): divide by batch size to average out noise
        times.push_back((t1 - t0) / BATCH);
    }


    // Sort to compute order statistics efficiently
    std::sort(times.begin(), times.end());
    int n = (int)times.size();


    // Linear interpolation between adjacent samples for smooth percentile estimates
    auto percentile = [&](double p) -> double {
        double idx = p * (n - 1);
        int lo = (int)idx, hi = lo + 1;
        if (hi >= n) return times[n - 1];
        return times[lo] + (idx - lo) * (times[hi] - times[lo]);
    };

    double tmin = times[0]; // This is the best optimal execution time
    double tmax = times[n - 1]; // This is the worst-case execution time, stored as C1
    double q1   = percentile(0.25);
    double q2   = percentile(0.50);
    double q3   = percentile(0.75);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "===== WCET measurement of tau1 (" << N_RUNS << " runs x " << BATCH << ") =====\n";
    std::cout << "  Min  : " << tmin << " ns\n";
    std::cout << "  Q1   : " << q1   << " ns\n";
    std::cout << "  Q2   : " << q2   << " ns\n"; // Median
    std::cout << "  Q3   : " << q3   << " ns\n";
    std::cout << "  Max  : " << tmax << " ns\n";

    // The maximum is kept as C1 (conservative WCET): it captures OS interrupts
    // that may delay the task in a real deployment
    std::cout << "  WCET : " << tmax << " ns  <-- C1\n";


    // Save C1 to disk so Step 2 and Step 3 can read it
    std::ofstream out("c1_value.txt");
    out << tmax << "\n";
    std::cout << "\nC1 = " << tmax << " ns save in c1_value.txt\n";
}
