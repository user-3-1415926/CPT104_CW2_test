# CPT104 CW2 Design, Testing, and AI Usage

## 1 Design and OS Concepts

### FCFS

My understanding:
- Non-preemptive.
- Processes run in arrival order.
- If a long process arrives early, later short processes must wait.
- This can increase average waiting time.
- Simple, but not always efficient.

In my program:
- I select processes by arrival time.
- If arrival time is the same, I use PID lexicographic order.
- Once a process starts, it runs until its burst time finishes.
- If no process has arrived, I add an `IDLE` segment.
- I record start time and finish time.
- Waiting, turnaround, and response time are calculated from the final result.

### SJF

My understanding:
- Non-preemptive.
- The CPU chooses the shortest burst time from ready processes.
- It can reduce average waiting time compared with FCFS.
- It cannot interrupt a process after it starts.
- A short process arriving later must wait until the current process finishes.

In my program:
- When the CPU is free, I scan all arrived and unfinished processes.
- I select the process with the smallest burst time.
- If burst times are equal, I apply tie-breaking.
- Smaller arrival time is selected first.
- If arrival time is also equal, smaller PID is selected.
- If there is no ready process, I add an `IDLE` segment.

### SRTF

My understanding:
- Preemptive version of SJF.
- The CPU always runs the process with the shortest remaining time.
- A newly arrived short process can interrupt the current process.
- This can reduce waiting time for short jobs.
- It can also increase context switches.

In my program:
- I simulate time step by time step.
- At each time unit, I check all arrived processes.
- I choose the process with the smallest remaining time.
- If another process becomes shorter, preemption happens.
- I record start time only the first time a process gets the CPU.
- Finish time is recorded when remaining time becomes 0.

### Round Robin

My understanding:
- Preemptive.
- Uses a FIFO ready queue.
- Each process can run for at most one quantum.
- If it is not finished, it goes back to the end of the queue.
- It improves fairness and response time.
- A very small quantum may create more context switches.

In my program:
- I use a ready queue.
- Processes are enqueued when they arrive.
- If several processes arrive at the same time, I enqueue them by PID order.
- The running process executes for `min(quantum, remaining time)`.
- New arrivals during the time slice are added to the queue first.
- If the running process is not finished, it is re-enqueued after those new arrivals.

### Priority Scheduling With Aging

My understanding:
- This is a priority-based scheduling algorithm.
- I used it for the bonus part of the coursework.
- Smaller priority number means higher priority.
- The process with the best priority is selected first.
- Aging helps reduce starvation.
- A process that waits longer can get a better effective priority.

In my program:
- It is selected using `--alg PRIORITY`.
- Every workload line must include a priority value.
- I compare effective priority first.
- If priorities are equal, I use the same tie-breaking rule.
- Smaller effective priority is selected first.
- Aging is applied when the scheduler selects the next process.
- The effective priority is calculated from the current time and waiting time.
- This is my bonus algorithm instead of MLFQ.

Formula used in my code:

```text
already_run = burst - remaining
waited = current_time - arrival - already_run
effective_priority = base_priority - (waited / 5)
```

So a process that waits longer gets a smaller effective priority value. Since smaller priority numbers are better, aging makes a waiting process more likely to run. If two processes have the same effective priority, the scheduler uses the normal tie-breaking rule: earlier arrival time first, then smaller PID.

### Tie-Breaking

Tie-breaking flow:

```text
Scheduling choice
        |
        v
Compare main rule
FCFS: arrival time
SJF: burst time
SRTF: remaining time
        |
        v
Tie?
        |
        +-- No --> Select process
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
P1 -> P2     counted
P2 -> P3     counted
```

Not counted:

```text
IDLE -> P1   not counted
P1 -> IDLE   not counted
P1 -> P1     not counted
```

Explanation:
- I count a context switch only when the CPU changes from one process to another different process.
- I do not count `IDLE` to process.
- I do not count process to `IDLE`.
- This matches the implementation and README.

### Tricky Scenario: Arrival During RR Time Slice

Problem:
- A process is running.
- Another process arrives during its quantum.
- The queue order must stay fair.

Example:

```text
Running process: P1
New arrival: P2
P1 still not finished
```

Correct order:
- `P2` enters queue first.
- `P1` goes back after `P2`.

Queue example:

```text
Before: [P1 running]
During quantum: P2 arrives
After quantum: [P2, P1]
```

Program-style snippet:

```c
while (used < quantum && p[current].remaining > 0) {
    run_one_time_unit(current);
    time++;
    used++;

    add_new_arrivals(time, queue);
}

if (p[current].remaining > 0) {
    enqueue(queue, current);
}
```

Explanation:
- New arrivals are added before the running process is re-enqueued.
- This keeps FIFO order correct.
- It prevents the same process from running again too early.

## 2 Testing Strategy

### Required Algorithm Tests

I tested each required algorithm:

```text
FCFS
SJF
SRTF
RR
```

I used `basic.txt` for normal scheduling behavior:

```sh
./sched tests/workloads/basic.txt --alg FCFS
./sched tests/workloads/basic.txt --alg SJF
./sched tests/workloads/basic.txt --alg SRTF
./sched tests/workloads/basic.txt --alg RR --q 3
```

This checks:
- Gantt chart output.
- Start and finish times.
- Waiting time.
- Turnaround time.
- Response time.
- Average metrics.

### Edge Case Testing

`basic.txt`
-> FCFS, SJF, SRTF, RR
-> normal scheduling behavior
-> Gantt chart, finish time, average metrics

`idle.txt`
-> SRTF
-> CPU has no ready process
-> Gantt chart should contain `IDLE`

`same_arrival.txt`
-> RR
-> multiple processes arrive at the same time
-> enqueue by PID order

`same_burst.txt`
-> SJF / SRTF
-> equal burst time
-> check tie-breaking

`rr_quantum.txt`
-> RR `q=1`
-> many time slices
-> check re-enqueueing and context switches

`srtf_preempt.txt`
-> SRTF
-> shorter process arrives later
-> current process should be preempted

`invalid_workload.txt`
-> parser
-> negative arrival, zero burst, malformed line
-> program should print error and exit non-zero

`priority_aging.txt`
-> PRIORITY
-> bonus only
-> check priority aging behavior

### Testing Results

| Test file | Algorithm / Feature | Expected behaviour | Actual result | Status |
|---|---|---|---|---|
| basic.txt | FCFS | Processes run in arrival order | Gantt chart was `P1 -> P2 -> P3 -> P4`; averages printed correctly | Pass |
| basic.txt | SJF | Shortest ready job selected first | Gantt chart selected `P3` before `P2` after `P1` finished; metrics printed correctly | Pass |
| basic.txt | SRTF | Shortest remaining time selected at each time step | Output showed preemptive order `P1 -> P2 -> P3 -> P2 -> P4 -> P1` | Pass |
| basic.txt | RR q=3 | Processes rotate using quantum 3 | Output showed repeated time slices and re-enqueueing; context switches = 7 | Pass |
| idle.txt | SRTF | CPU idle periods are shown | Gantt chart contained `IDLE` from 0 to 3 and 5 to 8 | Pass |
| same_arrival.txt | RR q=2 | Same arrival time processes are enqueued by PID order | Gantt chart started `P1 -> P2 -> P3`, so PID order was used | Pass |
| same_burst.txt | SJF | Equal burst times use deterministic tie-breaking | Processes ran in arrival order with PID tie-breaking where needed | Pass |
| rr_quantum.txt | RR q=1 | Frequent time slicing and re-enqueueing | Output showed many 1-unit slices and context switches = 13 | Pass |
| srtf_preempt.txt | SRTF | Later shorter job preempts current job | Gantt chart showed `P1` preempted by `P2`, then `P3`, then `P4` | Pass |
| invalid_workload.txt | Parser | Invalid input rejected | Program printed `ARRIVAL must be a non-negative integer` and exited non-zero | Pass |
| priority_aging.txt | PRIORITY bonus | Priority aging changes effective priority after waiting | Trace showed effective priorities changing, for example `P2` base 8 became effective 6 | Pass |

### Metric Example

Example process:

```text
PID = P2
Arrival = 2
Burst = 4
Start = 7
Finish = 11
```

Response:

```text
Response = Start - Arrival
Response = 7 - 2 = 5
```

Turnaround:

```text
Turnaround = Finish - Arrival
Turnaround = 11 - 2 = 9
```

Waiting:

```text
Waiting = Turnaround - Burst
Waiting = 9 - 4 = 5
```

CPU utilisation example:

```text
Busy time = 16
Makespan = 16

CPU_UTIL = Busy time / Makespan * 100
CPU_UTIL = 16 / 16 * 100 = 100%
```

Average waiting time example:

```text
Waiting times = 0, 5, 7, 6
AVG_WAIT = (0 + 5 + 7 + 6) / 4
AVG_WAIT = 4.50
```

## 3 AI Usage Declaration

I used AI as a support tool during this coursework.

AI helped me with:
- Explaining scheduling concepts.
- Checking whether the project structure was clear.
- Suggesting debugging ideas.
- Suggesting edge case test workloads.
- Improving README and report wording.

I did not use AI as a replacement for testing.

I verified the code by:
- Compiling with `make`.
- Running FCFS, SJF, SRTF, and RR.
- Checking Gantt charts.
- Checking waiting, turnaround, and response time.
- Testing invalid input.
- Running the bonus priority test separately.

AI did not write the full solution for me. I used it to review and improve my own implementation.
