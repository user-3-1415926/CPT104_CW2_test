#include "gantt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16

void timeline_init(Timeline *timeline) {
    timeline->capacity = INITIAL_CAPACITY;
    timeline->count = 0;
    timeline->items = malloc((size_t)timeline->capacity * sizeof(Segment));
    if (timeline->items == NULL) {
        die("memory allocation failed");
    }
}

void timeline_add(Timeline *timeline, const char *label, int start, int end) {
    if (end <= start) {
        return;
    }

    // keep adjacent intervals compact when the label is unchanged
    if (timeline->count > 0) {
        Segment *last = &timeline->items[timeline->count - 1];
        if (last->end == start && strcmp(last->label, label) == 0) {
            last->end = end;
            return;
        }
    }
    if (timeline->count == timeline->capacity) {
        // allocate more room for additional timeline intervals
        timeline->capacity *= 2;
        timeline->items = realloc(timeline->items, (size_t)timeline->capacity * sizeof(Segment));
        if (timeline->items == NULL) {
            die("memory allocation failed");
        }
    }
    strncpy(timeline->items[timeline->count].label, label, PID_LEN - 1);
    timeline->items[timeline->count].label[PID_LEN - 1] = '\0';
    timeline->items[timeline->count].start = start;
    timeline->items[timeline->count].end = end;
    timeline->count++;
}

void timeline_free(Timeline *timeline) {
    free(timeline->items);
    timeline->items = NULL;
    timeline->count = 0;
    timeline->capacity = 0;
}

void print_gantt(const Timeline *timeline) {
    printf("GANTT:\n");
    if (timeline->count == 0) {
        printf("(empty)\n");
        return;
    }
    printf("%d", timeline->items[0].start);
    for (int i = 0; i < timeline->count; i++) {
        printf(" | %s | %d", timeline->items[i].label, timeline->items[i].end);
    }
    printf("\n");
}
