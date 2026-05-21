#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PID_LEN 32
#define INITIAL_CAPACITY 16
#define IDLE_LABEL "IDLE"

typedef enum {
    ALG_FCFS,
    ALG_SJF,
    ALG_SRTF,
    ALG_RR
} Algorithm;

typedef struct {
    char pid[PID_LEN];
    int arrival;
    int burst;
    int priority;
    int has_priority;
    int remaining;
    int started;
    int finished;
    int start;
    int finish;
    int queued;
} Process;

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

typedef struct {
    int *items;
    int head;
    int tail;
    int count;
    int capacity;
} Queue;

static void die(const char *message) {
    fprintf(stderr, "ERROR: %s\n", message);
    exit(EXIT_FAILURE);
}

static void die_line(int line_no, const char *message) {
    fprintf(stderr, "ERROR: line %d: %s\n", line_no, message);
    exit(EXIT_FAILURE);
}

static int same_text(const char *a, const char *b) {
    while (*a && *b) {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static int pid_cmp(const Process *a, const Process *b) {
    return strcmp(a->pid, b->pid);
}

static int ready_tie_less(const Process *a, const Process *b) {
    if (a->arrival != b->arrival) {
        return a->arrival < b->arrival;
    }
    return pid_cmp(a, b) < 0;
}

static void timeline_init(Timeline *timeline) {
    timeline->capacity = INITIAL_CAPACITY;
    timeline->count = 0;
    timeline->items = malloc((size_t)timeline->capacity * sizeof(Segment));
    if (timeline->items == NULL) {
        die("memory allocation failed");
    }
}

static void timeline_add(Timeline *timeline, const char *label, int start, int end) {
    if (end <= start) {
        return;
    }
    if (timeline->count > 0) {
        Segment *last = &timeline->items[timeline->count - 1];
        if (last->end == start && strcmp(last->label, label) == 0) {
            last->end = end;
            return;
        }
    }
    if (timeline->count == timeline->capacity) {
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

static void timeline_free(Timeline *timeline) {
    free(timeline->items);
    timeline->items = NULL;
    timeline->count = 0;
    timeline->capacity = 0;
}

static void queue_init(Queue *queue, int capacity) {
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

static Process *copy_processes(const Process *processes, int n) {
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

static char *trim(char *line) {
    char *end;
    while (isspace((unsigned char)*line)) {
        line++;
    }
    if (*line == '\0') {
        return line;
    }
    end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return line;
}

static int parse_int_token(const char *text, int *out) {
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (*text == '\0' || *end != '\0' || value < -1000000000L || value > 1000000000L) {
        return 0;
    }
    *out = (int)value;
    return 1;
}

static void add_process(Process **processes, int *count, int *capacity, const Process *process) {
    if (*count == *capacity) {
        *capacity *= 2;
        *processes = realloc(*processes, (size_t)*capacity * sizeof(Process));
        if (*processes == NULL) {
            die("memory allocation failed");
        }
    }
    (*processes)[*count] = *process;
    (*count)++;
}

static Process *read_workload(const char *path, int *out_count) {
    FILE *file = fopen(path, "r");
    char line[256];
    int capacity = INITIAL_CAPACITY;
    int count = 0;
    int line_no = 0;
    Process *processes;

    if (file == NULL) {
        fprintf(stderr, "ERROR: cannot open workload file '%s'\n", path);
        exit(EXIT_FAILURE);
    }

    processes = malloc((size_t)capacity * sizeof(Process));
    if (processes == NULL) {
        fclose(file);
        die("memory allocation failed");
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *text;
        char at_text[32];
        char burst_text[32];
        char priority_text[32];
        char extra[32];
        Process p;
        int fields;

        line_no++;
        text = trim(line);
        if (*text == '\0' || *text == '#') {
            continue;
        }

        memset(&p, 0, sizeof(p));
        fields = sscanf(text, "%31s %31s %31s %31s %31s",
                        p.pid, at_text, burst_text, priority_text, extra);
        if (fields != 3 && fields != 4) {
            die_line(line_no, "expected: PID ARRIVAL BURST [PRIORITY]");
        }
        if (!parse_int_token(at_text, &p.arrival)) {
            die_line(line_no, "ARRIVAL must be an integer");
        }
        if (!parse_int_token(burst_text, &p.burst)) {
            die_line(line_no, "BURST must be an integer");
        }
        p.has_priority = fields == 4;
        if (p.has_priority && !parse_int_token(priority_text, &p.priority)) {
            die_line(line_no, "PRIORITY must be an integer");
        }
        if (p.arrival < 0) {
            die_line(line_no, "ARRIVAL must be a non-negative integer");
        }
        if (p.burst <= 0) {
            die_line(line_no, "BURST must be a positive integer");
        }
        for (int i = 0; i < count; i++) {
            if (strcmp(processes[i].pid, p.pid) == 0) {
                die_line(line_no, "duplicate PID");
            }
        }
        p.remaining = p.burst;
        p.start = -1;
        p.finish = -1;
        add_process(&processes, &count, &capacity, &p);
    }

    fclose(file);
    if (count == 0) {
        die("workload contains no processes");
    }
    *out_count = count;
    return processes;
}

static const char *algorithm_name(Algorithm alg) {
    switch (alg) {
        case ALG_FCFS:
            return "FCFS";
        case ALG_SJF:
            return "SJF";
        case ALG_SRTF:
            return "SRTF";
        case ALG_RR:
            return "RR";
    }
    return "UNKNOWN";
}

static Algorithm parse_algorithm(const char *text) {
    if (same_text(text, "FCFS")) {
        return ALG_FCFS;
    }
    if (same_text(text, "SJF")) {
        return ALG_SJF;
    }
    if (same_text(text, "SRTF")) {
        return ALG_SRTF;
    }
    if (same_text(text, "RR")) {
        return ALG_RR;
    }
    die("unknown algorithm; use FCFS, SJF, SRTF, or RR");
    return ALG_FCFS;
}

static int next_arrival_time(const Process *p, int n, int time) {
    int best = -1;
    for (int i = 0; i < n; i++) {
        if (!p[i].finished && p[i].arrival > time && (best == -1 || p[i].arrival < best)) {
            best = p[i].arrival;
        }
    }
    return best;
}

static int select_fcfs(const Process *p, int n, int time) {
    int best = -1;
    for (int i = 0; i < n; i++) {
        if (!p[i].finished && p[i].arrival <= time) {
            if (best == -1 || ready_tie_less(&p[i], &p[best])) {
                best = i;
            }
        }
    }
    return best;
}

static int select_sjf(const Process *p, int n, int time) {
    int best = -1;
    for (int i = 0; i < n; i++) {
        if (!p[i].finished && p[i].arrival <= time) {
            if (best == -1 ||
                p[i].burst < p[best].burst ||
                (p[i].burst == p[best].burst && ready_tie_less(&p[i], &p[best]))) {
                best = i;
            }
        }
    }
    return best;
}

static int select_srtf(const Process *p, int n, int time) {
    int best = -1;
    for (int i = 0; i < n; i++) {
        if (!p[i].finished && p[i].arrival <= time && p[i].remaining > 0) {
            if (best == -1 ||
                p[i].remaining < p[best].remaining ||
                (p[i].remaining == p[best].remaining && ready_tie_less(&p[i], &p[best]))) {
                best = i;
            }
        }
    }
    return best;
}

static void run_nonpreemptive(Process *p, int n, Algorithm alg, Timeline *timeline, int trace) {
    int completed = 0;
    int time = 0;

    while (completed < n) {
        int selected = (alg == ALG_FCFS) ? select_fcfs(p, n, time) : select_sjf(p, n, time);
        if (selected == -1) {
            int next = next_arrival_time(p, n, time);
            if (next == -1) {
                die("internal scheduling error");
            }
            if (trace) {
                printf("TRACE %d-%d: IDLE\n", time, next);
            }
            timeline_add(timeline, IDLE_LABEL, time, next);
            time = next;
            continue;
        }

        p[selected].started = 1;
        p[selected].start = time;
        if (trace) {
            printf("TRACE %d-%d: %s\n", time, time + p[selected].burst, p[selected].pid);
        }
        timeline_add(timeline, p[selected].pid, time, time + p[selected].burst);
        time += p[selected].burst;
        p[selected].remaining = 0;
        p[selected].finish = time;
        p[selected].finished = 1;
        completed++;
    }
}

static void run_srtf(Process *p, int n, Timeline *timeline, int trace) {
    int completed = 0;
    int time = 0;

    while (completed < n) {
        int selected = select_srtf(p, n, time);
        if (selected == -1) {
            int next = next_arrival_time(p, n, time);
            if (next == -1) {
                die("internal scheduling error");
            }
            if (trace) {
                printf("TRACE %d-%d: IDLE\n", time, next);
            }
            timeline_add(timeline, IDLE_LABEL, time, next);
            time = next;
            continue;
        }

        if (!p[selected].started) {
            p[selected].started = 1;
            p[selected].start = time;
        }
        if (trace) {
            printf("TRACE %d-%d: %s remaining_before=%d\n",
                   time, time + 1, p[selected].pid, p[selected].remaining);
        }
        timeline_add(timeline, p[selected].pid, time, time + 1);
        p[selected].remaining--;
        time++;

        if (p[selected].remaining == 0) {
            p[selected].finish = time;
            p[selected].finished = 1;
            completed++;
        }
    }
}

static int unqueued_arrival_less(const Process *p, int a, int b) {
    if (p[a].arrival != p[b].arrival) {
        return p[a].arrival < p[b].arrival;
    }
    return strcmp(p[a].pid, p[b].pid) < 0;
}

static void enqueue_arrivals_up_to(Process *p, int n, int time, Queue *queue) {
    while (1) {
        int best = -1;
        for (int i = 0; i < n; i++) {
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

static void run_rr(Process *p, int n, int quantum, Timeline *timeline, int trace) {
    Queue queue;
    int completed = 0;
    int time = 0;

    queue_init(&queue, n + 1);
    while (completed < n) {
        int selected;
        int slice;
        int end;

        enqueue_arrivals_up_to(p, n, time, &queue);
        if (queue_empty(&queue)) {
            int next = next_unfinished_arrival(p, n, time);
            if (next == -1) {
                die("internal scheduling error");
            }
            if (trace) {
                printf("TRACE %d-%d: IDLE\n", time, next);
            }
            timeline_add(timeline, IDLE_LABEL, time, next);
            time = next;
            enqueue_arrivals_up_to(p, n, time, &queue);
        }

        selected = queue_pop(&queue);
        slice = p[selected].remaining < quantum ? p[selected].remaining : quantum;
        end = time + slice;

        if (!p[selected].started) {
            p[selected].started = 1;
            p[selected].start = time;
        }
        if (trace) {
            printf("TRACE %d-%d: %s slice=%d remaining_before=%d\n",
                   time, end, p[selected].pid, slice, p[selected].remaining);
        }
        timeline_add(timeline, p[selected].pid, time, end);
        p[selected].remaining -= slice;
        time = end;

        enqueue_arrivals_up_to(p, n, time, &queue);
        if (p[selected].remaining == 0) {
            p[selected].finish = time;
            p[selected].finished = 1;
            completed++;
        } else {
            queue_push(&queue, selected);
        }
    }
    queue_free(&queue);
}

static int process_order_less(const Process *a, const Process *b) {
    if (a->arrival != b->arrival) {
        return a->arrival < b->arrival;
    }
    return strcmp(a->pid, b->pid) < 0;
}

static void sort_for_output(Process *p, int n) {
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

static void print_gantt(const Timeline *timeline) {
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

static void print_results(Process *p, int n, const Timeline *timeline, Algorithm alg) {
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

static void run_algorithm(const Process *base, int n, Algorithm alg, int quantum, int trace) {
    Process *p = copy_processes(base, n);
    Timeline timeline;

    timeline_init(&timeline);
    if (alg == ALG_FCFS || alg == ALG_SJF) {
        run_nonpreemptive(p, n, alg, &timeline, trace);
    } else if (alg == ALG_SRTF) {
        run_srtf(p, n, &timeline, trace);
    } else {
        run_rr(p, n, quantum, &timeline, trace);
    }
    print_results(p, n, &timeline, alg);

    timeline_free(&timeline);
    free(p);
}

static void run_demo(void) {
    Process demo[] = {
        {"P1", 0, 7, 0, 0, 7, 0, 0, -1, -1, 0},
        {"P2", 2, 4, 0, 0, 4, 0, 0, -1, -1, 0},
        {"P3", 4, 1, 0, 0, 1, 0, 0, -1, -1, 0},
        {"P4", 6, 4, 0, 0, 4, 0, 0, -1, -1, 0}
    };
    int n = (int)(sizeof(demo) / sizeof(demo[0]));

    printf("=== DEMO: FCFS ===\n");
    run_algorithm(demo, n, ALG_FCFS, 0, 0);
    printf("\n=== DEMO: SJF ===\n");
    run_algorithm(demo, n, ALG_SJF, 0, 0);
    printf("\n=== DEMO: SRTF ===\n");
    run_algorithm(demo, n, ALG_SRTF, 0, 0);
    printf("\n=== DEMO: RR q=3 ===\n");
    run_algorithm(demo, n, ALG_RR, 3, 0);
}

static void usage(void) {
    printf("Usage:\n");
    printf("  ./sched --demo\n");
    printf("  ./sched <workload> --alg FCFS\n");
    printf("  ./sched <workload> --alg SJF\n");
    printf("  ./sched <workload> --alg SRTF [--trace]\n");
    printf("  ./sched <workload> --alg RR --q <quantum> [--trace]\n");
}

int main(int argc, char *argv[]) {
    const char *workload = NULL;
    const char *alg_text = NULL;
    int quantum = 0;
    int trace = 0;
    int has_demo = 0;
    int has_quantum = 0;

    if (argc < 2) {
        usage();
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--demo") == 0) {
            has_demo = 1;
        } else if (strcmp(argv[i], "--alg") == 0) {
            if (i + 1 >= argc) {
                die("--alg requires a value");
            }
            alg_text = argv[++i];
        } else if (strcmp(argv[i], "--q") == 0) {
            char *end = NULL;
            long value;
            if (i + 1 >= argc) {
                die("--q requires a value");
            }
            value = strtol(argv[++i], &end, 10);
            if (*end != '\0' || value <= 0 || value > 1000000) {
                die("--q must be a positive integer");
            }
            quantum = (int)value;
            has_quantum = 1;
        } else if (strcmp(argv[i], "--trace") == 0) {
            trace = 1;
        } else if (argv[i][0] == '-') {
            usage();
            return EXIT_FAILURE;
        } else if (workload == NULL) {
            workload = argv[i];
        } else {
            usage();
            return EXIT_FAILURE;
        }
    }

    if (has_demo) {
        if (argc != 2) {
            die("--demo must be used on its own");
        }
        run_demo();
        return EXIT_SUCCESS;
    }

    if (workload == NULL) {
        die("workload file is required unless --demo is used");
    }
    if (alg_text == NULL) {
        die("--alg is required unless --demo is used");
    }

    {
        int n = 0;
        Algorithm alg = parse_algorithm(alg_text);
        Process *processes;

        if (alg == ALG_RR && !has_quantum) {
            die("--q is required when --alg RR");
        }
        if (alg != ALG_RR && has_quantum) {
            die("--q is only valid when --alg RR");
        }

        processes = read_workload(workload, &n);
        run_algorithm(processes, n, alg, quantum, trace);
        free(processes);
    }

    return EXIT_SUCCESS;
}
