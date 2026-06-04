# CPU Scheduling Simulator Report

## 1 Design and OS Concepts

### Program Structure

```text
.
├── src
│   ├── main.c
│   ├── process.c/h            - process data
│   ├── parser.c/h             - input parsing
│   ├── scheduler.c/h          - algorithm selection
│   ├── gantt.c/h              - Gantt chart
│   ├── metrics.c/h            - metric calculation
│   ├── cli.c/h                - command-line options
│   └── algorithm/
│       ├── fcfs.c/h
│       ├── sjf.c/h
│       ├── srtf.c/h
│       ├── rr.c/h
│       └── priority.c/h       - priority with aging
├── tests
│   ├── test_runner.c          - automated tests
│   ├── basic.txt
│   ├── idle.txt
│   ├── same_arrival.txt
│   ├── same_burst.txt
│   ├── rr_quantum.txt
│   ├── srtf_preempt.txt
│   ├── invalid_workload.txt
│   └── priority_aging.txt
├── README.md
├── REPORT.md
└── Makefile
```

### FCFS

My understanding:

- FCFS is a simple non-preemptive scheduling rule.
- In my view, it works like a simple queue: the process that arrives first gets served first.
- After a process starts running, it keeps the CPU until its burst is complete.
- This makes the result simple to trace.
- Its weakness is the convoy effect, where one long process near the front can delay several shorter processes behind it.

In my program:

- I scan the process list for an arrived unfinished process with the earliest arrival time.
- PID order is used when two processes arrive together, so repeated runs produce the same schedule.
- If the CPU has nothing ready to run, the timeline records `IDLE` and time moves to the next arrival.
- For the selected process, I record its start time, run the whole burst, then record its finish time.
- The metrics are not guessed inside FCFS; they are calculated later from the recorded start and finish values.

### SJF

My understanding:

- SJF is non-preemptive, but its choice is based on burst length instead of only arrival order.
- When the CPU becomes available, I understand it as choosing the ready process with the smallest burst time.
- This can lower the average waiting time because short jobs can finish without waiting behind larger jobs.
- The weakness is that SJF cannot stop a running process, even if a shorter one arrives later.
- A long process may also be delayed for a long time if shorter jobs are always ready.

In my program:

- At each scheduling point, I check all processes that have arrived and are still unfinished.
- The smallest burst value is the main selection rule.
- If burst values are equal, I compare arrival time, then PID.
- After a process is chosen, it runs to completion because this version of SJF is non-preemptive.
- If no process is ready, an `IDLE` segment is added before the next decision.

### SRTF

My understanding:

- SRTF is the preemptive form of shortest-job scheduling.
- Instead of using the original burst time only, it keeps checking how much time each ready process still needs.
- If a new process arrives with less remaining work than the current process, the CPU can switch to the new process.
- This can help short jobs get a faster response, but it also makes process changes more frequent.

In my program:

- I model SRTF in small time steps so that new arrivals can be considered immediately.
- During each step, I select the arrived unfinished process with the lowest remaining time.
- Equal remaining times are resolved by arrival time and then PID.
- When a different process becomes the best choice, the timeline records a new segment.
- A process start time is stored only once, on its first CPU allocation.
- Finish time is stored when `remaining` becomes zero.

### Round Robin

My understanding:

- Round Robin is preemptive and gives processes turns using a fixed time quantum.
- The ready queue decides the order, and the front process gets one limited CPU turn.
- If it finishes during the quantum, it leaves the queue.
- If it still has remaining time, it is placed back at the tail.
- This makes the CPU sharing feel fairer, although a very small quantum can create many switches.
- If the quantum is very large, the behaviour becomes close to FCFS because processes may finish before being interrupted.

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

I designed a C test runner in `tests/test_runner.c`.

- `check_gantt_line()` checks that the Gantt chart order matches the expected timeline.
- `check_summary_value()` checks summary values including `AVG_WAIT`, `AVG_TAT`, `AVG_RESP`, `CONTEXT_SWITCHES`, and `CPU_UTIL`.
- `check_nonzero_exit()` checks that invalid workload input exits with an error.

The test runner can be executed with:

```sh
make test
```

### Individual Checking

I used hand calculation to verify selected metric values from the program output.

- Read `Arrival`, `Burst`, `Start`, and `Finish` from the output table.
- `Response = Start - Arrival`.
- `Turnaround = Finish - Arrival`.
- `Waiting = Turnaround - Burst`.
- `CPU_UTIL = total busy time / makespan * 100%`.
- Compare these hand calculations with the values printed by the program.

This helped confirm that the Gantt chart order and the computed metrics were consistent.

After running these tests, I checked the Gantt chart order and manually verified selected metric values using hand calculations.

### Observations

- In `basic.txt`, FCFS can show the convoy effect from the lecture because a long early process may make shorter later processes wait.
- In `basic.txt`, SJF gives a lower average waiting time than FCFS because it chooses the shortest ready burst after `P1`.
- In `srtf_preempt.txt`, SRTF lets short jobs respond earlier by preempting a longer running process, but the Gantt chart has more process changes.
- In `rr_quantum.txt` with `q=1`, Round Robin gives quick first CPU access, but the small quantum creates many context switches.
- If the Round Robin quantum is very large, the result becomes closer to FCFS. If the quantum is too small, context switch overhead becomes more obvious.
- Priority scheduling can cause low-priority processes to wait for a long time, so aging helps reduce starvation by making waiting processes more competitive.

## 3 AI Usage Declaration

I used Codex and ChatGPT as supporting tools during this coursework.

AI helped me with:

- Suggestions for improving parts of my code.
- Finding possible weaknesses in my implementation.
- Debugging directions when I was unsure how to locate a problem.
- Edge-case workload ideas for testing.
- Code logic review and advice for parts I did not fully understand.
- Report clarity and wording.

The design, implementation decisions, and final verification were completed by me. I verified the program myself by:

- Building the project with `make`.
- Running workload files for FCFS, SJF, SRTF, RR, and priority scheduling.
- Checking the Gantt chart order.
- Manually confirming selected waiting time, turnaround time, response time, and CPU utilisation values.
- Testing invalid workload input.
