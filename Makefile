# Set the compiler and flags
CC = gcc
CFLAGS = -Wall -g
INCLUDES = -I./libs/uthash

# Set the output binary directory and the program name
OUT_DIR = ./out
PROGRAM = $(OUT_DIR)/main

# Set the source files
SRC = ./src/main.c ./src/parser.c ./src/util.c ./src/lexer.c ./src/interpreter.c

# Create the out directory if it doesn't exist
$(OUT_DIR):
	mkdir -p $(OUT_DIR)

# Rule to compile the program
$(PROGRAM): $(SRC) | $(OUT_DIR)
	$(CC) $(CFLAGS) -o $(PROGRAM) $(SRC)

# Rule to clean up the compiled files
clean:
	rm -rf $(OUT_DIR)

# Default target: build the program
all: $(PROGRAM)
