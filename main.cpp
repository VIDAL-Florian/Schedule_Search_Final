#include <iostream>

using namespace std;

#include <iostream>


// Declaration of each step's entry-point function
void run_step1();
void run_step2();
void run_step3();

int main() {
    std::cout << "========================================\n";
    std::cout << "   SCHEDULE SEARCH - VIDAL Florian\n";
    std::cout << "========================================\n\n";


    // --- Step 1: Experimentally measure the WCET of task tau1 ---
    // We time 10,000 batches of 1,000 multiplications and keep the worst observed time
    std::cout << ">>> STEP 1 : Measuring the WCET of tau1\n\n";
    run_step1();


    // --- Step 2: Formal schedulability analysis ---
    // Utilisation test + non-preemptive Response Time Analysis (RTA)
    std::cout << "\n>>> STEP 2 : Schedulability analysis\n\n";
    run_step2();


    // --- Step 3: Build a concrete non-preemptive EDF schedule ---
    // Schedule 1: all deadlines met
    // Schedule 2: tau5's deadline is relaxed to observe the effect
    std::cout << "\n>>> STEP 3 : Non-preemptive scheduler\n\n";
    run_step3();

    return 0;
}
