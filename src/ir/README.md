# IR

The `src/ir/` directory contains PocScript's lowering stage from validated AST nodes to an internal intermediate representation and, from there, to textual LLVM IR.

## Files

- `ir.h`: shared IR data model, public constructors, symbol/scope API, builder API, and printer API
- `ir_core.c`: creation and destruction of types, literals, operands, and generic IR values
- `ir_instr.c`: instruction allocation and cleanup
- `ir_module.c`: module, function, global, and basic block storage
- `ir_scope.c`: IR symbol table and nested scope chain
- `ir_builder.c`: lowering from AST + semantic result into the IR module
- `ir_printer.c`: conversion from the internal IR module to LLVM IR text

## Role In The Compiler

The backend flow is currently:

1. The parser builds the AST.
2. The semantic analyzer validates the AST and produces `SemanticResult`.
3. `irBuildModule(...)` lowers the validated tree into an `IRModule`.
4. `irPrintModuleToFile(...)` writes the LLVM IR text to `build/ir/IR.ll`.
5. `clang` compiles `build/ir/IR.ll` and `ld` links it with the runtime objects.

`irBuildModule(...)` refuses to run when semantic analysis reported errors. The IR layer assumes the input program is already semantically valid.

## High-Level Design

The internal IR is LLVM-like, but intentionally small.

It models:

- a module with globals and functions
- functions split into basic blocks
- instructions inside each basic block
- operands for temporaries, parameters, globals, literals, and block labels
- a symbol table chain used during lowering

The representation is not fully SSA in the classical sense. Temporary instruction results use numbered locals such as `%t1`, `%t2`, and `%t3`, but named locals are materialized as stack slots with `alloca`, `store`, and `load`.

## Main Data Structures

### `IRModule`

Defined in `ir.h`.

It owns:

- `global_scope`: root symbol scope for globals, user functions, and builtin runtime functions
- `global_items[]`: all global storage entries, including compiler-generated string storage globals
- `functions[]`: all function definitions collected from the AST

### `IRFunction`

Each function stores:

- `name`
- `return_type`
- `scope`: function-local scope whose parent is the module global scope
- `params[]`: parameter names, parameter types, and their parameter ids
- `blocks[]`: ordered basic blocks
- `next_local_id`: source for temporary ids
- `next_block_id`: source for basic block ids

### `IRBasicBlock`

Each block has:

- `id`
- `instructions[]`

Blocks are printed as LLVM labels in the form `bbN`.

### `IRType`

The IR type system is recursive and currently supports:

- `IR_TYPE_VOID`
- `IR_TYPE_INT`
- `IR_TYPE_FLOAT`
- `IR_TYPE_CHAR`
- `IR_TYPE_BOOL`
- `IR_TYPE_STRING`
- `IR_TYPE_ARRAY`
- `IR_TYPE_POINTER`

Arrays and pointers reference an `element_type`. Arrays can optionally carry a fixed `array_size`.

Important LLVM mappings used by the printer:

- `int` -> `i32`
- `float` -> `double`
- `char` -> `i8`
- `bool` -> `i1`
- `string` -> `i8*`
- arrays -> `[N x T]`
- pointers -> `T*`

### `IRLiteral`

Literals are immediate constants attached to operands or globals.

Current literal kinds are:

- integer
- float
- string
- bool

### `IROperand`

Operands are typed references used by instructions.

Current operand kinds are:

- local temporary id
- function parameter id
- global id
- literal value
- basic block id

### `IRInstruction`

Instructions optionally produce a result id and store instruction-specific payload in a tagged union.

The enum contains more variants than the current lowering path emits, but the printer now covers the instruction kinds used by the active backend path described below.

### `IRScope`, `IRSymbolTable`, and `IRSymbol`

The lowering phase maintains its own scope chain instead of directly reusing semantic symbols.

Each symbol stores:

- `name`
- `kind`: global, local, or function
- `type`
- `global_id`, `local_id`, or function signature metadata depending on the symbol kind

Lookup walks the parent chain, so function lowering can resolve:

- locals in the current block/function scope
- function parameters materialized as locals
- globals
- builtin runtime functions declared in the module scope

## Lowering Strategy

`irBuildModule(...)` runs in four major stages.

### 1. Builtin declaration

Before touching the program AST, the builder inserts builtin runtime function symbols into the global scope:

- `printString(string) -> void`
- `printInt(int) -> void`

These are not emitted as function definitions. The printer later emits them as external LLVM declarations.

### 2. Global and function predeclaration

The builder performs a first pass over the program items.

For global variable declarations, it:

- converts the AST declared type to `IRType`
- converts literal initializers to `IRLiteral` when present
- creates an `IRGlobal`
- registers the symbol in the module global scope

For function declarations, it:

- creates an `IRFunction`
- records parameter metadata
- registers a function symbol with return type and parameter types
- appends the function to the module

This pass ensures later lowering can resolve forward references to functions and globals.

### 3. String literal collection

The builder traverses the whole AST and creates private storage globals for every string literal it finds.

Each string literal becomes a generated global such as `.str.1` with:

- array type `[N x i8]`
- `private unnamed_addr constant`
- a null terminator appended by the printer

String globals used as source-level global initializers are also connected to their storage entry through `storage_global_id`.

### 4. Function body lowering

Each function body is lowered after predeclaration.

The builder:

1. creates an entry basic block
2. materializes each parameter into a stack slot with `alloca` + `store`
3. lowers each statement in order
4. inserts a default `ret` when the current block does not already end with a terminator

Default returns are zero-like values based on the declared return type.

## Expression And Statement Lowering

### Locals and identifiers

Named local variables are represented as pointers to stack slots.

- reading a local lowers to `load` from that slot
- assigning to a local lowers to `store` into that slot

Globals are treated similarly, except their address is a global operand like `@x` instead of a local temporary.

### Variable declarations

Local declarations lower to:

1. `alloca` for the declared type
2. symbol registration in the current scope
3. optional initialization with `store`

Array literals, including nested array literals, are lowered element-by-element with `getelementptr` followed by `store` for each slot.

### Binary expressions

The builder currently emits IR for:

- `+` -> `add`
- `-` -> `sub`
- `*` -> `mul`
- `/` -> `sdiv`
- `&&` -> `and`
- `||` -> `or`
- `>` -> `icmp sgt`
- `<` -> `icmp slt`
- `<=` -> `icmp sle`
- `>=` -> `icmp sge`
- `==` -> `icmp eq`
- `!=` -> `icmp ne`

It also lowers unary operators:

- unary `-` -> `sub 0, value`
- unary `!` -> `xor value, 1`

Comparison results use `bool` / `i1`.

### Function calls

Calls lower by:

1. lowering each argument to an rvalue
2. emitting `call`
3. capturing the result in a temporary when the callee return type is not `void`

Calls target names already present in the module global scope, which includes both user functions and runtime builtins.

### Arrays

Array element access lowers through `getelementptr inbounds`.

Current implementation details:

- the base expression must lower to an addressable array value
- one or more indices are lowered in the active backend path
- the generated index list starts with `0` and then appends every source index, matching LLVM access into an aggregate stored behind a pointer

The resulting address is then loaded when the access is used as an rvalue.

### Assignments

Simple assignment lowers to `store`.

Compound assignments currently supported are:

- `+=`
- `-=`

They lower as:

1. `load` current value
2. compute `add` or `sub`
3. `store` the result back

### Control flow

`if`, `while`, and `for` create explicit basic blocks and branch instructions.

Current patterns are:

- `if`: `then`, `else`, and `end` blocks
- `while`: `cond`, `body`, and `end` blocks
- `for`: optional init in a fresh scope, then `cond`, `body`, `update`, and `end` blocks

Loop lowering also maintains explicit loop targets so:

- `break` branches to the current loop end block
- `continue` branches to the current `while` condition block or current `for` update block

A block that does not already end in `ret`, `br`, or conditional `br` receives an explicit jump to the next block.

### Returns

`ret expr;` lowers to `ret <type> <operand>`.

`ret;` lowers to `ret void` in `void` functions.

If a function body falls through without a terminator, the builder emits a default return:

- `0` for `int` and `char`
- `false` for `bool`
- `0.0` for `float`
- `null`-like operand for pointer-shaped values

## LLVM Printer Behavior

`ir_printer.c` walks the module in this order:

1. globals
2. external declarations for function symbols without a body
3. function definitions

Important printing rules:

- normal globals are emitted as `@name = global ...`
- compiler-generated string storage globals are emitted as `private unnamed_addr constant`
- string globals initialized from source literals become a `getelementptr` into their storage global
- empty functions still print a synthetic `bb0` with a default return
- external runtime functions are emitted as `declare ...`

## Example

Source:

```ps
func sum(a::int, b::int) -> int {
    ret a + b;
}
```

Typical emitted LLVM IR:

```llvm
define i32 @sum(i32 %p1, i32 %p2) {
bb1:
  %t1 = alloca i32
  store i32 %p1, i32* %t1
  %t2 = alloca i32
  store i32 %p2, i32* %t2
  %t3 = load i32, i32* %t1
  %t4 = load i32, i32* %t2
  %t5 = add i32 %t3, %t4
  ret i32 %t5
}
```

This example shows the current backend style clearly:

- parameters arrive as `%pN`
- named values are spilled into stack slots first
- expression results use numbered temporaries `%tN`
- the function returns textual LLVM IR, not machine code directly

## Current Coverage

The current IR backend covers:

- global variables with optional literal initializers
- generated storage for string literals
- function definitions and function calls
- local variable declarations
- scalar loads and stores
- array literal initialization
- single-index array access
- arithmetic `+`, `-`, `*`, `/`
- comparisons `>`, `<`, `<=`
- assignments `=`, `+=`, `-=`
- `if`, `while`, and `for`
- `ret`
- builtin runtime calls to `printString` and `printInt`

## Current Limitations

The code in `ir/` intentionally remains small, and some enum variants are reserved for future work.

Notable limitations in the current implementation are:

- no lowering for all instruction kinds declared in `IRInstructionKind`
- no printed support for `IR_INSTR_CONST`, `IR_INSTR_GLOBAL_ADDR`, `IR_INSTR_NEG`, `IR_INSTR_NOT`, `IR_INSTR_CAST`, and several comparison variants
- array access lowering currently accepts exactly one index
- no SSA phi nodes
- locals are not promoted; they always go through memory slots
- the builder depends on semantic analysis to reject unsupported or inconsistent programs before IR generation

## Tests

`tests/ir/test_ir.c` covers the current backend contract with checks for:

- empty module creation
- symbol predeclaration for globals and functions
- string literal global emission
- arithmetic lowering
- compound assignment lowering
- conditional and loop branch emission
- array access through `getelementptr`
- function calls
- builtin runtime declarations
- global variable loads and stores

Those tests are the fastest way to confirm which pieces of the IR pipeline are already expected to work.
