CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = sched
TEST_TARGET = tests/test_runner

SRC = \
	src/main.c \
	src/cli.c \
	src/parser.c \
	src/process.c \
	src/scheduler.c \
	src/algorithm/fcfs.c \
	src/algorithm/sjf.c \
	src/algorithm/srtf.c \
	src/algorithm/rr.c \
	src/algorithm/priority.c \
	src/gantt.c \
	src/metrics.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

$(TEST_TARGET): tests/test_runner.c
	$(CC) $(CFLAGS) -o $(TEST_TARGET) tests/test_runner.c

test: $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET)
