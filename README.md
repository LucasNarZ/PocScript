# PocScript

PocScript is a study project focused on implementing the core building blocks of a programming language in C. In the current state of the repository, the main focus is the compiler frontend: source reading, tokenization, parsing, semantic validation, and AST generation/printing.

## Current State

- `lexer/`: converts source code into a linked list of tokens with `type`, `value`, `line`, and `column`
- `parser/`: consumes the token list and produces an AST for global declarations, function bodies, expressions, blocks, conditionals, loops, and arrays
- `semantic/`: walks the AST in two passes to validate declarations, scopes, types, function calls, function returns, array access, and global initializer rules
- `tests/`: contains unit and integration tests for the lexer, parser, semantic analyzer, and AST serialization
- `main.c`: main debug executable; reads `input.ps`, prints recognized tokens, prints the resulting AST, and then prints semantic analysis results

Today the project is organized as a compiler frontend. The old README described assembly generation as a central part of the flow, but the code currently present in this repository is centered on the lexer, parser, semantic analysis, and AST.

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
|-- semantic/
|   |-- semantic.c
|   |-- semantic.h
|   |-- errors.c
|   |-- errors.h
|   |-- scope.c
|   |-- scope.h
|   |-- types.c
|   `-- types.h
`-- tests/
    |-- lexer/
    |-- parser/
    |-- semantic/
    |-- integration/
    |-- helpers/
    |-- fixtures/
    |-- test_main.c
    `-- README.md
```

## Current Flow

1. `tokenizeFile("input.ps")` reads the input file and produces the token list.
2. `parserParseProgram(...)` walks through that list and builds the AST.
3. `semanticAnalyze(...)` validates declarations, scopes, expressions, calls, array access, and global initializer rules.
4. `main.c` prints the token sequence, the textual tree representation, and the semantic analysis result.

In practice, this binary works as a simple way to inspect the language frontend while the grammar evolves.

## Features Covered By The Current Code

- variable declarations with `::`
- file scope restricted to global variable declarations and function declarations
- primitive types `int`, `float`, `char`, `bool`, `void`
- `Array` type and types with `[]` suffixes
- integer, float, string, and bool literals
- binary expressions: `+`, `-`, `*`, `/`, `>`, `<`, `>=`, `<=`, `==`, `!=`, `&&`, `||`
- unary operators `!` and `-`
- assignments `=`, `+=`, `-=`
- blocks
- `if`, `else`, `while`, `for`, `break`, `continue`
- function declarations with mandatory explicit return types and `ret`
- global variable initializers restricted to direct literals
- function calls
- array access with one or more indices
- nested array literals
- semantic validation for scope, duplicate declarations, initializer types, conditions, function calls, function return contracts, loop control usage, and array indexing

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

The program reads `input.ps`, prints the recognized tokens, prints the formatted AST, and then prints semantic validation results.

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
- `semantic/README.md`: semantic analysis flow, data structures, and validation rules
- `tests/README.md`: test suite structure and the role of each group

## Notes

- The project prioritizes simplicity and readability over completeness.
- The AST has its own textual representation in `parser/ast.c`, used by the integration tests.
- The parser remains syntactic; semantic validation is handled in a separate phase under `semantic/`.
- Executable statements such as `if`, `while`, `for`, assignments, and expression statements are valid inside blocks and functions, not at file scope.
