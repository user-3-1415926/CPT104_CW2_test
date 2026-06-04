#include "rr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int *items;
    int head;
    int tail;
    int count;
    int capacity;
} Queue;

static void queue_init(Queue *queue, int capacity) {
    // leave one extra slot for circular queue movement
    queue->capacity = capacity > 0 ? capacity + 1 : 1;
    queue->items = malloc((size_t)queue->capacity * sizeof(int));
    if (queue->items == NULL) {
        die("memory allocation failed");
    }
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

static int queue_empty(const Queue *queue) {
    return queue->count == 0;
}

static void queue_push(Queue *queue, int value) {
    if (queue->count == queue->capacity) {
        die("ready queue overflow");
    }
    queue->items[queue->tail] = value;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
}

static int queue_pop(Queue *queue) {
    int value;
    if (queue_empty(queue)) {
        die("ready queue underflow");
    }
    value = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    return value;
}

static void queue_free(Queue *queue) {
    free(queue->items);
    queue->items = NULL;
}

static int unqueued_arrival_less(const Process *p, int a, int b) {
    if (p[a].arrival != p[b].arrival) {
        return p[a].arrival < p[b].arrival;
    }
    // PID solve same-time arrivals 
    return strcmp(p[a].pid, p[b].pid) < 0;
}

static void enqueue_arrivals_up_to(Process *p, int n, int time, Queue *queue) {
    while (1) {
        int best = -1;

        for (int i = 0; i < n; i++) {
            // enqueue newly available jobs in arrival order
            if (!p[i].queued && !p[i].finished && p[i].arrival <= time) {
                if (best == -1 || unqueued_arrival_less(p, i, best)) {
                    best = i;
                }
            }
        }
        if (best == -1) {
            return;
        }
        p[best].queued = 1;
        queue_push(queue, best);
    }
}

static int next_unfinished_arrival(const Process *p, int n, int time) {
    int best = -1;
    for (int i = 0; i < n; i++) {
        if (!p[i].finished && !p[i].queued && p[i].arrival > time) {
            if (best == -1 || p[i].arrival < best) {
                best = p[i].arrival;
            }
        }
    }
    return best;
}

void run_rr(Process *p, int n, int quantum, Timeline *timeline, int trace) {
    Queue queue;
    int completed = 0;
    int time = 0;

    if (trace) {
        fprintf(stderr, "--- RR trace begins ---\n");
    }

    queue_init(&queue, n + 1);
    while (completed < n) {
        int selected;
        int slice;
        int end;

        enqueue_arrivals_up_to(p, n, time, &queue);

        // no process can run yet, jump to the next arrival
        if (queue_empty(&queue)) {
            int next = next_unfinished_arrival(p, n, time);
            if (next == -1) {
                die("internal scheduling error");
            }
            if (trace) {
                fprintf(stderr, "[Time %4d] processor waits for next arrival\n", time);
            }
            timeline_add(timeline, IDLE_LABEL, time, next);
            time = next;
            enqueue_arrivals_up_to(p, n, time, &queue);
        }

        selected = queue_pop(&queue);
        slice = p[selected].remaining < quantum ? p[selected].remaining : quantum;
        end = time + slice;

        // response time is based on the first CPU slice
        if (!p[selected].started) {
            p[selected].started = 1;
            p[selected].start = time;
        }

        if (trace) {
            fprintf(stderr, "[Time %4d] %s runs for %d unit(s), remaining=%d\n",
                time, p[selected].pid, slice, p[selected].remaining);
        }

        // consume either a full quantum or the remaining work
        timeline_add(timeline, p[selected].pid, time, end);
        p[selected].remaining -= slice;
        time = end;

        enqueue_arrivals_up_to(p, n, time, &queue);
        if (p[selected].remaining == 0) {
            p[selected].finish = time;
            p[selected].finished = 1;
            completed++;

            if (trace) {
                fprintf(stderr, "[Time %4d] %s completes\n", time, p[selected].pid);
            }
        } else {
            queue_push(&queue, selected);
        }
    }
    queue_free(&queue);

    if (trace) {
        fprintf(stderr, "--- RR trace ends ---\n\n");
    }
}
