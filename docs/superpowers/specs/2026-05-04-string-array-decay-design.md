# String As Array With Controlled Array-to-Pointer Decay

## Context

Today PocScript treats string literals as `*char` during semantic analysis and lowers them in the IR as a pointer to compiler-generated global storage.

That works for pointer-based APIs such as `printString`, but it means strings are not modeled as real arrays in the language type system. The project already has explicit support for array literals, array declarations, array initialization, and array indexing, so treating string literals as arrays is a better fit for the rest of the language.

The target behavior is to make string literals real arrays while preserving pointer-style use where appropriate, similar to C.

## Goals

- Make string literals resolve to array types rather than `*char`
- Preserve valid code such as `value::*char = "hello";`
- Allow string literals to initialize `Array<char>` variables naturally
- Introduce controlled `Array<T>[N] -> *T` decay for all array types
- Limit decay to specific contexts to keep semantics and lowering simple

## Non-Goals

- No universal implicit decay in every expression
- No new first-class `string` type name
- No change to parser tokenization rules for string literals
- No attempt to emulate all C array/pointer rules at once

## Approved Behavior

### String literal type

A string literal such as `"aaaaa"` has semantic type `Array<char>[6]`.

The type size includes the trailing `\0` terminator.

Examples:

```ps
buffer::Array<char> = "aaaaa";
ptr::*char = "aaaaa";
```

Both remain valid.

### Array-to-pointer decay

Any `Array<T>[N]` may decay implicitly to `*T`, but only in these contexts:

- variable initialization
- assignment
- function argument passing

Decay does not happen automatically in every rvalue expression.

### Indexing

Indexing should work for both arrays and pointers.

Examples:

```ps
arr[0];
ptr[0];
```

This keeps pointer-style traversal usable after introducing array-backed strings.

## Design

### 1. AST and parser

No parser rewrite is required.

- Keep `AST_STRING_LITERAL`
- Keep the lexer and parser behavior unchanged
- Do not rewrite string literals into `AST_ARRAY_LITERAL`

This preserves source intent, avoids bloating the AST with per-character nodes, and keeps current diagnostics and pretty-printing straightforward.

### 2. Semantic model

#### String literals

`checkExpression(...)` for `AST_STRING_LITERAL` should return:

- `SEM_TYPE_ARRAY`
- `element_type = SEM_TYPE_CHAR`
- `has_array_size = true`
- `array_size = decoded_string_length + 1`

The decoded length must reflect escape processing in the same way the backend already unquotes string contents.

#### Compatibility

The semantic layer needs an explicit compatibility rule:

- exact structural compatibility remains valid
- `char <- int` remains valid where it already is
- `Array<T>[N]` is compatible with `*T` only in decay-enabled contexts

This rule should not be implemented as a blanket change inside every type comparison call site. The safer design is to distinguish:

- strict compatibility
- compatibility with decay allowed

The following validation paths should allow decay:

- variable initializer compatibility
- assignment compatibility
- function argument compatibility

Other paths should stay strict unless intentionally expanded later.

#### Indexing

`checkArrayAccess(...)` should accept both:

- `SEM_TYPE_ARRAY`
- `SEM_TYPE_POINTER`

For either case, the resulting expression type is the element type.

Error text should be updated from "cannot index non-array value" to something that reflects the broader accepted set, for example "cannot index non-array/non-pointer value of type ..." or equivalent wording.

### 3. IR lowering strategy

The current backend already has efficient string storage emission:

- compiler-generated constant globals for string contents
- `getelementptr` to obtain `i8*` for pointer-style use

That path should be preserved.

The main change is that the IR builder must distinguish between:

- array-valued storage
- pointer-valued decay result

#### String literals in IR

String literals should continue to allocate compiler-generated storage globals such as `[N x i8]` with a trailing zero byte.

When a string literal is used in a decay-enabled context, lowering should produce a pointer to the first element via `getelementptr ..., 0, 0`.

When a string literal is used to initialize an array destination, lowering should treat it as array data, not as an already-decayed pointer.

That means the builder needs a path that can materialize string contents element-wise for array initialization, while preserving the existing pointer lowering path for `*char` consumers.

#### General array decay

For non-string arrays, decay should lower to a pointer to the first element.

Examples:

- local `[N x T]` stack slot -> `getelementptr 0, 0` -> `T*`
- global `[N x T]` storage -> `getelementptr 0, 0` -> `T*`

This likely belongs in a dedicated helper rather than being duplicated across assignment, initialization, and call lowering.

#### Array initialization from string literal

For code such as:

```ps
buffer::Array<char> = "aaaaa";
```

the backend should initialize the destination array as if the source were the character sequence `{ 'a', 'a', 'a', 'a', 'a', '\0' }`.

This does not require rewriting the AST node. It only requires the IR builder to recognize that an array destination is being initialized from a string literal and emit per-element stores accordingly.

### 4. Implementation shape

The expected file impact is:

- `src/semantic/semantic.c`
  - string literal typing
  - decay-aware compatibility checks
  - pointer indexing support
- `src/semantic/types.c`
  - helpers for array/pointer classification and decay compatibility
- `include/types.h`
  - helper declarations if needed
- `src/ir/ir_builder_expr.c`
  - decay lowering helpers
  - pointer-aware indexing lowering
  - updated string literal lowering behavior
- `src/ir/ir_builder_stmt.c`
  - array initialization from string literal
  - assignment/initializer paths that require decay-aware lowering
- tests in `tests/semantic/` and `tests/ir/`

## Recommended helper boundaries

Keep the change minimal and localized. The most useful new helpers are likely:

- semantic helper to test whether an array can decay to a given pointer type
- semantic helper to choose strict vs decay-aware compatibility
- IR helper to compute pointer-to-first-element from an addressable array
- IR helper to store string literal contents into an array destination

Avoid introducing a new AST node or a new IR instruction unless the existing IR builder becomes too contorted without one.

## Edge Cases

The implementation should define behavior for these cases:

1. Unsized destination array initialized from string literal

```ps
buffer::Array<char> = "hi";
```

Expected: infer size `3`.

2. Sized destination array smaller than string length plus terminator

```ps
buffer::Array<char>[2] = "hi";
```

Expected: semantic error due to incompatible sizes.

3. Sized destination array larger than string length plus terminator

```ps
buffer::Array<char>[8] = "hi";
```

Expected: reject for strict size mismatch unless the language intentionally adopts zero-fill semantics later. For now, keep current strictness.

4. Function argument decay

```ps
extern func printString(value::*char) -> void;
func main() -> void { printString("hi"); ret; }
```

Expected: valid through decay.

5. General array decay

```ps
arr::Array<int>[3] = {1, 2, 3};
ptr::*int = arr;
```

Expected: valid through decay in assignment.

## Testing

Add or update tests for:

- string literal semantic type behaves as sized char array
- `*char` initialized from string literal still works
- `Array<char>` initialized from string literal works
- function arguments accept array-to-pointer decay
- assignments accept array-to-pointer decay
- pointer indexing is accepted semantically
- IR still emits pointer-based string globals for pointer consumers
- IR initializes arrays correctly from string literals
- size inference for unsized char arrays from string literals
- rejection of incompatible sized char arrays initialized from string literals

## Risks

### Semantic ambiguity

If decay is added too deep in shared compatibility helpers, it may silently affect contexts that were meant to stay strict. The implementation should keep decay opt-in at the call site.

### IR value model

The current IR builder is more storage-oriented for arrays than value-oriented. The most likely complexity is not semantic typing, but deciding when to lower an array as an addressable object versus when to lower it as a decayed pointer.

### String length accounting

The semantic size of string literals must match the backend's decoded storage length exactly, including escape sequences and the trailing zero byte.

## Recommendation

Implement this as a semantic and IR enhancement, not as a parser rewrite.

The minimal sound path is:

1. keep `AST_STRING_LITERAL`
2. type string literals as `Array<char>[N+1]`
3. allow controlled `Array<T>[N] -> *T` decay only in initialization, assignment, and call arguments
4. allow indexing on both arrays and pointers
5. preserve current string storage globals and use decay lowering to obtain pointers when needed
