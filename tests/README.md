# Tests

The `tests/` directory contains the automated validation for PocScript's current frontend. The general idea of the suite is to ensure that the lexer and parser produce consistent structures and that syntax errors are reported with predictable messages and positions.

## Organization

- `test_main.c`: runner entry point; registers and executes all tests
- `helpers/`: shared assertion macros and helper functions
- `lexer/`: unit tests for the lexical analyzer
- `parser/`: unit tests for the parser and syntax errors
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
- `nodeTreeToString`: converts the AST into the textual format used by the tests

## How To Run

```bash
make test
```

This target builds `tests_runner` and executes the full suite.

## General Idea

The suite is designed to protect the current frontend contract. Instead of testing optimizations or semantics, it mainly verifies:

- token format
- AST structure
- basic quality of parsing errors
- stability of fixture outputs
