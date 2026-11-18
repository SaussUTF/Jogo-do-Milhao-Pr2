# Makefile - Quiz (C99)
# Uso: make | make run | make debug | make valgrind | make clean

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -g
LDLIBS  = -lm
BINNAME = programa

ifeq ($(OS),Windows_NT)
	BIN = $(BINNAME).exe
	RM  = del /Q /F
else
	BIN = ./$(BINNAME)
	RM  = rm -f
endif

SRC = main.c arquivo.c perguntas.c jogo.c
OBJ = $(SRC:.c=.o)

all: $(BINNAME)

$(BINNAME): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

%.o: %.c quiz.h
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	$(BIN)

debug: all
	gdb $(BIN)

valgrind: all
	valgrind --tool=memcheck --leak-check=full --track-origins=yes --show-leak-kinds=all $(BIN)

clean:
	$(RM) $(OBJ) $(BINNAME) $(BINNAME).exe perguntas.bin

.PHONY: all run debug valgrind clean
