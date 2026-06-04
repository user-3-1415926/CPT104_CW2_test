#include "priority.h"

#include "../scheduler.h"

#include <stdio.h>

#define AGING_INTERVAL 5

static int effective_priority(const Process *p, int time) {
    // lower priority number -> higher scheduling priority
    int waited = time - p->arrival - (p->burst - p->remaining);
    if (waited < 0) {
        waited = 0;
    }
    return p->priority - (waited / AGING_INTERVAL);
}

static int priority_less(const Process *p, int a, int b, int time) {
    int ea = effective_priority(&p[a], time);
    int eb = effective_priority(&p[b], time);

    if (ea != eb) {
        return ea < eb;
    }
    return ready_tie_less(&p[a], &p[b]);
}

static int select_priority(const Process *p, int n, int time) {
    int best = -1;

    for (int i = 0; i < n; i++) {
        // compare ready jobs using aged priority
        if (!p[i].finished && p[i].arrival <= time) {
            if (best == -1 || priority_less(p, i, best, time)) {
                best = i;
            }
        }
    }
    return best;
}

static void require_priorities(const Process *p, int n) {
    for (int i = 0; i < n; i++) {
        if (!p[i].has_priority) {
            die("PRIORITY scheduling requires every workload line to include PRIORITY");
        }
    }
}

void run_priority(Process *p, int n, Timeline *timeline, int trace) {
    int completed = 0;
    int time = 0;

    require_priorities(p, n);
    if (trace) {
        fprintf(stderr, "--- Priority trace begins ---\n");
    }

    while (completed < n) {
        int selected = select_priority(p, n, time);
        int effective;

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

        effective = effective_priority(&p[selected], time);

        // selected priority job begins its non-preemptive run
        p[selected].started = 1;
        p[selected].start = time;

        if (trace) {
            fprintf(stderr, "[Time %4d] %s is dispatched, priority=%d, effective=%d\n",
                time, p[selected].pid,
                p[selected].priority, effective);
        }

        // this implementation completes the selected job in one run
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
        fprintf(stderr, "--- Priority trace ends ---\n\n");
    }
}
