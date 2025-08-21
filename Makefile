CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -O2 -Iinclude
OBJ=src/main.o src/game.o src/maze.o src/random.o src/utils.o

all: maze_game

maze_game: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	del /Q $(OBJ) maze_game.exe 2>NUL || true

run: maze_game
	./maze_game

.PHONY: all clean run
