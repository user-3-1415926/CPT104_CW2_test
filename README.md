# CPT104 Coursework 2 - CPU Scheduling Simulator

This project implements a CPU scheduling simulator in C for CPT104 Coursework 2.

## Build

```sh
make clean
make
```

The executable is created in the project root as `sched`.

## Usage

```sh
./sched --demo
./sched tests/workload1.txt --alg FCFS
./sched tests/workload1.txt --alg SJF
./sched tests/workload1.txt --alg SRTF
./sched tests/workload1.txt --alg RR --q 3
./sched tests/workload1.txt --alg SRTF --trace
```

`--alg` is required unless `--demo` is used. `--q` is required for Round Robin and must be a positive integer. `--trace` prints step-by-step scheduling decisions.

## Workload Format

Each non-comment line has this format:

```text
PID ARRIVAL BURST [PRIORITY]
```

Example:

```text
P1 0 7
P2 2 4
P3 4 1
P4 6 4
```

Blank lines are ignored. Lines starting with `#` are ignored. Invalid lines print a clear error and exit with a non-zero status.

## Implemented Algorithms

The required algorithms are implemented:

```text
FCFS  - First Come First Served, non-preemptive
SJF   - Shortest Job First, non-preemptive
SRTF  - Shortest Remaining Time First, preemptive
RR    - Round Robin, preemptive
```

## Tie-Breaking Rules

The simulator uses deterministic tie-breaking:

```text
1. Smaller arrival time
2. Smaller PID using lexicographic string comparison
```

Algorithm-specific comparisons are:

```text
SJF   compares burst time first, then applies the tie-break rules.
SRTF  compares remaining time first, then applies the tie-break rules.
RR    uses a FIFO ready queue. Processes arriving at the same time are enqueued by PID order.
```

## Context Switch Definition

This implementation counts a context switch only when the CPU changes directly from one process to a different process. Transitions involving `IDLE` are not counted. For example, `P1 -> P2` counts as one switch, while `IDLE -> P1` and `P1 -> IDLE` do not.

## Output

Each run prints:

```text
GANTT:
...

PID     Arrival Burst Start Finish Waiting Turnaround Response
...

RESULT: OK
ALG=...
AVG_WAIT=...
AVG_TAT=...
AVG_RESP=...
CONTEXT_SWITCHES=...
CPU_UTIL=...%
```

`CPU_UTIL` is calculated as:

```text
total CPU busy time / makespan * 100%
```

where makespan is the end time of the last Gantt segment.
