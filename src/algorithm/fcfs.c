#include "fcfs.h"

#include "../scheduler.h"

#include <stdio.h>

static int select_fcfs(const Process *p, int n, int time) {
    int best = -1;

    for (int i = 0; i < n; i++) {
        // FCFS takes the ready job that arrived first
        if (!p[i].finished && p[i].arrival <= time) {
            if (best == -1 || ready_tie_less(&p[i], &p[best])) {
                best = i;
            }
        }
    }
    return best;
}

void run_fcfs(Process *p, int n, Timeline *timeline, int trace) {
    int completed = 0;
    int time = 0;

    if (trace) {
        fprintf(stderr, "--- FCFS trace begins ---\n");
    }

    while (completed < n) {
        int selected = select_fcfs(p, n, time);

        // no process can run at this time, jump to the next arrival
        if (selected == -1) {
            int next = next_arrival_time(p, n, time);
            if (next == -1) {
                die("internal scheduling error");
            }
            if (trace) {
                fprintf(stderr, "[Time %4d] processor waits for next arrival\n", time);
            }
            timeline_add(timeline, IDLE_LABEL, time, next);
            time = next;
            continue;
        }

        // non-preemptive job starts
        p[selected].started = 1;
        p[selected].start = time;

        if (trace) {
            fprintf(stderr, "[Time %4d] %s is dispatched\n", time, p[selected].pid);
        }

        // FCFS keeps the CPU until the selected job completes
        timeline_add(timeline, p[selected].pid, time, time + p[selected].burst);
        time += p[selected].burst;
        p[selected].remaining = 0;
        p[selected].finish = time;
        p[selected].finished = 1;
        completed++;

        if (trace) {
            fprintf(stderr, "[Time %4d] %s completes\n", time, p[selected].pid);
        }
    }

    if (trace) {
        fprintf(stderr, "--- FCFS trace ends ---\n\n");
    }
}
