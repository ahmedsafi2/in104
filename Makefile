CC = gcc
CFLAGS = `sdl2-config --cflags` -Wall -Wextra -g -O2
LDFLAGS = `sdl2-config --libs` -lSDL2_ttf -lm
SRC = main.c
OBJ = $(SRC:.c=.o)
EXEC = nuts

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

run: $(EXEC)
	./$(EXEC)

clean:
	rm -f *.o $(EXEC)

.PHONY: all clean run
