// ============================================================
// Étape 2 — Analyse de schedulabilité
// ============================================================



// ============================================================
// We verify that the task set can be scheduled without missing
// any deadline under *non-preemptive* fixed-priority scheduling
// (Rate Monotonic order: shortest period = highest priority)
//
// Two complementary analyses are performed:
//   1. Utilisation test  – necessary condition: U < 1
//   2. Non-preemptive Response Time Analysis (RTA) – sufficient
//      condition: each task's worst-case response time R_i ≤ D_i
//
// RTA formula (non-preemptive):
//   R_i^(0)   = C_i + B_i
//   R_i^(k+1) = C_i + B_i + Σ_{j<i} ceil(R_i^(k) / T_j) * C_j
//
//   where B_i = max_{j>i} C_j  (blocking by a lower-priority task
//   that started just before the current task was released)
//
// The iteration converges to a fixed point; if R_i > D_i at any
// point, the task misses its deadline.
// ============================================================



#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <cmath>



// Task descriptor: name, WCET (C), period (T), deadline (D), RM priority
struct Task {
    std::string name;
    double C;
    int    T;
    int    D;
    int    priority;
};


// --- Helper: greatest common divisor (Euclidean algorithm) ---
static long long gcd2(long long a, long long b) { return b == 0 ? a : gcd2(b, a % b); }


// --- Helper: least common multiple (used to compute the hyperperiod) ---
static long long lcm2(long long a, long long b) { return a / gcd2(a, b) * b; }



// ---------------------------------------------------------------
// Non-preemptive RTA for task at position 'idx' in the task array
// (tasks must be sorted by decreasing priority, i.e. index 0 = highest)
// Returns the worst-case response time R_i
// ---------------------------------------------------------------

static double rta_nonpreemptive(const std::vector<Task>& tasks, int idx) {


    // --- Blocking term B_i ---
    // In non-preemptive scheduling, task i may be delayed by one full
    // execution of any lower-priority task that started just before it
    // We take the worst (longest) such task
    double B = 0.0;
    for (int j = idx + 1; j < (int)tasks.size(); ++j)
        B = std::max(B, tasks[j].C);


    // --- Fixed-point iteration ---
    // Start with the minimum possible response time (no interference yet)
    double R = tasks[idx].C + B;
    while (true) {

        // Recompute R by adding the interference from every higher-priority task j:
        // ceil(R / T_j) is the maximum number of times task j can preempt (or
        // in this non-preemptive model: the number of jobs of j that fall within
        // the busy window of length R), each costing C_j
        double R_next = tasks[idx].C + B;
        for (int j = 0; j < idx; ++j)
            R_next += std::ceil(R / tasks[j].T) * tasks[j].C;


        // Convergence check (floating-point safe epsilon)
        if (std::abs(R_next - R) < 1e-9) break;
        R = R_next;

        // Early exit: if R already exceeds the deadline, the task will miss it
        if (R > tasks[idx].D) return R;
    }
    return R;
}

void run_step2() {


    // --- Load C1 measured in Step 1 ---
         // C1 is read from file; if the file is absent, default to 1.0
         // Either way we normalise to 1 abstract unit because
         // C1 (≈ 60 ns) ≪ T1 (10 ms), so it is negligible relative to the period
    double C1 = 1.0;
    {
        std::ifstream f("c1_value.txt");
        if (f.is_open()) f >> C1;
        C1 = 1.0; // Normalised: C1 << T1, so we assign 1 abstract unit
    }


    // --- Task set (sorted by RM priority: shortest period first) ---
    std::vector<Task> tasks = {
                               {"tau1", C1, 10, 10, 1},  // highest priority
                               {"tau2", 3,  10, 10, 2},
                               {"tau3", 2,  20, 20, 3},
                               {"tau4", 2,  20, 20, 4},
                               {"tau5", 2,  40, 40, 5},
                               {"tau6", 2,  40, 40, 6},
                               {"tau7", 3,  80, 80, 7},  // lowest priority
                               };

    std::cout << "============================================\n";
    std::cout << "  SCHEDULABILITY ANALYSIS\n";
    std::cout << "============================================\n\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "C1 (WCET tau1) = " << C1 << " units\n\n";


    // ---------------------------------------------------------------
    // 1) Utilisation test
    //    U = Σ C_i / T_i  must be < 1 (necessary condition for any
    //    work-conserving scheduler)
    //    The Liu & Layland bound U_LL = n*(2^(1/n)-1) is the exact
    //    threshold for preemptive RM; we display it for reference
    //    but we do not use it as a hard limit here
    // ---------------------------------------------------------------

    double U = 0.0;
    for (auto& t : tasks) U += t.C / t.T;
    double U_bound = tasks.size() * (std::pow(2.0, 1.0 / tasks.size()) - 1.0);

    std::cout << std::left
              << std::setw(6)  << "Task"
              << std::setw(8)  << "C"
              << std::setw(8)  << "T"
              << std::setw(8)  << "D"
              << "U_i\n";
    for (auto& t : tasks)
        std::cout << std::setw(6)  << t.name
                  << std::setw(8)  << t.C
                  << std::setw(8)  << t.T
                  << std::setw(8)  << t.D
                  << t.C / t.T << "\n";

    std::cout << "\nTotal utilisation U = " << U << "\n";
    std::cout << "Liu & Layland bound  = " << U_bound << "\n";

    // If U > 1 the system is trivially unschedulable (processor overloaded)
    std::cout << (U <= 1.0 ? "=> U <= 1 : necessary condition OK\n" : "=> U > 1 : NOT SCHEDULABLE\n");


    // ---------------------------------------------------------------
    // 2) Hyperperiod
    //    H = lcm(T_i) is the length of one full repeating cycle
    //    It is sufficient to analyse a single window of length H
    // ---------------------------------------------------------------

    long long H = 1;
    for (auto& t : tasks) H = lcm2(H, (long long)t.T);
    std::cout << "\nHyperperiod H = " << H << "\n";


    // ---------------------------------------------------------------
    // 3) Non-preemptive Response Time Analysis
    //    For each task, compute its worst-case response time R_i
    //    The system is schedulable iff R_i <= D_i for every task
    // ---------------------------------------------------------------

    std::cout << "\n-- Response Time Analysis (non-preemptive) --\n";
    std::cout << std::left
              << std::setw(6)  << "Task"
              << std::setw(12) << "R_i"
              << std::setw(8)  << "D_i"
              << "Status\n";
    std::cout << std::string(34, '-') << "\n";

    bool all_ok = true;
    for (int i = 0; i < (int)tasks.size(); ++i) {
        double R = rta_nonpreemptive(tasks, i);
        bool ok = (R <= tasks[i].D);
        if (!ok) all_ok = false;
        std::cout << std::setw(6)  << tasks[i].name
                  << std::setw(12) << R
                  << std::setw(8)  << tasks[i].D
                  << (ok ? "OK" : "MISSED!") << "\n";
    }

    // Final verdict
    std::cout << "\n=> Systeme " << (all_ok ? "SCHEDULABLE" : "NON SCHEDULABLE") << "\n";

    // Algorithmic complexity reminder:
    // Each of the n tasks requires at most log(H / C_min) iterations of the
    // inner loop (which itself runs in O(n)), giving O(n^2 * log(H/C_min))
    std::cout << "\nComplexite : O(n^2 * log(H/C_min)), n=" << tasks.size() << ", H=" << H << "\n";
}
