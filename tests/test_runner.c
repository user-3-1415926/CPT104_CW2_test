#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

typedef struct {
    char *text;
    int exit_code;
} CommandResult;

static CommandResult run_command(const char *command) {
    CommandResult result;
    FILE *pipe;
    size_t used = 0;
    size_t capacity = 4096;
    int status;

    result.text = malloc(capacity);
    if (result.text == NULL) {
        fprintf(stderr, "test runner: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    result.text[0] = '\0';

    pipe = popen(command, "r");
    if (pipe == NULL) {
        fprintf(stderr, "test runner: cannot run command: %s\n", command);
        free(result.text);
        exit(EXIT_FAILURE);
    }

    while (1) {
        size_t available = capacity - used;
        if (available < 1024) {
            capacity *= 2;
            result.text = realloc(result.text, capacity);
            if (result.text == NULL) {
                fprintf(stderr, "test runner: memory allocation failed\n");
                pclose(pipe);
                exit(EXIT_FAILURE);
            }
            available = capacity - used;
        }

        if (fgets(result.text + used, (int)available, pipe) == NULL) {
            break;
        }
        used += strlen(result.text + used);
    }

    status = pclose(pipe);
    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else {
        result.exit_code = -1;
    }
    return result;
}

static void free_result(CommandResult *result) {
    free(result->text);
    result->text = NULL;
}

static int check_contains(const char *test_name, const char *output, const char *expected) {
    if (strstr(output, expected) == NULL) {
        printf("FAIL %s: missing '%s'\n", test_name, expected);
        return 0;
    }
    return 1;
}

static int check_gantt_line(const char *test_name, const char *output, const char *expected) {
    const char *line = strstr(output, "GANTT:\n");
    char actual[512];
    size_t len;

    if (line == NULL) {
        printf("FAIL %s: missing GANTT block\n", test_name);
        return 0;
    }

    line += strlen("GANTT:\n");
    len = strcspn(line, "\n");
    if (len >= sizeof(actual)) {
        len = sizeof(actual) - 1;
    }
    memcpy(actual, line, len);
    actual[len] = '\0';

    if (strcmp(actual, expected) != 0) {
        printf("FAIL %s: expected GANTT '%s', got '%s'\n", test_name, expected, actual);
        return 0;
    }
    return 1;
}

static int check_summary_value(const char *test_name, const char *output, const char *expected) {
    return check_contains(test_name, output, expected);
}

static int check_exit_code(const char *test_name, int actual, int expected) {
    if (actual != expected) {
        printf("FAIL %s: expected exit code %d, got %d\n", test_name, expected, actual);
        return 0;
    }
    return 1;
}

static int check_nonzero_exit(const char *test_name, int actual) {
    if (actual == 0) {
        printf("FAIL %s: expected non-zero exit code\n", test_name);
        return 0;
    }
    return 1;
}

static int test_fcfs_basic(void) {
    const char *name = "fcfs_basic";
    CommandResult result = run_command("./sched tests/basic.txt --alg FCFS 2>&1");
    int ok = 1;

    ok &= check_exit_code(name, result.exit_code, 0);
    ok &= check_gantt_line(name, result.text, "0 | P1 | 7 | P2 | 11 | P3 | 12 | P4 | 16");
    ok &= check_summary_value(name, result.text, "AVG_WAIT=4.50");
    ok &= check_summary_value(name, result.text, "CPU_UTIL=100.00%");
    free_result(&result);
    return ok;
}

static int test_sjf_basic(void) {
    const char *name = "sjf_basic";
    CommandResult result = run_command("./sched tests/basic.txt --alg SJF 2>&1");
    int ok = 1;

    ok &= check_exit_code(name, result.exit_code, 0);
    ok &= check_gantt_line(name, result.text, "0 | P1 | 7 | P3 | 8 | P2 | 12 | P4 | 16");
    ok &= check_summary_value(name, result.text, "AVG_WAIT=3.75");
    free_result(&result);
    return ok;
}

static int test_srtf_preempt(void) {
    const char *name = "srtf_preempt";
    CommandResult result = run_command("./sched tests/srtf_preempt.txt --alg SRTF 2>&1");
    int ok = 1;

    ok &= check_exit_code(name, result.exit_code, 0);
    ok &= check_gantt_line(name, result.text, "0 | P1 | 1 | P2 | 2 | P3 | 4 | P4 | 5 | P2 | 8 | P1 | 15");
    ok &= check_summary_value(name, result.text, "CONTEXT_SWITCHES=5");
    free_result(&result);
    return ok;
}

static int test_rr_quantum(void) {
    const char *name = "rr_quantum";
    CommandResult result = run_command("./sched tests/rr_quantum.txt --alg RR --q 1 2>&1");
    int ok = 1;

    ok &= check_exit_code(name, result.exit_code, 0);
    ok &= check_gantt_line(name, result.text,
        "0 | P1 | 1 | P2 | 2 | P1 | 3 | P3 | 4 | P2 | 5 | P1 | 6 | P3 | 7 | P2 | 8 | P1 | 9 | P3 | 10 | P2 | 11 | P1 | 12 | P2 | 13 | P1 | 16");
    ok &= check_summary_value(name, result.text, "CONTEXT_SWITCHES=13");
    free_result(&result);
    return ok;
}

static int test_invalid_workload(void) {
    const char *name = "invalid_workload";
    CommandResult result = run_command("./sched tests/invalid_workload.txt --alg FCFS 2>&1");
    int ok = 1;

    ok &= check_nonzero_exit(name, result.exit_code);
    ok &= check_contains(name, result.text, "ARRIVAL must be a non-negative integer");
    free_result(&result);
    return ok;
}

int main(void) {
    int passed = 0;
    int total = 0;

    total++;
    passed += test_fcfs_basic();
    total++;
    passed += test_sjf_basic();
    total++;
    passed += test_srtf_preempt();
    total++;
    passed += test_rr_quantum();
    total++;
    passed += test_invalid_workload();

    printf("Tests passed: %d/%d\n", passed, total);
    return passed == total ? EXIT_SUCCESS : EXIT_FAILURE;
}
