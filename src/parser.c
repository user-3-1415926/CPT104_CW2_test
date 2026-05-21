#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16

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

Process *read_workload(const char *path, int *out_count) {
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
