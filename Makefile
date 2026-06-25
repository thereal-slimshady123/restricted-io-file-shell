# Makefile for Restricted I/O Shell

CC = gcc
CFLAGS = -Wall -Wextra -g -O2
TARGET = restricted_io_shell
SRC = restricted_io_shell.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
	rm -rf folder_*
