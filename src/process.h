#ifndef PROCESS_H
#define PROCESS_H

#define PID_LEN 32
#define IDLE_LABEL "IDLE"

/**
 * Stores one workload entry and the values produced by a scheduler.
 */
typedef struct {
    char pid[PID_LEN];
    int arrival;
    int burst;
    int priority;
    int has_priority;

    // runtime state
    int remaining;      // remaining burst time
    int started;        // 1 after the first run
    int finished;       // 1 after completion
    int start;          // first start time
    int finish;         // completion time
    int waiting;        // turnaround - burst
    int turnaround;     // finish - arrival
    int response;       // start - arrival
    int completed;      // same meaning as finished, kept for coursework wording
    int queued;         // used by RR ready queue
} Process;

void die(const char *message);
void die_line(int line_no, const char *message);
Process *copy_processes(const Process *processes, int n);
int ready_tie_less(const Process *a, const Process *b);
void sort_for_output(Process *p, int n);

#endif
