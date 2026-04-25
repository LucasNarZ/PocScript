TARGET = output
TEST_TARGET = tests_runner
IR_TARGET = IR.ll
IR_OBJ = IR.o
RUNTIME_IO_OBJ = runtime/io.o
RUNTIME_START_OBJ = runtime/start.o

SRC = main.c ./lexer/lexer.c ./parser/parser.c ./parser/ast.c ./semantic/semantic.c ./semantic/errors.c ./semantic/types.c ./semantic/scope.c ./ir/ir_core.c ./ir/ir_instr.c ./ir/ir_module.c ./ir/ir_scope.c ./ir/ir_builder.c ./ir/ir_printer.c
TEST_SRC = tests/test_main.c tests/helpers/test_support.c tests/lexer/test_lexer.c tests/parser/test_parser.c tests/integration/test_integration.c tests/semantic/test_semantic.c tests/ir/test_ir.c ./lexer/lexer.c ./parser/parser.c ./parser/ast.c ./semantic/semantic.c ./semantic/errors.c ./semantic/types.c ./semantic/scope.c ./ir/ir_core.c ./ir/ir_instr.c ./ir/ir_module.c ./ir/ir_scope.c ./ir/ir_builder.c ./ir/ir_printer.c

OBJ = $(SRC:.c=.o)
TEST_OBJ = $(TEST_SRC:.c=.o)
TARGET_AST = compiler

CC = gcc
CLANG = clang
NASM = nasm
LD = ld
CFLAGS = -Wall -g
LDFLAGS =

default: $(TARGET_AST)

$(TARGET): $(IR_OBJ) $(RUNTIME_IO_OBJ) $(RUNTIME_START_OBJ)
	$(LD) -o $(TARGET) $(RUNTIME_START_OBJ) $(RUNTIME_IO_OBJ) $(IR_OBJ)

$(TARGET_AST): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $(TARGET_AST)

$(TEST_TARGET): $(TEST_OBJ)
	$(CC) $(CFLAGS) $(TEST_OBJ) $(LDFLAGS) -o $(TEST_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(IR_TARGET): $(TARGET_AST)
	./$(TARGET_AST)

$(IR_OBJ): $(IR_TARGET)
	$(CLANG) -c $(IR_TARGET) -o $(IR_OBJ)

$(RUNTIME_IO_OBJ): runtime/io.asm
	$(NASM) -f elf64 $< -o $@

$(RUNTIME_START_OBJ): runtime/start.asm
	$(NASM) -f elf64 $< -o $@


clean:
	rm -f $(OBJ) $(TEST_OBJ) $(TARGET) $(TARGET_AST) $(TEST_TARGET) $(IR_TARGET) $(IR_OBJ) $(RUNTIME_IO_OBJ) $(RUNTIME_START_OBJ)

ast: $(TARGET_AST)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

all: $(TARGET_AST)

assembly: $(TARGET)
