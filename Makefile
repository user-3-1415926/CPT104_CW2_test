CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = sched

SRC = \
	src/main.c \
	src/cli.c \
	src/parser.c \
	src/process.c \
	src/scheduler.c \
	src/fcfs.c \
	src/sjf.c \
	src/srtf.c \
	src/rr.c \
	src/gantt.c \
	src/metrics.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)