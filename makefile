TARGET = $(BINDIR)/output
TEST_TARGET = $(BINDIR)/tests_runner
IR_TARGET = $(IRDIR)/IR.ll
IR_OBJ = $(OBJDIR)/IR.o
RUNTIME_IO_OBJ = $(OBJDIR)/runtime/io.o
RUNTIME_START_OBJ = $(OBJDIR)/runtime/start.o

SRC = src/main.c src/lexer/lexer.c src/parser/parser.c src/parser/ast.c src/semantic/semantic.c src/semantic/errors.c src/semantic/types.c src/semantic/scope.c src/ir/ir_core.c src/ir/ir_instr.c src/ir/ir_module.c src/ir/ir_scope.c src/ir/ir_builder.c src/ir/ir_printer.c
TEST_SRC = tests/test_main.c tests/helpers/test_support.c tests/lexer/test_lexer.c tests/parser/test_parser.c tests/integration/test_integration.c tests/semantic/test_semantic.c tests/ir/test_ir.c src/lexer/lexer.c src/parser/parser.c src/parser/ast.c src/semantic/semantic.c src/semantic/errors.c src/semantic/types.c src/semantic/scope.c src/ir/ir_core.c src/ir/ir_instr.c src/ir/ir_module.c src/ir/ir_scope.c src/ir/ir_builder.c src/ir/ir_printer.c

OBJ = $(addprefix $(OBJDIR)/,$(SRC:.c=.o))
TEST_OBJ = $(addprefix $(OBJDIR)/,$(TEST_SRC:.c=.o))
TARGET_AST = $(BINDIR)/compiler

BINDIR = build/bin
OBJDIR = build/obj
IRDIR = build/ir

CC = gcc
CLANG = clang
NASM = nasm
LD = ld
CPPFLAGS = -Iinclude
CFLAGS = -Wall -g
LDFLAGS =

default: $(TARGET_AST)

$(TARGET): $(IR_OBJ) $(RUNTIME_IO_OBJ) $(RUNTIME_START_OBJ)
	@mkdir -p $(dir $@)
	$(LD) -o $(TARGET) $(RUNTIME_START_OBJ) $(RUNTIME_IO_OBJ) $(IR_OBJ)

$(TARGET_AST): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $(TARGET_AST)

$(TEST_TARGET): $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(TEST_OBJ) $(LDFLAGS) -o $(TEST_TARGET)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(IR_TARGET): $(TARGET_AST)
	@mkdir -p $(dir $@)
	./$(TARGET_AST)

$(IR_OBJ): $(IR_TARGET)
	@mkdir -p $(dir $@)
	$(CLANG) -c $(IR_TARGET) -o $(IR_OBJ)

$(RUNTIME_IO_OBJ): runtime/io.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 $< -o $@

$(RUNTIME_START_OBJ): runtime/start.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 $< -o $@


clean:
	rm -rf build

ast: $(TARGET_AST)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

all: $(TARGET_AST)

assembly: $(TARGET)
