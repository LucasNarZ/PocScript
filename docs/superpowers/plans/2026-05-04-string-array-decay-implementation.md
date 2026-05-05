# String Array Decay Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make string literals behave as sized `Array<char>` values while preserving controlled `Array<T>[N] -> *T` decay in initialization, assignment, and function-call arguments.

**Architecture:** Keep `AST_STRING_LITERAL` unchanged, move the new behavior into semantic typing and compatibility checks, then teach the IR builder how to lower both decayed arrays and string-to-array initialization. Reuse the existing string-storage globals and add small helpers instead of introducing new AST or IR node kinds.

**Tech Stack:** C, GCC, Make, project test runner, PocScript semantic analyzer, custom IR builder/printer.

---

## File Map

- Modify: `include/types.h`
  - declare small semantic type helpers for array/pointer inspection and decay-aware compatibility
- Modify: `src/semantic/types.c`
  - implement the helpers used by semantic validation
- Modify: `src/semantic/semantic.c`
  - type string literals as sized char arrays
  - allow decay only in initializer/assignment/call paths
  - accept pointer indexing
- Modify: `src/ir/ir_builder_internal.h`
  - declare array-decay and string-materialization helpers for the IR builder
- Modify: `src/ir/ir_builder_utils.c`
  - add helper logic to compute decoded string length and first-element pointers for arrays
- Modify: `src/ir/ir_builder_expr.c`
  - lower decayed arrays and pointer indexing
- Modify: `src/ir/ir_builder_stmt.c`
  - initialize char arrays from string literals
- Modify: `tests/semantic/test_semantic.c`
  - add focused semantic coverage for string-as-array and controlled decay
- Modify: `tests/ir/test_ir.c`
  - add focused IR coverage for decayed arguments/assignments and array initialization from string literals
- Modify: `tests/test_main.c`
  - register the new tests

### Task 1: Add Semantic Type Helpers And First Failing Tests

**Files:**
- Modify: `include/types.h`
- Modify: `src/semantic/types.c`
- Modify: `tests/semantic/test_semantic.c`
- Modify: `tests/test_main.c`

- [ ] **Step 1: Add semantic tests that describe the new contract**

Append these tests near the existing string and array semantic coverage in `tests/semantic/test_semantic.c`:

```c
void test_semantic_accepts_char_array_initialized_from_string_literal(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { text::Array<char> = \"hi\"; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 0);
    semanticResultFree(&result);
}

void test_semantic_accepts_general_array_to_pointer_decay_in_assignment(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { values::Array<int>[3] = {1, 2, 3}; ptr::*int = values; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 0);
    semanticResultFree(&result);
}

void test_semantic_accepts_general_array_to_pointer_decay_in_call_arguments(void) {
    SemanticResult result = analyzeRootFromString(
        "extern func first(value::*int) -> int; "
        "func main() -> int { values::Array<int>[3] = {1, 2, 3}; ret first(values); }"
    );

    EXPECT_TRUE(result.errors.count == 0);
    semanticResultFree(&result);
}
```

Register them in `tests/test_main.c` by adding declarations near the top and `RUN_TEST(...)` calls beside the existing semantic array/string tests.

- [ ] **Step 2: Run the semantic-focused suite and confirm failure**

Run:

```bash
make test
```

Expected: the new semantic tests fail because string literals still resolve to `*char` and generic array decay is not accepted.

- [ ] **Step 3: Add small helper declarations in `include/types.h`**

Add these declarations below the existing `semanticTypeName(...)` declaration:

```c
bool semanticTypeIsArray(const SemanticType *type);
bool semanticTypeIsPointer(const SemanticType *type);
bool semanticTypeCanDecayToPointer(const SemanticType *array_type, const SemanticType *pointer_type);
bool semanticTypeIsCompatibleWithDecay(const SemanticType *expected, const SemanticType *actual);
```

- [ ] **Step 4: Implement the helpers in `src/semantic/types.c`**

Add a minimal block near the existing equality helpers:

```c
bool semanticTypeIsArray(const SemanticType *type) {
    return type != NULL && type->kind == SEM_TYPE_ARRAY;
}

bool semanticTypeIsPointer(const SemanticType *type) {
    return type != NULL && type->kind == SEM_TYPE_POINTER;
}

bool semanticTypeCanDecayToPointer(const SemanticType *array_type, const SemanticType *pointer_type) {
    return semanticTypeIsArray(array_type)
        && semanticTypeIsPointer(pointer_type)
        && array_type->element_type != NULL
        && pointer_type->element_type != NULL
        && semanticTypeEquals(array_type->element_type, pointer_type->element_type);
}

bool semanticTypeIsCompatibleWithDecay(const SemanticType *expected, const SemanticType *actual) {
    return semanticTypeEquals(expected, actual)
        || (expected->kind == SEM_TYPE_CHAR && actual->kind == SEM_TYPE_INT)
        || semanticTypeCanDecayToPointer(actual, expected);
}
```

Keep this helper small. It should answer only the compatibility question for decay-enabled call sites.

- [ ] **Step 5: Re-run the suite to confirm tests still fail for the intended next reason**

Run:

```bash
make test
```

Expected: the new tests still fail, but now the next missing piece is in `src/semantic/semantic.c`, not the helper layer.

### Task 2: Type String Literals As Arrays And Allow Controlled Decay

**Files:**
- Modify: `src/semantic/semantic.c`
- Modify: `tests/semantic/test_semantic.c`
- Modify: `tests/test_main.c`

- [ ] **Step 1: Add a failing test for sized array mismatch from string literals**

Append this test to `tests/semantic/test_semantic.c`:

```c
void test_semantic_rejects_too_small_char_array_initialized_from_string_literal(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { text::Array<char>[2] = \"hi\"; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    semanticResultFree(&result);
}
```

Register it in `tests/test_main.c` beside the other new semantic tests.

- [ ] **Step 2: Run the full suite to verify failure**

Run:

```bash
make test
```

Expected: the newly added mismatch test and the earlier decay tests fail.

- [ ] **Step 3: Change string literal typing in `src/semantic/semantic.c`**

Replace the `AST_STRING_LITERAL` branch inside `checkExpression(...)` with logic shaped like this:

```c
case AST_STRING_LITERAL: {
    SemanticType *array_type = semanticTypeNewPrimitive(SEM_TYPE_ARRAY);

    if (array_type == NULL) {
        return NULL;
    }

    array_type->element_type = semanticTypeNewPrimitive(SEM_TYPE_CHAR);
    if (array_type->element_type == NULL) {
        semanticTypeFree(array_type);
        return NULL;
    }

    array_type->has_array_size = true;
    array_type->array_size = semanticStringLiteralDecodedLength(node->data.string_literal.value) + 1;
    return array_type;
}
```

Add a small local helper in the same file to decode the string length using the same escape rules already used by the backend.

- [ ] **Step 4: Use decay-aware compatibility only in the approved semantic paths**

Update the compatibility checks in `validateVariableDeclaration(...)`, `checkAssign(...)`, and `checkCall(...)` to use `semanticTypeIsCompatibleWithDecay(...)` instead of the strict helper.

The call sites should end up shaped like this:

```c
if (initializerType->kind != SEM_TYPE_ERROR
        && !semanticTypeIsCompatibleWithDecay(declaredType, initializerType)) {
    appendInitializerTypeError(node, ctx->result, node->data.var_decl.name, declaredType, initializerType);
}
```

Do not change arithmetic, comparison, or return-type validation to use decay.

- [ ] **Step 5: Run the suite and confirm the semantic tests pass**

Run:

```bash
make test
```

Expected: the new semantic decay tests pass, while IR tests may still fail once they are added later.

### Task 3: Accept Pointer Indexing In Semantics And Lower Risky Diagnostics Cleanly

**Files:**
- Modify: `src/semantic/semantic.c`
- Modify: `tests/semantic/test_semantic.c`
- Modify: `tests/test_main.c`

- [ ] **Step 1: Add pointer-indexing semantic coverage**

Append these tests to `tests/semantic/test_semantic.c`:

```c
void test_semantic_accepts_pointer_indexing(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> int { text::*char = \"hi\"; ret text[1]; }"
    );

    EXPECT_TRUE(result.errors.count == 0);
    semanticResultFree(&result);
}

void test_semantic_reports_indexing_non_array_or_pointer_value(void) {
    AstNode *root = parseRootFromString("func main() -> void { flag::bool = true; flag[0]; }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "cannot index") != NULL);

    semanticResultFree(&result);
    astFree(root);
}
```

Register both in `tests/test_main.c`.

- [ ] **Step 2: Run tests to verify failure**

Run:

```bash
make test
```

Expected: pointer indexing fails in semantics because `checkArrayAccess(...)` only accepts arrays.

- [ ] **Step 3: Extend `checkArrayAccess(...)` to accept pointers**

Reshape the main branch in `src/semantic/semantic.c` like this:

```c
if (baseType->kind != SEM_TYPE_ARRAY && baseType->kind != SEM_TYPE_POINTER) {
    char message[256];

    snprintf(message, sizeof(message), "cannot index non-array/non-pointer value of type %s", semanticTypeName(baseType));
    appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, message);
    semanticTypeFree(indexType);
    semanticTypeFree(baseType);
    return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
}

{
    SemanticType *nextType = semanticTypeClone(baseType->element_type);
    semanticTypeFree(baseType);
    baseType = nextType;
}
```

Do not add pointer arithmetic here; this task is only about indexing existing pointer values.

- [ ] **Step 4: Run tests to verify semantic coverage passes**

Run:

```bash
make test
```

Expected: the new pointer-indexing semantic tests pass.

### Task 4: Lower Controlled Decay For Arrays And String Arguments In The IR

**Files:**
- Modify: `src/ir/ir_builder_internal.h`
- Modify: `src/ir/ir_builder_utils.c`
- Modify: `src/ir/ir_builder_expr.c`
- Modify: `tests/ir/test_ir.c`
- Modify: `tests/test_main.c`

- [ ] **Step 1: Add IR tests for pointer-style decay uses**

Append these tests to `tests/ir/test_ir.c`:

```c
void test_ir_printer_decays_string_literal_for_char_pointer_initializer(void) {
    char *llvm = emitLlvmIrFromString("func main() -> void { msg::*char = \"hi\"; ret; }");

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "getelementptr inbounds ([3 x i8], [3 x i8]* @") != NULL);
        free(llvm);
    }
}

void test_ir_printer_decays_array_argument_to_pointer_parameter(void) {
    char *llvm = emitLlvmIrFromString(
        "extern func first(value::*int) -> int; "
        "func main() -> int { values::Array<int>[3] = {1, 2, 3}; ret first(values); }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "call i32 @first(i32*") != NULL);
        EXPECT_TRUE(strstr(llvm, "getelementptr inbounds [3 x i32]") != NULL);
        free(llvm);
    }
}
```

Register them in `tests/test_main.c` near the existing IR string and pointer tests.

- [ ] **Step 2: Run the suite to verify these IR expectations fail first**

Run:

```bash
make test
```

Expected: the new IR tests fail because there is no generic array-to-pointer decay helper.

- [ ] **Step 3: Add array-decay helper declarations in `src/ir/ir_builder_internal.h`**

Add declarations shaped like this:

```c
IRValue irBuilderDecayArrayAddress(IRBuilder *builder, IRValue array_lvalue);
IRValue irBuilderLowerDecayedRValue(IRBuilder *builder, const AstNode *node);
```

- [ ] **Step 4: Implement the decay helpers in `src/ir/ir_builder_utils.c` and use them from expressions/calls**

Implement the core helper with a `gep 0, 0` shape:

```c
IRValue irBuilderDecayArrayAddress(IRBuilder *builder, IRValue array_lvalue) {
    IROperand *indices;

    if (!array_lvalue.has_address || array_lvalue.type == NULL || array_lvalue.type->kind != IR_TYPE_ARRAY) {
        return irValueEmpty();
    }

    indices = calloc(2, sizeof(IROperand));
    indices[0] = irOperandCreateLiteral(irTypeCreate(IR_TYPE_INT), irLiteralCreateInt(0));
    indices[1] = irOperandCreateLiteral(irTypeCreate(IR_TYPE_INT), irLiteralCreateInt(0));
    return irBuilderEmitGep(
        builder,
        array_lvalue.address,
        indices,
        2,
        irTypeCreatePointer(irTypeClone(array_lvalue.type->element_type))
    );
}
```

Then update the call/initializer lowering path in `src/ir/ir_builder_expr.c` so that when a context expects a pointer and the source node is an array lvalue or string literal, the builder lowers the decayed pointer instead of trying to pass the raw array storage type.

- [ ] **Step 5: Run tests to verify decayed IR now passes**

Run:

```bash
make test
```

Expected: the new IR decay tests pass, leaving only string-to-array initialization work if it is still missing.

### Task 5: Initialize Char Arrays From String Literals In The IR

**Files:**
- Modify: `src/ir/ir_builder_stmt.c`
- Modify: `src/ir/ir_builder_utils.c`
- Modify: `tests/ir/test_ir.c`
- Modify: `tests/test_main.c`

- [ ] **Step 1: Add IR tests for array initialization from string literals**

Append these tests to `tests/ir/test_ir.c`:

```c
void test_ir_printer_initializes_unsized_char_array_from_string_literal(void) {
    char *llvm = emitLlvmIrFromString("func main() -> void { text::Array<char> = \"hi\"; ret; }");

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "alloca [3 x i8]") != NULL);
        EXPECT_TRUE(strstr(llvm, "store i8 104") != NULL);
        EXPECT_TRUE(strstr(llvm, "store i8 105") != NULL);
        EXPECT_TRUE(strstr(llvm, "store i8 0") != NULL);
        free(llvm);
    }
}

void test_ir_printer_initializes_sized_char_array_from_string_literal(void) {
    char *llvm = emitLlvmIrFromString("func main() -> void { text::Array<char>[3] = \"hi\"; ret; }");

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "alloca [3 x i8]") != NULL);
        EXPECT_TRUE(strstr(llvm, "store i8 0") != NULL);
        free(llvm);
    }
}
```

Register them in `tests/test_main.c`.

- [ ] **Step 2: Run the suite and confirm failure**

Run:

```bash
make test
```

Expected: these tests fail because local array initialization currently handles `AST_ARRAY_LITERAL` only.

- [ ] **Step 3: Add a string-materialization helper and call it from `irBuilderLowerVarDecl(...)`**

Add a helper in `src/ir/ir_builder_utils.c` or `src/ir/ir_builder_stmt.c` shaped like this:

```c
static bool irBuilderStoreStringLiteralElements(
        IRBuilder *builder,
        const IROperand *base_address,
        const AstNode *literal,
        size_t expected_size) {
    char *decoded = irBuilderUnquoteString(literal->data.string_literal.value);
    size_t i;

    if (decoded == NULL) {
        return false;
    }

    for (i = 0; i < expected_size; i++) {
        unsigned char ch = i < strlen(decoded) ? (unsigned char) decoded[i] : 0;
        /* build gep 0, i and store i8 ch */
    }

    free(decoded);
    return true;
}
```

Then branch inside `irBuilderLowerVarDecl(...)` before the generic scalar initializer path:

```c
if (node->data.var_decl.initializer != NULL
        && value_type->kind == IR_TYPE_ARRAY
        && value_type->element_type != NULL
        && value_type->element_type->kind == IR_TYPE_CHAR
        && node->data.var_decl.initializer->type == AST_STRING_LITERAL) {
    /* materialize characters and trailing zero into the array slot */
}
```

Preserve the existing array-literal path for non-string array initializers.

- [ ] **Step 4: Run the full suite to verify all tests pass**

Run:

```bash
make test
```

Expected: full PASS.

### Task 6: Final Verification And Documentation Touch-Ups

**Files:**
- Modify: `README.md`
- Modify: `src/semantic/README.md`
- Modify: `src/ir/README.md`

- [ ] **Step 1: Update docs to reflect the new behavior**

Adjust the affected bullets and notes with content shaped like this:

```md
- string literals are modeled semantically as sized `Array<char>` values including the trailing `\0`
- arrays may decay to pointers only in initialization, assignment, and function-call argument contexts
- pointer indexing is accepted alongside array indexing
```

The most important places are:

- `README.md` language coverage and feature table
- `src/semantic/README.md` type-checking strategy and current validation coverage
- `src/ir/README.md` string storage and array lowering notes

- [ ] **Step 2: Run the canonical verification command one last time**

Run:

```bash
make test
```

Expected: `all tests passed`.

- [ ] **Step 3: Smoke-check one representative source file through the compiler**

Use `input.ps` temporarily with a small representative example such as:

```ps
extern func printString(value::*char) -> void;

func main() -> void {
    text::Array<char> = "hi";
    ptr::*char = text;
    printString(ptr);
    ret;
}
```

Then run:

```bash
make ast
./build/bin/compiler input.ps build/ir/input.ll
```

Expected: compiler exits successfully and `build/ir/input.ll` contains both `[3 x i8]` array storage and `getelementptr`-based pointer decay.

## Self-Review

- Spec coverage: all approved behaviors from `docs/superpowers/specs/2026-05-04-string-array-decay-design.md` map to tasks above: string typing, controlled decay, pointer indexing, IR decay, string-to-array initialization, tests, and docs.
- Placeholder scan: no `TODO`, `TBD`, or unspecified “handle appropriately” steps remain.
- Type consistency: the plan consistently uses `Array<T>[N] -> *T` as the only implicit decay rule and keeps `AST_STRING_LITERAL` unchanged throughout.
