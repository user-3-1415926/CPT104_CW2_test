#include "metrics.h"

#include <stdio.h>
#include <string.h>

static int count_context_switches(const Timeline *timeline) {
    int switches = 0;
    const char *last_process = NULL;

    for (int i = 0; i < timeline->count; i++) {
        const char *label = timeline->items[i].label;

        // only transitions between real processes are counted
        if (strcmp(label, IDLE_LABEL) == 0) {
            last_process = NULL;
            continue;
        }
        if (last_process != NULL && strcmp(last_process, label) != 0) {
            switches++;
        }
        last_process = label;
    }
    return switches;
}

static int total_busy_time(const Timeline *timeline) {
    int busy = 0;
    for (int i = 0; i < timeline->count; i++) {
        if (strcmp(timeline->items[i].label, IDLE_LABEL) != 0) {
            busy += timeline->items[i].end - timeline->items[i].start;
        }
    }
    return busy;
}

static int makespan(const Timeline *timeline) {
    if (timeline->count == 0) {
        return 0;
    }
    return timeline->items[timeline->count - 1].end;
}

void print_results(Process *p, int n, const Timeline *timeline, Algorithm alg) {
    double total_wait = 0.0;
    double total_tat = 0.0;
    double total_resp = 0.0;
    int switches = count_context_switches(timeline);
    int busy = total_busy_time(timeline);
    int span = makespan(timeline);
    double cpu_util = span > 0 ? ((double)busy / (double)span) * 100.0 : 0.0;

    sort_for_output(p, n);
    print_gantt(timeline);
    printf("\n");
    printf("PID\tArrival\tBurst\tStart\tFinish\tWaiting\tTurnaround\tResponse\n");

    // derive per-process statistics from the final start and finish times
    for (int i = 0; i < n; i++) {
        int turnaround = p[i].finish - p[i].arrival;
        int waiting = turnaround - p[i].burst;
        int response = p[i].start - p[i].arrival;
        total_wait += waiting;
        total_tat += turnaround;
        total_resp += response;
        printf("%s\t%d\t%d\t%d\t%d\t%d\t%d\t\t%d\n",
            p[i].pid, p[i].arrival, p[i].burst, p[i].start, p[i].finish,
            waiting, turnaround, response);
    }
    printf("\n");
    printf("RESULT: OK\n");
    printf("ALG=%s\n", algorithm_name(alg));
    printf("AVG_WAIT=%.2f\n", total_wait / n);
    printf("AVG_TAT=%.2f\n", total_tat / n);
    printf("AVG_RESP=%.2f\n", total_resp / n);
    printf("CONTEXT_SWITCHES=%d\n", switches);
    printf("CPU_UTIL=%.2f%%\n", cpu_util);
}
