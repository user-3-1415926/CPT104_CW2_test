# CPT104 Coursework 2 - CPU Scheduling Simulator

This project implements a CPU scheduling simulator in C for CPT104 Coursework 2.

## Build

```sh
make clean && make
```

The executable is created in the project root as `sched`.

## Usage

```sh
./sched --demo
./sched tests/workloads/basic.txt --alg FCFS
./sched tests/workloads/basic.txt --alg SJF
./sched tests/workloads/basic.txt --alg SRTF
./sched tests/workloads/basic.txt --alg RR --q 3
./sched tests/workloads/basic.txt --alg SRTF --trace
./sched tests/workloads/priority_aging.txt --alg PRIORITY
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

Blank lines are ignored. Lines starting with `#` are ignored. The optional `PRIORITY` value is used by the `PRIORITY` algorithm and ignored by FCFS, SJF, SRTF, and RR. Invalid lines print a clear error and exit with a non-zero status.

## Implemented Algorithms

The required algorithms are implemented:

```text
FCFS  - First Come First Served, non-preemptive
SJF   - Shortest Job First, non-preemptive
SRTF  - Shortest Remaining Time First, preemptive
RR    - Round Robin, preemptive
PRIORITY - Priority Scheduling with Aging
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
PRIORITY compares effective priority first, then applies the tie-break rules.
```

For `PRIORITY`, every workload line must include the priority field.
Smaller priority numbers run first. Aging improves a waiting process by reducing
its effective priority by 1 for every 5 time units waited.

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

## Testing

Test input files are stored in `tests/workloads/`.

Compile first:

```sh
make clean && make
```

Required algorithm tests:

```sh
./sched tests/workloads/basic.txt --alg FCFS
./sched tests/workloads/basic.txt --alg SJF
./sched tests/workloads/basic.txt --alg SRTF
./sched tests/workloads/basic.txt --alg RR --q 3
```

Edge case tests:

```sh
./sched tests/workloads/idle.txt --alg SRTF
./sched tests/workloads/same_arrival.txt --alg RR --q 2
./sched tests/workloads/same_burst.txt --alg SJF
./sched tests/workloads/rr_quantum.txt --alg RR --q 1
./sched tests/workloads/srtf_preempt.txt --alg SRTF
```

Invalid input test:

```sh
./sched tests/workloads/invalid_workload.txt --alg FCFS
```

This test is expected to fail with a clear error message and a non-zero exit code.

Priority scheduling test:

```sh
./sched tests/workloads/priority_aging.txt --alg PRIORITY
```

| Test file | Purpose |
|---|---|
| basic.txt | Normal scheduling case |
| idle.txt | CPU idle period |
| same_arrival.txt | Same arrival time and PID tie-breaking |
| same_burst.txt | Equal burst time tie-breaking |
| rr_quantum.txt | Round Robin quantum behavior |
| srtf_preempt.txt | SRTF preemption |
| invalid_workload.txt | Parser error handling |
| priority_aging.txt | Priority scheduling with aging |

The invalid workload contains a negative arrival time, a zero burst time, and a malformed line. The program stops at the first invalid line and exits with an error.
