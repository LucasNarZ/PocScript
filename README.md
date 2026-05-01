# PocScript

PocScript is a study project focused on implementing the core building blocks of a programming language in C. The repository currently covers the full path from source code to tokens, AST, semantic validation, internal IR, textual LLVM IR, and a final executable linked with a small assembly runtime.

## Overview

The compiler is intentionally split into small, explicit stages:

- `src/lexer/`: turns source text into a linked list of tokens with `type`, `value`, `line`, and `column`
- `src/parser/`: consumes tokens and builds an AST for declarations, statements, expressions, calls, control flow, and arrays
- `src/semantic/`: validates scopes, declarations, types, function calls, returns, loop control, builtin runtime functions, and array rules
- `src/ir/`: lowers the validated AST to an internal IR and prints LLVM IR to `build/ir/IR.ll`
- `runtime/`: provides `_start` and builtin runtime functions linked into the final executable without libc
- `tests/`: unit and integration tests for lexer, parser, semantic analysis, IR building, and IR printing
- `src/main.c`: debug-oriented compiler entry point that reads `input.ps`, runs the full pipeline, and emits `build/ir/IR.ll`

## Current Architecture

The current pipeline is:

1. `tokenizeFile("input.ps")` reads the source file and produces tokens.
2. `parserParseProgram(...)` builds the AST.
3. `semanticAnalyze(...)` validates the AST in two passes.
4. `irBuildModule(...)` lowers the validated AST to the internal IR.
5. `irPrintModuleToFile(...)` prints LLVM IR to `build/ir/IR.ll`.
6. `make assembly` compiles `build/ir/IR.ll`, assembles the runtime, and links everything into `build/bin/output`.

In practice, there are two main artifacts:

- `build/bin/compiler`: useful for inspecting parsing, semantic validation, and IR generation
- `build/bin/output`: the final executable built from generated LLVM IR plus the runtime layer

## Design Decisions

The project makes a few explicit design choices to keep the implementation understandable.

### Simple staged architecture

Each phase has one clear responsibility.

- the lexer only classifies text
- the parser is syntactic only
- semantic validation is isolated from parsing
- IR generation only runs after semantic success

This separation keeps failure modes easier to understand and avoids mixing syntax, typing, and code generation logic in the same pass.

### AST first, semantics second

The parser builds a full AST without trying to resolve identifiers or types on the fly. That makes the parser simpler and keeps semantic rules centralized in `src/semantic/`.

### Two-pass semantic analysis

Semantic analysis first collects symbols, then validates usage. This is a deliberate choice so the compiler can:

- support forward references to functions
- separate declaration collection from type checking
- accumulate multiple semantic errors in one run instead of aborting immediately

### Small LLVM-like IR

The backend does not emit LLVM IR directly from the AST. Instead, it lowers to a small internal IR first and then prints LLVM IR text. This keeps lowering logic independent from LLVM syntax details and makes the backend easier to test in isolation.

### Stack-slot locals instead of aggressive SSA

Named local variables are materialized as stack slots with `alloca`, `store`, and `load`. Temporary expression results still use numbered temporaries such as `%t1`, `%t2`, and `%t3`, but the compiler does not implement full SSA construction or phi nodes yet.

This is a conscious simplicity tradeoff: the generated IR is easier to produce and reason about, even though it is not yet optimized.

### Minimal runtime and no libc dependency

The generated executable is linked directly with `ld` and a tiny assembly runtime. `_start` is provided by `runtime/start.asm`, and runtime helpers such as `printString` and `printInt` live in `runtime/io.asm`.

This keeps the backend close to the machine model and makes the final artifact easier to understand end-to-end.

### End-to-end language as the long-term goal

The long-term goal of PocScript is to make the language work end to end with as many self-hosted or project-owned stages as possible.

That means the intended direction is not to stop at:

- AST -> LLVM IR -> `clang` -> object file

The intended direction is to progressively replace borrowed stages with project-controlled ones, for example:

- AST -> internal IR -> handwritten NASM x86 assembly
- handwritten assembly emission -> project-controlled object or binary generation in later stages

The current use of LLVM IR is a pragmatic design decision, not the final architectural destination. It was chosen so the language could become executable as early as possible, while the frontend and IR model were still being designed.

The same strategy may be used again in future phases: use a practical intermediate step first, then replace it with a more direct implementation once the surrounding compiler stages are stable enough.

In other words, the project tries to build as much as possible from scratch, but without blocking execution on finishing every low-level stage first.

## Project Structure

```text
.
|-- build/
|-- include/
|   |-- ast.h
|   |-- constants.h
|   |-- errors.h
|   |-- ir.h
|   |-- lexer.h
|   |-- parser.h
|   |-- scope.h
|   |-- semantic.h
|   |-- token.h
|   `-- types.h
|-- input.ps
|-- makefile
|-- src/
|   |-- main.c
|   |-- lexer/
|   |   |-- lexer.c
|   |   `-- README.md
|   |-- parser/
|   |   |-- parser.c
|   |   |-- ast.c
|   |   `-- README.md
|   |-- semantic/
|   |   |-- semantic.c
|   |   |-- errors.c
|   |   |-- scope.c
|   |   |-- types.c
|   |   `-- README.md
|   `-- ir/
|       |-- ir_core.c
|       |-- ir_instr.c
|       |-- ir_module.c
|       |-- ir_scope.c
|       |-- ir_builder.c
|       |-- ir_printer.c
|       `-- README.md
|-- runtime/
|   |-- io.asm
|   |-- start.asm
|   `-- README.md
`-- tests/
    |-- lexer/
    |-- parser/
    |-- semantic/
    |-- integration/
    |-- helpers/
    |-- fixtures/
    |-- ir/
    |-- test_main.c
    `-- README.md
```

## Language Coverage

The current codebase supports:

- variable declarations with `::`
- top-level global variable declarations and function declarations only
- primitive types `int`, `float`, `char`, `bool`, and `void`
- pointer types with `*T`
- `Array` and `[]`-style array types
- integer, float, string, and bool literals
- array literals, including nested array literals
- assignments `=`, `+=`, and `-=`
- binary expressions including arithmetic, comparison, and logical operators across the active pipeline
- unary operators `!`, `-`, `&`, and `*` across the active pipeline
- blocks
- `if`, `else`, `while`, `for`, `break`, and `continue`
- function declarations with explicit return types and `ret`
- function calls
- builtin runtime calls to `printString` and `printInt`
- array access syntax with one or more indices across the active pipeline
- pointer address-of and dereference syntax across the active pipeline
- LLVM IR emission for globals, functions, control flow, array access, string storage, and external runtime declarations
- final executable generation with `_start` and a libc-free runtime linked by `ld`

## Feature Status

The table below distinguishes between features that are currently supported across the full implemented pipeline.

| Feature | Status | Notes |
| --- | --- | --- |
| Global variable declarations | `fully-supported` | Parsed, semantically validated, lowered to IR, and emitted to LLVM IR |
| Function declarations | `fully-supported` | Explicit return type required |
| Primitive types `int`, `float`, `char`, `bool`, `void` | `fully-supported` | Within the currently implemented semantic and backend rules |
| String literals | `fully-supported` | Lowered through generated string storage globals |
| Bool literals | `fully-supported` | Validated semantically and emitted in the backend |
| Integer and float literals | `fully-supported` | Supported by frontend and backend |
| Local variable declarations | `fully-supported` | Lowered with stack slots |
| Function calls | `fully-supported` | Includes calls to user functions and runtime builtins |
| `printString` and `printInt` | `fully-supported` | Declared in semantic analysis, IR, and resolved by runtime linking |
| `if` / `else` | `fully-supported` | Lowered to explicit blocks and branches |
| `while` | `fully-supported` | Lowered to explicit loop blocks |
| `for` | `fully-supported` | Lowered to init/cond/body/update/end blocks |
| `ret` | `fully-supported` | Includes default return insertion when needed |
| Assignments `=` | `fully-supported` | Supported for addressable targets in current backend path |
| Compound assignments `+=` and `-=` | `fully-supported` | Lowered through load/compute/store |
| Single-index array access | `fully-supported` | Lowered with `getelementptr` in current backend |
| Array literals | `fully-supported` | Current backend supports element-by-element initialization |
| Nested array literals | `fully-supported` | Lowered recursively with element-by-element initialization |
| Multi-index array access | `fully-supported` | Lowered through multi-step `getelementptr` index lists |
| Binary arithmetic `+`, `-`, `*`, `/` | `fully-supported` | Lowered in current backend |
| Comparisons `>`, `<`, `<=` | `fully-supported` | Lowered in current backend |
| Comparisons `>=`, `==`, `!=` | `fully-supported` | Lowered and emitted as LLVM comparisons in the active backend path |
| Logical operators `&&`, `||` | `fully-supported` | Lowered and emitted as boolean LLVM operations |
| Unary `!` and unary `-` | `fully-supported` | Lowered and emitted in the active backend path |
| Pointer types `*T` | `fully-supported` | Parsed, validated structurally, and lowered to LLVM pointer types |
| Address-of `&` | `fully-supported` | Supported for variables and array elements |
| Dereference `*p` | `fully-supported` | Supported for reads and simple assignment targets |
| `break` and `continue` | `fully-supported` | Lowered through explicit loop target branches |

`fully-supported` means the feature is implemented coherently across the currently active pipeline: parser, semantic analysis, IR generation, LLVM IR emission, and executable generation.

## Current Language And Backend Limitations

The project is intentionally incomplete, but the currently documented language surface is supported across the active pipeline.

### Language limitations

- file scope is restricted to global variable declarations and function declarations
- function declarations require an explicit return type after `->`
- global variable initializers must be direct literals
- builtin I/O is currently limited to `printString` and `printInt`
- the language has no user-defined structs, records, classes, enums, modules, or generics
- there is no explicit memory management model exposed to the language
- there is no standard library beyond the tiny runtime helpers
- pointer support is intentionally minimal: no pointer arithmetic, no `null`, and no implicit array-to-pointer decay

### Semantic limitations

- type rules are intentionally strict
- there is no implicit numeric promotion between `int` and `float`
- semantic analysis focuses on correctness, not optimization or recovery of malformed syntax

### Backend limitations

- locals are always lowered through stack slots rather than promoted values
- there are no SSA phi nodes
- the backend depends on semantic analysis to reject unsupported or inconsistent programs before IR generation
- code generation is focused on readability and correctness, not optimization quality

## Build And Run

### Requirements

- `gcc`
- `clang`
- `nasm`
- `ld`
- `make`

### Build the compiler

```bash
make
```

This generates `build/bin/compiler`.

### Run with the default input file

```bash
./build/bin/compiler
```

The program reads `input.ps`, parses it, runs semantic validation, and writes `build/ir/IR.ll` when the program is valid. When semantic errors exist, they are printed with line and column information and IR generation is skipped.

### Build the final executable

```bash
make assembly
```

This performs the full backend build:

1. runs `./build/bin/compiler` to generate `build/ir/IR.ll`
2. compiles `build/ir/IR.ll` into `build/obj/IR.o` with `clang -c`
3. assembles `runtime/io.asm` into `build/obj/runtime/io.o`
4. assembles `runtime/start.asm` into `build/obj/runtime/start.o`
5. links `build/obj/IR.o`, `build/obj/runtime/io.o`, and `build/obj/runtime/start.o` into `build/bin/output` with `ld`

### Run the final executable

```bash
./build/bin/output
```

The final program starts at `_start`, calls the generated `main`, uses the runtime functions linked from `runtime/`, and exits through Linux syscalls without libc.

### Run the tests

```bash
make test
```

This builds and runs `build/bin/tests_runner`.

### Clean artifacts

```bash
make clean
```

## Internal Documentation

- `src/lexer/README.md`: lexer overview, token categories, and tokenization flow
- `src/parser/README.md`: parser organization and AST structure
- `src/semantic/README.md`: semantic analysis flow, data structures, and validation rules
- `src/ir/README.md`: IR model, lowering strategy, LLVM printer behavior, and backend limits
- `runtime/README.md`: runtime entry point, exported symbols, and link flow
- `tests/README.md`: test suite structure and the role of each group

## Architecture Notes

- the parser remains purely syntactic; all semantic rules live in `src/semantic/`
- builtin runtime functions are treated as global functions during semantic analysis and IR generation
- AST textual serialization in `src/parser/ast.c` is part of the practical contract because integration tests depend on it
- the compiler favors simple explicit data structures over compact or heavily abstracted implementations
- the project prioritizes readability and phase separation over performance and feature completeness
