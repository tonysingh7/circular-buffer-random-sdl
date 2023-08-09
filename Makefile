CC = gcc
CFLAGS = -O3 -Wall -Wextra -pthread -I/usr/include/SDL2
LDFLAGS = -lSDL2

TARGET = circular_buffer_plot

SRCS = circular_buffer_plot.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
