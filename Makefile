# Makefile for Nuts Puzzle game

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 
LDFLAGS = -lSDL2 -lSDL2_ttf -lm

# Target executable name
TARGET = nuts_puzzle

# Source files
SRC = main.c

# Object files
OBJ = $(SRC:.c=.o)

# Default target
all: $(TARGET)

# Link the target executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean generated files
clean:
	rm -f $(OBJ) $(TARGET)

# Run the game
run: $(TARGET)
	./$(TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build the game (default)"
	@echo "  clean     - Remove object files and executable"
	@echo "  run       - Build and run the game"
	@echo "  help      - Display this help message"

# Dependencies
main.o: main.c

.PHONY: all clean run help
