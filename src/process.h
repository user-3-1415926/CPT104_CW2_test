#ifndef PROCESS_H
#define PROCESS_H

#define PID_LEN 32
#define IDLE_LABEL "IDLE"

typedef struct {
    char pid[PID_LEN];
    int arrival;
    int burst;
    int priority;
    int has_priority;
    int remaining;
    int started;
    int finished;
    int start;
    int finish;
    int queued;
} Process;

void die(const char *message);
void die_line(int line_no, const char *message);
Process *copy_processes(const Process *processes, int n);
int ready_tie_less(const Process *a, const Process *b);
void sort_for_output(Process *p, int n);

#endif
