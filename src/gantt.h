#ifndef GANTT_H
#define GANTT_H

#include "process.h"

/**
 * One continuous interval on the CPU timeline.
 */
typedef struct {
    char label[PID_LEN];
    int start;
    int end;
} Segment;

/**
 * Resizable container for timeline intervals.
 */
typedef struct {
    Segment *items;
    int count;
    int capacity;
} Timeline;

void timeline_init(Timeline *timeline);
void timeline_add(Timeline *timeline, const char *label, int start, int end);
void timeline_free(Timeline *timeline);
void print_gantt(const Timeline *timeline);

#endif
