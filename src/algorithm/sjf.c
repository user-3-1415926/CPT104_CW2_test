#include "sjf.h"

#include "../scheduler.h"

#include <stdio.h>

static int select_sjf(const Process *p, int n, int time) {
    int best = -1;

    for (int i = 0; i < n; i++) {
        // among ready jobs, prefer the smallest total CPU burst
        if (!p[i].finished && p[i].arrival <= time) {
            if (best == -1 ||
                p[i].burst < p[best].burst ||
                (p[i].burst == p[best].burst && ready_tie_less(&p[i], &p[best]))) {
                best = i;
            }
        }
    }
    return best;
}

void run_sjf(Process *p, int n, Timeline *timeline, int trace) {
    int completed = 0;
    int time = 0;

    if (trace) {
        fprintf(stderr, "--- SJF trace begins ---\n");
    }

    while (completed < n) {
        int selected = select_sjf(p, n, time);

        // no process can run yet
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

        // SJF is non-preemptive, so this is also the only start
        p[selected].started = 1;
        p[selected].start = time;

        if (trace) {
            fprintf(stderr, "[Time %4d] %s is dispatched\n", time, p[selected].pid);
        }

        // let the chosen shortest job finish before selecting again
        timeline_add(timeline, p[selected].pid, time, time + p[selected].burst);
        time += p[selected].burst;
        p[selected].remaining = 0;
        p[selected].finish = time;
        p[selected].finished = 1;
        p[selected].completed = 1;
        completed++;

        if (trace) {
            fprintf(stderr, "[Time %4d] %s completes\n", time, p[selected].pid);
        }
    }

    if (trace) {
        fprintf(stderr, "--- SJF trace ends ---\n\n");
    }
}
