#include "cli.h"

#include "parser.h"
#include "scheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int cli_run(int argc, char *argv[]) {
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
