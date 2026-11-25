CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
SRC = show.c arquivo.c perguntas.c jogo.c
OBJ = $(SRC:.c=.o)
BIN = jogo-milhao

ifeq ($(OS),Windows_NT)
  LDLIBS = -lraylib -lopengl32 -lgdi32 -lwinmm -lm
  RM = del /Q /F
else
  LDLIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
  RM = rm -f
endif

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

%.o: %.c quiz.h
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(BIN)

clean:
	$(RM) $(OBJ) $(BIN) $(BIN).exe perguntas.bin perguntas_exportadas.csv

.PHONY: all run clean
