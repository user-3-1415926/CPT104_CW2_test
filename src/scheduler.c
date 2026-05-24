#include "scheduler.h"

#include "fcfs.h"
#include "metrics.h"
#include "priority.h"
#include "rr.h"
#include "sjf.h"
#include "srtf.h"

#include <ctype.h>
#include <stdlib.h>

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

const char *algorithm_name(Algorithm alg) {
    switch (alg) {
        case ALG_FCFS:
            return "FCFS";
        case ALG_SJF:
            return "SJF";
        case ALG_SRTF:
            return "SRTF";
        case ALG_RR:
            return "RR";
        case ALG_PRIORITY:
            return "PRIORITY";
    }
    return "UNKNOWN";
}

Algorithm parse_algorithm(const char *text) {
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
    if (same_text(text, "PRIORITY") || same_text(text, "PRI")) {
        return ALG_PRIORITY;
    }
    die("unknown algorithm; use FCFS, SJF, SRTF, RR, or PRIORITY");
    return ALG_FCFS;
}

int next_arrival_time(const Process *p, int n, int time) {
    int best = -1;
    for (int i = 0; i < n; i++) {
        if (!p[i].finished && p[i].arrival > time && (best == -1 || p[i].arrival < best)) {
            best = p[i].arrival;
        }
    }
    return best;
}

void run_algorithm(const Process *base, int n, Algorithm alg, int quantum, int trace) {
    Process *p = copy_processes(base, n);
    Timeline timeline;

    timeline_init(&timeline);
    if (alg == ALG_FCFS) {
        run_fcfs(p, n, &timeline, trace);
    } else if (alg == ALG_SJF) {
        run_sjf(p, n, &timeline, trace);
    } else if (alg == ALG_SRTF) {
        run_srtf(p, n, &timeline, trace);
    } else if (alg == ALG_PRIORITY) {
        run_priority(p, n, &timeline, trace);
    } else {
        run_rr(p, n, quantum, &timeline, trace);
    }
    print_results(p, n, &timeline, alg);

    timeline_free(&timeline);
    free(p);
}
