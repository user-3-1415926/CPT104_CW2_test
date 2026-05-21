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

static int pid_cmp(const Process *a, const Process *b) {
    return strcmp(a->pid, b->pid);
}

int ready_tie_less(const Process *a, const Process *b) {
    if (a->arrival != b->arrival) {
        return a->arrival < b->arrival;
    }
    return pid_cmp(a, b) < 0;
}

Process *copy_processes(const Process *processes, int n) {
    Process *copy = malloc((size_t)n * sizeof(Process));
    if (copy == NULL) {
        die("memory allocation failed");
    }
    memcpy(copy, processes, (size_t)n * sizeof(Process));
    for (int i = 0; i < n; i++) {
        copy[i].remaining = copy[i].burst;
        copy[i].started = 0;
        copy[i].finished = 0;
        copy[i].start = -1;
        copy[i].finish = -1;
        copy[i].queued = 0;
    }
    return copy;
}

static int process_order_less(const Process *a, const Process *b) {
    if (a->arrival != b->arrival) {
        return a->arrival < b->arrival;
    }
    return strcmp(a->pid, b->pid) < 0;
}

void sort_for_output(Process *p, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (!process_order_less(&p[j], &p[j + 1])) {
                Process tmp = p[j];
                p[j] = p[j + 1];
                p[j + 1] = tmp;
            }
        }
    }
}
