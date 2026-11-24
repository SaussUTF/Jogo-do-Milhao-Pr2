# Makefile - Quiz (C99)
# Uso: make | make run | make debug | make valgrind | make clean

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -g
<<<<<<< HEAD
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
=======
LDLIBS  = -lraylib -lm
BINNAME = programa

ifeq ($(OS),Windows_NT)
    BIN     = $(BINNAME).exe
    RM      = del /Q /F
    LDFLAGS = -L./libwin
    LDLIBS  = -lraylib -lopengl32 -lgdi32 -lwinmm -lm
else
    BIN     = ./$(BINNAME)
    RM      = rm -f
    LDFLAGS = -L./lib
    LDLIBS  = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

SRC = show.c arquivo.c perguntas.c jogo.c 
>>>>>>> ef57445 (Projeto Final)
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
<<<<<<< HEAD
	$(RM) $(OBJ) $(BINNAME) $(BINNAME).exe perguntas.bin
=======
	$(RM) $(OBJ) $(BINNAME) $(BINNAME).exe 
>>>>>>> ef57445 (Projeto Final)

.PHONY: all run debug valgrind clean
