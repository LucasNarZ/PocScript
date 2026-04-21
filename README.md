# PocScript

PocScript is a study project focused on implementing the core building blocks of a programming language in C. In the current state of the repository, the main focus is the compiler frontend: source reading, tokenization, parsing, and AST generation/printing.

## Current State

- `lexer/`: converts source code into a linked list of tokens with `type`, `value`, `line`, and `column`
- `parser/`: consumes the token list and produces an AST for declarations, expressions, blocks, functions, conditionals, loops, and arrays
- `tests/`: contains unit and integration tests for the lexer, parser, and AST serialization
- `main.c`: main debug executable; reads `input.ps`, prints recognized tokens, and then prints the resulting AST

Today the project is organized as a compiler frontend. The old README described assembly generation as a central part of the flow, but the code currently present in this repository is centered on the lexer, parser, and AST.

## Project Structure

```text
.
|-- main.c
|-- constants.h
|-- input.ps
|-- makefile
|-- lexer/
|   |-- lexer.c
|   |-- lexer.h
|   |-- token.h
|   `-- README.md
|-- parser/
|   |-- parser.c
|   |-- parser.h
|   |-- ast.c
|   |-- ast.h
|   `-- README.md
`-- tests/
    |-- lexer/
    |-- parser/
    |-- integration/
    |-- helpers/
    |-- fixtures/
    |-- test_main.c
    `-- README.md
```

## Current Flow

1. `tokenizeFile("input.ps")` reads the input file and produces the token list.
2. `parserParseProgram(...)` walks through that list and builds the AST.
3. `main.c` prints the token sequence and then the textual tree representation.

In practice, this binary works as a simple way to inspect the language frontend while the grammar evolves.

## Features Covered By The Current Code

- variable declarations with `::`
- primitive types `int`, `float`, `char`, `bool`
- `Array` type and types with `[]` suffixes
- integer, float, string, and bool literals
- binary expressions: `+`, `-`, `*`, `/`, `>`, `<`, `>=`, `<=`, `==`, `!=`, `&&`, `||`
- unary `!` operator
- assignments `=`, `+=`, `-=`
- blocks
- `if`, `else`, `while`, `for`
- function declarations and `ret`
- function calls
- array access with one or more indices
- nested array literals

## Build And Run

### Requirements

- `gcc`
- `make`

### Build the frontend

```bash
make
```

This generates the `compiler` executable.

### Run with the default input file

```bash
./compiler
```

The program reads `input.ps`, prints the recognized tokens, and then prints the formatted AST.

### Run the tests

```bash
make test
```

This builds and runs `tests_runner`.

### Clean artifacts

```bash
make clean
```

## Internal Documentation

- `lexer/README.md`: lexer overview, token categories, and tokenization flow
- `parser/README.md`: parser organization and general AST format
- `tests/README.md`: test suite structure and the role of each group

## Notes

- The project prioritizes simplicity and readability over completeness.
- The AST has its own textual representation in `parser/ast.c`, used by the integration tests.
- The parser is syntactic: the tests validate structure and parsing errors, not semantic analysis.
