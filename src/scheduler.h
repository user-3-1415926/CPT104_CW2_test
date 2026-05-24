#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "gantt.h"
#include "process.h"

typedef enum {
    ALG_FCFS,
    ALG_SJF,
    ALG_SRTF,
    ALG_RR,
    ALG_PRIORITY
} Algorithm;

const char *algorithm_name(Algorithm alg);
Algorithm parse_algorithm(const char *text);
int next_arrival_time(const Process *p, int n, int time);
void run_algorithm(const Process *base, int n, Algorithm alg, int quantum, int trace);

#endif
