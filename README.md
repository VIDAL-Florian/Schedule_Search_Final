# Schedule Search — VIDAL Florian

Final assignment — Real-time scheduling (C++)

## Description

This project implements a real-time scheduling analysis in three steps:

- **Step 1**: Experimental WCET measurement of task tau1 (64-bit multiplication)
- **Step 2**: Schedulability analysis — Utilisation test + non-preemptive Response Time Analysis (RTA)
- **Step 3**: Concrete non-preemptive EDF scheduler over one hyperperiod (H=80)

## Task set

| Task | C | T | D | Priority |
|------|---|---|---|----------|
| tau1 | 1 | 10 | 10 | 1 |
| tau2 | 3 | 10 | 10 | 2 |
| tau3 | 2 | 20 | 20 | 3 |
| tau4 | 2 | 20 | 20 | 4 |
| tau5 | 2 | 40 | 40 | 5 |
| tau6 | 2 | 40 | 40 | 6 |
| tau7 | 3 | 80 | 80 | 7 |

## Compilation

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Exécution

```bash
./Schedule_Search_Final
```

## Environnement

- Windows 10/11
- CMake 3.x
- MSVC ou MinGW (C++17)
