TARGET = compiler

SRC = main.c ./lexer/lexer.c ./parser/parser.c ./utils/utils.c ./parser/ast.c

OBJ = $(SRC:.c=.o)

CC = gcc
CFLAGS = -Wall -g

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

# Regra padrão para o Makefile (se você rodar `make` sem argumentos)
all: $(TARGET)