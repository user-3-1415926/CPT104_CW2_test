#ifndef GANTT_H
#define GANTT_H

#include "process.h"

typedef struct {
    char label[PID_LEN];
    int start;
    int end;
} Segment;

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
