TARGET = output
TEST_TARGET = tests_runner

SRC = main.c ./lexer/lexer.c ./parser/parser.c ./parser/ast.c ./semantic/semantic.c ./semantic/errors.c ./semantic/types.c ./semantic/scope.c
TEST_SRC = tests/test_main.c tests/helpers/test_support.c tests/lexer/test_lexer.c tests/parser/test_parser.c tests/integration/test_integration.c tests/semantic/test_semantic.c ./lexer/lexer.c ./parser/parser.c ./parser/ast.c ./semantic/semantic.c ./semantic/errors.c ./semantic/types.c ./semantic/scope.c

OBJ = $(SRC:.c=.o)
TEST_OBJ = $(TEST_SRC:.c=.o)
SRC_NASM = output.asm
OBJ_NASM = output.o
TARGET_AST = compiler

ASSEMBLER = nasm
NASM_FLAGS = -f elf64

CC = gcc
CFLAGS = -Wall -g
LDFLAGS =

default: $(TARGET_AST)

$(TARGET): $(SRC_NASM)
	$(ASSEMBLER) $(NASM_FLAGS) $(SRC_NASM) -o $(OBJ_NASM)
	ld $(LDFLAGS) $(OBJ_NASM) -o $(TARGET)

$(TARGET_AST): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $(TARGET_AST)

$(TEST_TARGET): $(TEST_OBJ)
	$(CC) $(CFLAGS) $(TEST_OBJ) $(LDFLAGS) -o $(TEST_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.asm: $(TARGET_AST)
	./$(TARGET_AST)


clean:
	rm -f $(OBJ) $(TEST_OBJ) $(TARGET) $(OBJ_NASM) $(TARGET_AST) $(TEST_TARGET) $(SRC_NASM)

ast: $(TARGET_AST)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

all: $(TARGET_AST)

assembly: $(TARGET)
