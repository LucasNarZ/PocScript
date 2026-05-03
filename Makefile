TARGET = $(BINDIR)/output
TEST_TARGET = $(BINDIR)/tests_runner

INPUT_PS = input.ps
INPUT_IR = $(IRDIR)/input.ll
INPUT_OBJ = $(OBJDIR)/input.o

STDLIB_SRC = runtime/lib/string.ps runtime/lib/memory.ps runtime/lib/io.ps
STDLIB_IR = $(IRDIR)/runtime/lib/string.ll $(IRDIR)/runtime/lib/memory.ll $(IRDIR)/runtime/lib/io.ll
STDLIB_OBJ = $(OBJDIR)/runtime/lib/string.o $(OBJDIR)/runtime/lib/memory.o $(OBJDIR)/runtime/lib/io.o

RUNTIME_OBJ = $(OBJDIR)/runtime/runtime.o
RUNTIME_START_OBJ = $(OBJDIR)/runtime/start.o

SRC = src/main.c src/compiler_driver.c src/lexer/lexer.c src/parser/parser.c src/parser/ast.c src/semantic/semantic.c src/semantic/errors.c src/semantic/types.c src/semantic/scope.c src/ir/ir_core.c src/ir/ir_instr.c src/ir/ir_module.c src/ir/ir_scope.c src/ir/ir_builder_utils.c src/ir/ir_builder_module.c src/ir/ir_builder_expr.c src/ir/ir_builder_stmt.c src/ir/ir_printer.c
TEST_SRC = tests/test_main.c tests/helpers/test_support.c tests/helpers/stdlib_test_support.c tests/lexer/test_lexer.c tests/parser/test_parser.c tests/integration/test_integration.c tests/semantic/test_semantic.c tests/ir/test_ir.c tests/stdlib/test_stdlib.c src/compiler_driver.c src/lexer/lexer.c src/parser/parser.c src/parser/ast.c src/semantic/semantic.c src/semantic/errors.c src/semantic/types.c src/semantic/scope.c src/ir/ir_core.c src/ir/ir_instr.c src/ir/ir_module.c src/ir/ir_scope.c src/ir/ir_builder_utils.c src/ir/ir_builder_module.c src/ir/ir_builder_expr.c src/ir/ir_builder_stmt.c src/ir/ir_printer.c

OBJ = $(addprefix $(OBJDIR)/,$(SRC:.c=.o))
TEST_OBJ = $(addprefix $(OBJDIR)/,$(TEST_SRC:.c=.o))
TARGET_AST = $(BINDIR)/compiler

BINDIR = build/bin
OBJDIR = build/obj
IRDIR = build/ir

CC = gcc
CLANG = clang
CLANG_IR_FLAGS = -Wno-override-module
NASM = nasm
LD = ld
CPPFLAGS = -Iinclude
CFLAGS = -Wall -g
LDFLAGS =

default: $(TARGET_AST)

$(TARGET): $(INPUT_OBJ) $(STDLIB_OBJ) $(RUNTIME_OBJ) $(RUNTIME_START_OBJ)
	@mkdir -p $(dir $@)
	$(LD) -o $(TARGET) $(RUNTIME_START_OBJ) $(RUNTIME_OBJ) $(STDLIB_OBJ) $(INPUT_OBJ)

$(TARGET_AST): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $(TARGET_AST)

$(TEST_TARGET): $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(TEST_OBJ) $(LDFLAGS) -o $(TEST_TARGET)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


$(IRDIR)/%.ll: %.ps $(TARGET_AST)
	@mkdir -p $(dir $@)
	./$(TARGET_AST) $< $@

$(OBJDIR)/%.o: $(IRDIR)/%.ll
	@mkdir -p $(dir $@)
	$(CLANG) $(CLANG_IR_FLAGS) -c $< -o $@

$(INPUT_IR): $(INPUT_PS) $(TARGET_AST)
	@mkdir -p $(dir $@)
	./$(TARGET_AST) $< $@

$(INPUT_OBJ): $(INPUT_IR)
	@mkdir -p $(dir $@)
	$(CLANG) $(CLANG_IR_FLAGS) -c $< -o $@

$(RUNTIME_OBJ): runtime/runtime.asm
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
