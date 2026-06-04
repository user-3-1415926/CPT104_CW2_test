#include "srtf.h"

#include "../scheduler.h"

#include <stdio.h>

static int select_srtf(const Process *p, int n, int time) {
    int best = -1;

    for (int i = 0; i < n; i++) {
        // re-evaluate every tick using the remaining burst
        if (!p[i].finished && p[i].arrival <= time && p[i].remaining > 0) {
            if (best == -1 ||
                p[i].remaining < p[best].remaining ||
                (p[i].remaining == p[best].remaining && ready_tie_less(&p[i], &p[best]))) {
                best = i;
            }
        }
    }
    return best;
}


void run_srtf(Process *p, int n, Timeline *timeline, int trace) {
    int completed = 0;
    int time = 0;

    if (trace) {
        fprintf(stderr, "--- SRTF trace begins ---\n");
    }

    while (completed < n) {
        int selected = select_srtf(p, n, time);

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

        // response time uses the first moment this job receives CPU
        if (!p[selected].started) {
            p[selected].started = 1;
            p[selected].start = time;
        }

        if (trace) {
            fprintf(stderr, "[Time %4d] %s executes for 1 unit, remaining=%d\n",
                time, p[selected].pid, p[selected].remaining);
        }

        // advance by one tick to allow a later preemption check
        timeline_add(timeline, p[selected].pid, time, time + 1);
        p[selected].remaining--;
        time++;

        if (p[selected].remaining == 0) {
            p[selected].finish = time;
            p[selected].finished = 1;
            completed++;

            if (trace) {
                fprintf(stderr, "[Time %4d] %s completes\n", time, p[selected].pid);
            }
        }
    }

    if (trace) {
        fprintf(stderr, "--- SRTF trace ends ---\n\n");
    }
}
