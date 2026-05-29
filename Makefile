CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pthread
TARGET  = monitor
SRCS    = main.c collector.c proc_reader.c display.c report.c utils.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c monitor.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
