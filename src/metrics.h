#ifndef METRICS_H
#define METRICS_H

#include "gantt.h"
#include "scheduler.h"

typedef struct {
    const Segment *segments;
    int segment_count;
    double avg_waiting;
    double avg_turnaround;
    double avg_response;
    double cpu_util;
    int context_switches;
    int makespan;
    int total_busy_time;
} Result;

void print_results(Process *p, int n, const Timeline *timeline, Algorithm alg);

#endif
