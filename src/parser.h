#ifndef PARSER_H
#define PARSER_H

#include "process.h"

Process *read_workload(const char *path, int *out_count);

#endif
