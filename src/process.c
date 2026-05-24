#include "process.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void die(const char *message) {
    fprintf(stderr, "ERROR: %s\n", message);
    exit(EXIT_FAILURE);
}

void die_line(int line_no, const char *message) {
    fprintf(stderr, "ERROR: line %d: %s\n", line_no, message);
    exit(EXIT_FAILURE);
}

int ready_tie_less(const Process *a, const Process *b) {
    if (a->arrival != b->arrival) {
        return a->arrival < b->arrival;
    }
    return strcmp(a->pid, b->pid) < 0;
}

Process *copy_processes(const Process *processes, int n) {
    Process *p = malloc((size_t)n * sizeof(Process));
    if (p == NULL) {
        die("memory allocation failed");
    }

    memcpy(p, processes, (size_t)n * sizeof(Process));
    for (int i = 0; i < n; i++) {
        p[i].remaining = p[i].burst;
        p[i].started = 0;
        p[i].finished = 0;
        p[i].start = -1;
        p[i].finish = -1;
        p[i].waiting = 0;
        p[i].turnaround = 0;
        p[i].response = 0;
        p[i].completed = 0;
        p[i].queued = 0;
    }
    return p;
}

void sort_for_output(Process *p, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (p[j].arrival > p[j + 1].arrival ||
                (p[j].arrival == p[j + 1].arrival && strcmp(p[j].pid, p[j + 1].pid) > 0)) {
                Process tmp = p[j];
                p[j] = p[j + 1];
                p[j + 1] = tmp;
            }
        }
    }
}
