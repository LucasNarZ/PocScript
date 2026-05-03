# Tests

The `tests/` directory contains the automated validation for PocScript's current frontend and backend. The general idea of the suite is to ensure that the lexer, parser, semantic analyzer, and IR generation produce consistent structures and predictable diagnostics.

## Organization

- `test_main.c`: runner entry point; registers and executes all tests
- `helpers/`: shared assertion macros and helper functions
- `lexer/`: unit tests for the lexical analyzer
- `parser/`: unit tests for the parser and syntax errors
- `semantic/`: unit tests for scope, symbol resolution, type checking, function calls, function returns, and array validation
- `ir/`: unit tests for IR construction and LLVM IR printing
- `integration/`: full-flow tests from tokenization through AST and selected IR emission checks
- `stdlib/`: focused end-to-end tests for PocScript stdlib functions compiled and linked with the runtime
- `fixtures/`: `.ps` files and expected outputs used by the tests

## What Each Group Validates

### `tests/lexer/`

Checks whether the lexer:

- recognizes the expected tokens
- preserves order and values
- records line and column correctly
- produces a stable textual representation for fixture comparison

### `tests/parser/`

Checks whether the parser:

- builds the expected AST for declarations and expressions
- recognizes calls, array access, grouping, and literals
- copies token strings into the AST
- fails predictably on invalid inputs
- reports error messages with correct line and column information

### `tests/semantic/`

Checks whether the semantic analyzer:

- detects duplicate declarations in the same scope
- allows valid shadowing in nested scopes
- reports undeclared identifiers and functions
- validates initializer, expression, call argument, and function return types
- validates boolean conditions and array indexing
- accumulates multiple semantic errors in one analysis pass

### `tests/integration/`

Validates the combined flow:

- reading an input fixture
- complete tokenization with a single `EOF`
- parsing the whole program
- comparing the serialized AST against an expected file
- selected end-to-end IR expectations for globals, arrays, control flow, and runtime calls

### `tests/ir/`

Checks whether the IR layer:

- predeclares globals, functions, and builtin runtime symbols
- lowers expressions, control flow, array access, and array literals
- preserves sized array information where required
- prints valid LLVM IR fragments for the active backend path
- writes LLVM IR to files when requested

### `tests/stdlib/`

Checks whether the stdlib pipeline:

- compiles standalone PocScript stdlib modules correctly
- links stdlib objects with a small user program and the assembly runtime
- preserves expected behavior for `strlen`, `strcmp`, `strcpy`, `strncpy`, `memcpy`, `memset`, and `puts`

## Helpers

The main helpers are:

- `EXPECT_TRUE` and `EXPECT_STR_EQ`: simple assertion macros
- `RUN_TEST`: records each test result and prints `PASS` or `FAIL`
- `parseRootFromString`: helper to tokenize and parse in one call
- `analyzeRootFromString`: helper to tokenize, parse, and run semantic analysis in one call
- `nodeTreeToString`: converts the AST into the textual format used by the tests
- `compileAndRunStdlibProgram`: compiles a temporary program plus stdlib modules, links them, runs the executable, and captures stdout

## How To Run

```bash
make test
```

This target builds `build/bin/tests_runner` and executes the full suite.

## General Idea

The suite is designed to protect the current frontend and backend contracts. It mainly verifies:

- token format
- AST structure
- basic quality of parsing errors
- semantic diagnostics and recovery
- IR construction and LLVM IR printing behavior
- stability of fixture outputs
