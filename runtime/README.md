# Runtime

The `runtime/` directory contains the low-level assembly runtime plus the PocScript standard library modules linked into the final executable.

## Files

- `runtime.asm`: syscall-backed primitive runtime symbols such as `__poc_write`
- `start.asm`: process entry point that defines `_start` and calls the generated `main`
- `lib/string.ps`: libc-like string helpers compiled from PocScript
- `lib/memory.ps`: libc-like memory helpers compiled from PocScript
- `lib/io.ps`: libc-like stdout helper functions compiled from PocScript

## Runtime vs Stdlib

The runtime and the stdlib now have separate responsibilities:

- assembly runtime exports low-level primitives like `__poc_write`
- stdlib modules export libc-like functions such as `strlen`, `strcmp`, `strcpy`, `strncpy`, `memcpy`, `memset`, and `puts`

User code consumes stdlib functions through explicit `extern func ...` declarations, and `make assembly` links the compiled stdlib objects automatically.

## `runtime.asm`

`runtime.asm` currently exports:

- `__poc_write`: receives the Linux `write` syscall arguments and performs the syscall directly

The implementation does not use libc.

## `start.asm`

`start.asm` defines the executable entry point:

1. `_start` calls the generated `main`
2. the return value from `main` is moved to the process exit status register
3. the program exits with the Linux `exit` syscall

This keeps process startup separate from runtime helper functions and allows the final executable to be linked directly with `ld`.

## Build Integration

The `makefile` assembles, compiles, and links this directory through these stages:

1. `./build/bin/compiler input.ps build/ir/input.ll`
2. `./build/bin/compiler runtime/lib/string.ps build/ir/runtime/lib/string.ll`
3. `./build/bin/compiler runtime/lib/memory.ps build/ir/runtime/lib/memory.ll`
4. `./build/bin/compiler runtime/lib/io.ps build/ir/runtime/lib/io.ll`
5. `clang -c ...` compiles each generated `.ll` into its matching `.o`
6. `nasm -f elf64 runtime/runtime.asm -o build/obj/runtime/runtime.o`
7. `nasm -f elf64 runtime/start.asm -o build/obj/runtime/start.o`
8. `ld -o build/bin/output build/obj/runtime/start.o build/obj/runtime/runtime.o build/obj/runtime/lib/*.o build/obj/input.o`

Use `make assembly` to build the final executable.
