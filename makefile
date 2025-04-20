TARGET = output

SRC = main.c ./lexer/lexer.c ./parser/parser.c ./utils/utils.c ./parser/ast.c ./assemblyGenerator/generator.c 

OBJ = $(SRC:.c=.o)
SRC_NASM = output.asm
OBJ_NASM = output.o
TARGET_AST = compiler

ASSEMBLER = nasm
NASM_FLAGS = -f elf64

CC = gcc
CFLAGS = -Wall -g

$(TARGET): $(SRC_NASM)
	$(ASSEMBLER) $(NASM_FLAGS) $(SRC_NASM) -o $(OBJ_NASM)
	ld $(OBJ_NASM) -o $(TARGET)

$(TARGET_AST): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET_AST)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.asm: $(TARGET_AST)
	./$(TARGET_AST)


clean:
	rm -f $(OBJ) $(TARGET) $(OBJ_NASM) $(TARGET_AST) $(SRC_NASM)

ast: $(TARGET_AST)

test: test.asm
	nasm -f elf64 test.asm -o test.o
	ld test.o -o test

all: $(TARGET)