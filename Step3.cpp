// ============================================================
// Étape 3 —  STEP 3 : Non-preemptive scheduler
// ============================================================



// ============================================================
// We build a concrete schedule over one hyperperiod H = 80
//
// Algorithm: Earliest Deadline First (EDF), non-preemptive
//   - At each moment the processor becomes free, scan all jobs
//     whose arrival time <= current time and pick the one with
//     the earliest absolute deadline
//   - If no job is ready, advance time to the next arrival
//   - A job, once started, always runs to completion (non-preemptive)
//
// Two schedules are produced:
//   Schedule 1 – standard EDF: all tasks keep their real deadlines
//   Schedule 2 – relaxed EDF: tau5's deadline is set to +∞ so EDF
//                             deprioritises it; we then check
//                             whether it still meets its real deadline
//
// Metrics reported for each schedule:
//   - Per-job waiting time  (start − arrival)
//   - Total waiting time    (sum of all waiting times)
//   - Total execution time  (sum of all WCETs = fixed by U)
//   - Total idle time       (H − total execution time)
//   - Missed deadlines      (finish > deadline ?)
// ============================================================





#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <string>
#include <climits>
#include <cmath>






// One job instance of a periodic task
struct Job {
    std::string task_name;   // Release time (k * T)
    double arrival;   // Absolute deadline (arrival + D); may be INT_MAX for tau5 in Schedule 2
    double deadline;   // Worst-case execution time (C_i)
    double wcet;   // Time at which the job starts executing
    double start   = -1;   // Time at which the job finishes (start + wcet)
    double finish  = -1;   // Time at which the job finishes (start + wcet)
    double waiting = 0;   // start − arrival
    bool   missed  = false;   // True if finish > real deadline
};



// ---------------------------------------------------------------
// Generate all job instances for one hyperperiod.
//
// 'relax_tau5': if true, tau5's deadline field is set to INT_MAX
//               so EDF treats it as the least urgent job.
// ---------------------------------------------------------------

static std::vector<Job> generate_jobs(double C1, long long H, bool relax_tau5) {
    struct TaskDef { std::string name; double C; int T; int D; };
    std::vector<TaskDef> tasks = {
                                  {"tau1", C1, 10, 10},
                                  {"tau2", 3,  10, 10},
                                  {"tau3", 2,  20, 20},
                                  {"tau4", 2,  20, 20},
                                  {"tau5", 2,  40, relax_tau5 ? INT_MAX : 40},
                                  {"tau6", 2,  40, 40},
                                  {"tau7", 3,  80, 80},
                                  };

    std::vector<Job> jobs;
    for (auto& t : tasks) {
        int count = H / t.T;   // Number of instances in one hyperperiod
        for (int k = 0; k < count; ++k) {
            Job j;
            j.task_name = t.name;
            j.arrival   = k * t.T;   // k-th release time
            j.deadline  = j.arrival + t.D;   // Absolute deadline
            j.wcet      = t.C;
            jobs.push_back(j);
        }
    }
    return jobs;
}


// ---------------------------------------------------------------
// Non-preemptive EDF scheduling algorithm.
//
// Iterates until every job has been executed exactly once.
// At each step:
//   1. Find the ready job (arrival <= time) with the smallest deadline.
//   2. If none is ready, jump to the next arrival (idle period).
//   3. Execute the chosen job to completion (time += wcet).
// ---------------------------------------------------------------


static void schedule_edf_np(std::vector<Job>& jobs) {
    int n = (int)jobs.size();
    std::vector<bool> done(n, false);  // Tracks which jobs are finished
    double time = 0.0;  // Current simulation time
    int scheduled = 0;  // Number of jobs completed so far

    while (scheduled < n) {

        // --- EDF selection: pick the ready job with the earliest deadline ---
        int best = -1;
        double best_dl = 1e18;
        for (int i = 0; i < n; ++i) {
            if (!done[i] && jobs[i].arrival <= time + 1e-9) {
                // 1e-9 tolerance handles floating-point rounding at exact arrivals
                if (jobs[i].deadline < best_dl) {
                    best_dl = jobs[i].deadline;
                    best = i;
                }
            }
        }

        if (best == -1) {

            // No job is ready yet → processor is idle
            // Advance time to the very next job arrival
            double next = 1e18;
            for (int i = 0; i < n; ++i)
                if (!done[i]) next = std::min(next, jobs[i].arrival);
            time = next;
            continue;
        }


        // --- Execute the selected job (non-preemptive: run to completion) ---
        jobs[best].start   = time;
        jobs[best].waiting = time - jobs[best].arrival;  // How long it waited in the queue
        jobs[best].finish  = time + jobs[best].wcet;

        // Check whether this job finished after its real deadline
        jobs[best].missed  = (jobs[best].finish > jobs[best].deadline + 1e-9);
        time = jobs[best].finish;
        done[best] = true;
        ++scheduled;
    }
}


// ---------------------------------------------------------------
// Print a formatted schedule table and summary statistics
// Jobs are displayed in chronological order of their start time
// ---------------------------------------------------------------

static void print_schedule(const std::vector<Job>& jobs,
                           const std::string& title, long long H) {
    std::cout << "\n========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================\n";
    std::cout << std::left
              << std::setw(6)  << "Task"
              << std::setw(9)  << "Arrival"
              << std::setw(8)  << "Start"
              << std::setw(8)  << "End"
              << std::setw(10) << "Deadline"
              << std::setw(9)  << "Wait"
              << "Status\n";
    std::cout << std::string(60, '-') << "\n";


    // Sort by start time for chronological display
    std::vector<const Job*> sorted;
    for (auto& j : jobs) sorted.push_back(&j);
    std::sort(sorted.begin(), sorted.end(),
              [](const Job* a, const Job* b){ return a->start < b->start; });


    double total_wait = 0, total_exec = 0;
    bool any_miss = false;
    for (auto* j : sorted) {
        if (j->missed) any_miss = true;
        total_wait += j->waiting;
        total_exec += j->wcet;


        std::cout << std::setw(6)  << j->task_name
                  << std::setw(9)  << j->arrival
                  << std::setw(8)  << j->start
                  << std::setw(8)  << j->finish
                  << std::setw(10) << (j->task_name == "tau5" && j->deadline > 1000
                                           ? j->arrival + 40 : j->deadline)
                  << std::setw(9)  << j->waiting
                  << (j->missed ? "MISSED!" : "OK") << "\n";
    }



    // --- Summary ---
    // total_exec is always Σ(H/T_i * C_i) = 59, regardless of scheduling order
    // total_idle = H - total_exec is therefore also constant (= 21)
    // Only total_wait changes between schedules
    double total_idle = H - total_exec;
    std::cout << std::string(60, '-') << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total wait : " << total_wait << "\n";
    std::cout << "Total execution : " << total_exec << "\n";
    std::cout << "Total idle : " << total_idle
              << "  (check: " << H << " - " << total_exec << " = " << total_idle << ")\n";
    std::cout << "Missed deadlines : " << (any_miss ? "YES" : "NO") << "\n";
}

void run_step3() {
    double C1 = 1.0;
    long long H = 80;

    // ---------------------------------------------------------------
    // Schedule 1: standard EDF, all tasks use their real deadlines
    // Expected: no missed deadline, total wait = 108, idle = 21
    // ---------------------------------------------------------------

    auto jobs1 = generate_jobs(C1, H, false);
    schedule_edf_np(jobs1);
    print_schedule(jobs1, "Schedule 1 - No missed deadline", H);


    // ---------------------------------------------------------------
    // Schedule 2: tau5's deadline is relaxed to +∞
    // EDF will schedule tau5 only when no other job is pending
    // After scheduling, re-evaluate tau5's missed flag against its
    // *real* deadline (arrival + 40) — stored deadline was INT_MAX
    // Expected: tau5 still meets its real deadline (enough idle time)
    // ---------------------------------------------------------------

    auto jobs2 = generate_jobs(C1, H, true);
    schedule_edf_np(jobs2);

    // Re-check tau5 against its actual deadline (not the relaxed +∞)
    for (auto& j : jobs2)
        if (j.task_name == "tau5")
            j.missed = (j.finish > j.arrival + 40 + 1e-9);
    print_schedule(jobs2, "Schedule 2 - tau5 deadline relaxed", H);
}
