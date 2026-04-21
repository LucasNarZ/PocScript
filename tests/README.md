# Tests

The `tests/` directory contains the automated validation for PocScript's current frontend. The general idea of the suite is to ensure that the lexer, parser, and semantic analyzer produce consistent structures and predictable diagnostics.

## Organization

- `test_main.c`: runner entry point; registers and executes all tests
- `helpers/`: shared assertion macros and helper functions
- `lexer/`: unit tests for the lexical analyzer
- `parser/`: unit tests for the parser and syntax errors
- `semantic/`: unit tests for scope, symbol resolution, type checking, function calls, function returns, and array validation
- `integration/`: full-flow tests, from input to serialized AST
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

## Helpers

The main helpers are:

- `EXPECT_TRUE` and `EXPECT_STR_EQ`: simple assertion macros
- `RUN_TEST`: records each test result and prints `PASS` or `FAIL`
- `parseRootFromString`: helper to tokenize and parse in one call
- `analyzeRootFromString`: helper to tokenize, parse, and run semantic analysis in one call
- `nodeTreeToString`: converts the AST into the textual format used by the tests

## How To Run

```bash
make test
```

This target builds `tests_runner` and executes the full suite.

## General Idea

The suite is designed to protect the current frontend contract. It mainly verifies:

- token format
- AST structure
- basic quality of parsing errors
- semantic diagnostics and recovery
- stability of fixture outputs
