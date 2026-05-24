#include "metrics.h"

#include <stdio.h>
#include <string.h>

static int count_context_switches(const Timeline *timeline) {
    int switches = 0;
    const char *last_process = NULL;

    for (int i = 0; i < timeline->count; i++) {
        const char *label = timeline->items[i].label;
        if (strcmp(label, IDLE_LABEL) == 0) {
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

static Result calculate_result(Process *p, int n, const Timeline *timeline) {
    Result result;
    double total_wait = 0.0;
    double total_tat = 0.0;
    double total_resp = 0.0;

    for (int i = 0; i < n; i++) {
        p[i].turnaround = p[i].finish - p[i].arrival;
        p[i].waiting = p[i].turnaround - p[i].burst;
        p[i].response = p[i].start - p[i].arrival;
        total_wait += p[i].waiting;
        total_tat += p[i].turnaround;
        total_resp += p[i].response;
    }

    result.segments = timeline->items;
    result.segment_count = timeline->count;
    result.total_busy_time = total_busy_time(timeline);
    result.makespan = makespan(timeline);
    result.context_switches = count_context_switches(timeline);
    result.cpu_util = result.makespan > 0 ?
        ((double)result.total_busy_time / (double)result.makespan) * 100.0 : 0.0;
    result.avg_waiting = total_wait / n;
    result.avg_turnaround = total_tat / n;
    result.avg_response = total_resp / n;
    return result;
}

void print_results(Process *p, int n, const Timeline *timeline, Algorithm alg) {
    Result result = calculate_result(p, n, timeline);

    sort_for_output(p, n);
    print_gantt(timeline);
    printf("\n");
    printf("PID\tArrival\tBurst\tStart\tFinish\tWaiting\tTurnaround\tResponse\n");
    for (int i = 0; i < n; i++) {
        printf("%s\t%d\t%d\t%d\t%d\t%d\t%d\t\t%d\n",
               p[i].pid, p[i].arrival, p[i].burst, p[i].start, p[i].finish,
               p[i].waiting, p[i].turnaround, p[i].response);
    }
    printf("\n");
    printf("RESULT: OK\n");
    printf("ALG=%s\n", algorithm_name(alg));
    printf("AVG_WAIT=%.2f\n", result.avg_waiting);
    printf("AVG_TAT=%.2f\n", result.avg_turnaround);
    printf("AVG_RESP=%.2f\n", result.avg_response);
    printf("CONTEXT_SWITCHES=%d\n", result.context_switches);
    printf("CPU_UTIL=%.2f%%\n", result.cpu_util);
}
