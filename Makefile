CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c11 -g -Iinclude
SRC = $(wildcard src/*.c) main.c
BIN_DIR = bin
TARGET = $(BIN_DIR)/main

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
