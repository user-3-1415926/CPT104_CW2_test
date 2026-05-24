#include "srtf.h"

#include "scheduler.h"

#include <stdio.h>

static int select_srtf(const Process *p, int n, int time) {
    int best = -1;
    for (int i = 0; i < n; i++) {
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

    while (completed < n) {
        int selected = select_srtf(p, n, time);
        if (selected == -1) {
            int next = next_arrival_time(p, n, time);
            if (next == -1) {
                die("internal scheduling error");
            }
            if (trace) {
                printf("TRACE %d-%d: IDLE\n", time, next);
            }
            timeline_add(timeline, IDLE_LABEL, time, next);
            time = next;
            continue;
        }

        if (!p[selected].started) {
            p[selected].started = 1;
            p[selected].start = time;
        }
        if (trace) {
            printf("TRACE %d-%d: %s remaining_before=%d\n",
                time, time + 1, p[selected].pid, p[selected].remaining);
        }
        timeline_add(timeline, p[selected].pid, time, time + 1);
        p[selected].remaining--;
        time++;

        if (p[selected].remaining == 0) {
            p[selected].finish = time;
            p[selected].finished = 1;
            completed++;
        }
    }
}
