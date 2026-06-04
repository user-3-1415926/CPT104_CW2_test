# CPU Scheduling Simulator Report

## 1 Design and OS Concepts

### Program Structure

```text
.
├── src
│   ├── main.c                 - program entry point
│   ├── process.c/h            - process data structure and helper functions
│   ├── parser.c/h             - workload txt input and validation
│   ├── scheduler.c/h          - algorithm selection and shared scheduler helpers
│   ├── gantt.c/h              - Gantt chart timeline structure and output
│   ├── metrics.c/h            - waiting, turnaround, response and CPU statistics
│   ├── cli.c/h                - command-line input handling
│   └── algorithm/
│       ├── fcfs.c/h           - FCFS implementation
│       ├── sjf.c/h            - SJF implementation
│       ├── srtf.c/h           - SRTF implementation
│       ├── rr.c/h             - Round Robin implementation
│       └── priority.c/h       - Priority scheduling with aging
├── tests
│   ├── test_runner.c          - small automated test runner
│   ├── basic.txt              - normal scheduling workload
│   ├── idle.txt               - workload with CPU idle time
│   ├── same_arrival.txt       - same-arrival workload
│   ├── same_burst.txt         - same-burst workload
│   ├── rr_quantum.txt         - Round Robin quantum workload
│   ├── srtf_preempt.txt       - SRTF preemption workload
│   ├── invalid_workload.txt   - invalid input workload
│   └── priority_aging.txt     - priority aging workload
├── README.md
├── REPORT.md
└── Makefile
```

### FCFS

My understanding:

- FCFS is the most direct scheduling rule in this simulator.
- The earliest arrived ready process is selected first.
- After selection, the process keeps the CPU until its burst is complete.
- This makes the result simple to trace, but it can perform badly when a long process arrives before several short ones.

In my program:

- I scan the process list for an arrived unfinished process with the earliest arrival time.
- PID order is used when two processes arrive together, so repeated runs produce the same schedule.
- If the CPU has nothing ready to run, the timeline records `IDLE` and time moves to the next arrival.
- For the selected process, I record its start time, run the whole burst, then record its finish time.
- The metrics are not guessed inside FCFS; they are calculated later from the recorded start and finish values.

### SJF

My understanding:

- SJF is non-preemptive, but its choice is based on burst length instead of only arrival order.
- When the CPU is free, the shortest ready job is preferred.
- This can lower the average waiting time because small jobs are less likely to sit behind long jobs.
- The weakness is that SJF cannot stop a process once it has started, even if a shorter process arrives later.

In my program:

- At each scheduling point, I check all processes that have arrived and are still unfinished.
- The smallest burst value is the main selection rule.
- If burst values are equal, I compare arrival time, then PID.
- After a process is chosen, it runs to completion because this version of SJF is non-preemptive.
- If no process is ready, an `IDLE` segment is added before the next decision.

### SRTF

My understanding:

- SRTF is the preemptive form of shortest-job scheduling.
- Instead of comparing original burst time only, it compares remaining time.
- A newly arrived process can preempt the current one if it has less work left.
- This is useful for response and waiting time of short jobs, but it can increase the number of process changes.

In my program:

- I model SRTF in small time steps so that new arrivals can be considered immediately.
- During each step, I select the arrived unfinished process with the lowest remaining time.
- Equal remaining times are resolved by arrival time and then PID.
- When a different process becomes the best choice, the timeline records a new segment.
- A process start time is stored only once, on its first CPU allocation.
- Finish time is stored when `remaining` becomes zero.

### Round Robin

My understanding:

- Round Robin uses a ready queue and a fixed time quantum.
- Each process at the front of the queue gets a limited CPU turn.
- If it finishes during the quantum, it leaves the queue.
- If it still has remaining time, it is placed back at the tail.
- This makes the algorithm fair, although a very small quantum can create many switches.

In my program:

- I keep a ready queue of process indexes.
- Arrived processes are inserted into the queue before the next process is selected.
- Same-time arrivals are inserted in PID order to keep the result deterministic.
- The running length is `min(quantum, remaining time)`.
- After the slice, new arrivals are handled before requeueing the preempted process.
- This ordering prevents the old process from cutting ahead of a process that arrived during its turn.

### Priority Scheduling With Aging

My understanding:

- Priority scheduling chooses according to a priority value rather than burst length or queue position.
- In this program, a lower number means a higher priority.
- Aging is added so that a process waiting for a long time can gradually become easier to select.

In my program:

- The user selects it with `--alg PRIORITY`.
- Because priority is required for this algorithm, every workload row must include that field.
- I calculate an effective priority from the original priority and the time already spent waiting.
- The selected process then runs non-preemptively, and its completion time is recorded like FCFS and SJF.

### Tie-Breaking

Tie-breaking is important because the same input should always create the same Gantt chart and the same metrics.
Without a fixed rule, two processes with equal scheduling values could be chosen in different orders.

FCFS tie-breaking:

```text
FCFS selection
        |
        v
Compare arrival time
        |
        v
Tie?
        |
        +-- No --> Select earlier arrival
        |
        +-- Yes
              |
              v
      Smaller PID lexicographic order
              |
              v
          Select process
```

SJF, SRTF, and Priority tie-breaking:

```text
Scheduling choice
        |
        v
Compare main rule
SJF -> burst time
SRTF -> remaining time
PRIORITY -> effective priority
        |
        v
Tie?
        |
        +-- No --> Select best process
        |
        +-- Yes
              |
              v
      Smaller arrival time?
              |
              +-- Yes --> Select process
              |
              +-- Tie
                    |
                    v
          Smaller PID lexicographic order
                    |
                    v
              Select process
```

RR tie-breaking:

```text
Same arrival time
        |
        v
Sort by PID
        |
        v
Enqueue into ready queue
```

### Context Switch Counting

Context switch policy:

Counted:

```text
P1 -> P2     
P2 -> P3     
```

Not counted:

```text
IDLE -> P1   
P1 -> IDLE   
P1 -> P1     
```

Explanation:

- I define a context switch as a **direct change** from one process to a different process.
- `IDLE` is treated as no process running, so `IDLE -> P1` is not counted.
- `P1 -> IDLE` is also ignored.
- Adjacent segments with the same PID are merged by the timeline, so they cannot create a fake switch.

### Tricky Scenario: Arrival During RR Time Slice

A tricky case in Round Robin occurs when new processes arrive while another process is using its time quantum. If the running process has not finished after the quantum, the scheduler must decide whether the newly arrived processes or the old running process should be placed first in the ready queue.

In my program, newly arrived processes are added to the ready queue before the unfinished running process is requeued. For example, suppose `P1` is running, and `P2` and `P3` arrive during `P1`'s quantum. If `P1` is still not finished after the quantum, the queue after the quantum should be:

```text
[P2, P3, P1]
```

instead of:

```text
[P1, P2, P3]
```

This means the processes that became ready during the time slice get their turn before the process that has just used the CPU. This keeps Round Robin fair and prevents the same process from receiving two close CPU turns.


## 2 Testing Strategy

### Required Algorithm Tests

I tested the required algorithms separately: FCFS, SJF, SRTF, RR

I used `basic.txt` as the first normal-case workload:

```sh
./sched tests/basic.txt --alg FCFS
./sched tests/basic.txt --alg SJF
./sched tests/basic.txt --alg SRTF
./sched tests/basic.txt --alg RR --q 3
```

This checks:
- Gantt chart order.
- Start and finish values.
- Waiting time calculation.
- Turnaround time calculation.
- Response time calculation.
- Average metric output.

### Edge Case Testing

`basic.txt` -> FCFS, SJF, SRTF, RR

- Normal scheduling behavior.
- Gantt chart order should be correct.
- Finish time and average metrics should match the expected result.

`idle.txt` -> SRTF

- CPU has no ready process.
- Gantt chart should contain `IDLE`.

`same_arrival.txt` -> RR

- Multiple processes arrive at the same time.
- Ready queue should enqueue them by PID order.

`same_burst.txt` -> SJF / SRTF

- Processes have equal burst time.
- Tie-breaking should still choose a deterministic process.

`rr_quantum.txt` -> RR `q=1`

- Many short time slices are created.
- Re-enqueueing and context switch counting should be correct.

`srtf_preempt.txt` -> SRTF

- A shorter process arrives after another process has started.
- The current process should be preempted.

`invalid_workload.txt` -> parser

- Workload includes negative arrival, zero burst, or malformed input.
- Program should print an error and exit non-zero.

`priority_aging.txt` -> PRIORITY

- Processes use priority values.
- Aging should improve waiting processes over time.

### Automated Checking

I designed a small C test runner in `tests/test_runner.c`.

- `check_gantt_line()` checks that the Gantt chart order matches the expected timeline.
- `check_summary_value()` checks summary values including `AVG_WAIT`, `AVG_TAT`, `AVG_RESP`, `CONTEXT_SWITCHES`, and `CPU_UTIL`.
- `check_nonzero_exit()` checks that invalid workload input exits with an error.

The test runner can be executed with:

```sh
make test
```

### Metric Example

I used hand calculation to verify selected metric values from the program output.

The method was:

- Read `Arrival`, `Burst`, `Start`, and `Finish` from the output table.
- Calculate `Response = Start - Arrival`.
- Calculate `Turnaround = Finish - Arrival`.
- Calculate `Waiting = Turnaround - Burst`.
- Calculate `CPU_UTIL = total busy time / makespan * 100%`.
- Compare these hand calculations with the values printed by the program.

This helped confirm that the Gantt chart order and the computed metrics were consistent.

### Observations

- In `basic.txt`, SJF gives a lower average waiting time than FCFS because it runs the shortest ready job after `P1`.
- In `srtf_preempt.txt`, SRTF lets short jobs finish earlier, but the Gantt chart has more process changes.
- In `rr_quantum.txt` with `q=1`, Round Robin gives quick first responses, but it creates many context switches.
- Priority scheduling depends strongly on the priority values, and aging helps waiting processes become more competitive.

## 3 AI Usage Declaration

I used Codex as a supporting tool during this coursework.

AI helped me with:

- Debugging directions.
- Edge-case workload ideas.
- Code logic review.
- Report clarity and wording.

The design, implementation decisions, and final verification were completed by me. I verified the program myself by:

- Building the project with `make`.
- Running workload files for FCFS, SJF, SRTF, RR, and priority scheduling.
- Checking the Gantt chart order.
- Manually confirming selected waiting time, turnaround time, response time, and CPU utilisation values.
- Testing invalid workload input.
