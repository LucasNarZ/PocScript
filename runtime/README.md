# Runtime

The `runtime/` directory contains the assembly code linked into the final executable generated from `IR.ll`.

## Files

- `io.asm`: runtime I/O helpers exported to the generated program
- `start.asm`: process entry point that defines `_start` and calls the generated `main`

## Exported Runtime Functions

The language currently exposes these runtime functions to user code:

- `printString(string) -> void`
- `printInt(int) -> void`

These names are registered as builtins in semantic analysis, mirrored into the IR global symbol table, emitted as external LLVM declarations, and resolved by `ld` against `runtime/io.o`.

## `io.asm`

`io.asm` exports two global symbols:

- `printString`: receives a null-terminated string pointer in `rdi`, computes its length, and writes it to stdout with the Linux `write` syscall
- `printInt`: receives an integer in `rdi`, converts it to ASCII on the stack, and forwards the resulting string to `printString`

The implementation does not use libc.

## `start.asm`

`start.asm` defines the executable entry point:

1. `_start` calls the generated `main`
2. the return value from `main` is moved to the process exit status register
3. the program exits with the Linux `exit` syscall

This keeps process startup separate from runtime helper functions and allows the final executable to be linked directly with `ld`.

## Build Integration

The `makefile` assembles and links this directory through these stages:

1. `nasm -f elf64 runtime/io.asm -o runtime/io.o`
2. `nasm -f elf64 runtime/start.asm -o runtime/start.o`
3. `clang -c IR.ll -o IR.o`
4. `ld -o output runtime/start.o runtime/io.o IR.o`

Use `make assembly` to build the final executable.
