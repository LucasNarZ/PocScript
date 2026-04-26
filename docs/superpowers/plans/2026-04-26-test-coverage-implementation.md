# PocScript Test Coverage Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the highest-value missing tests for semantic analysis, IR generation/printing, lexer contracts, and supported end-to-end compiler flows.

**Architecture:** Extend the existing C test suite in place, keeping tests localized by compiler phase. Add semantic and IR unit tests for language contracts that are currently weak, and add a small number of integration tests only for behaviors already documented as end-to-end supported.

**Tech Stack:** C, gcc, make, existing custom test runner, gcov for spot-checking coverage

---

### Task 1: Add semantic operator and array contract tests

**Files:**
- Modify: `tests/semantic/test_semantic.c`
- Modify: `tests/test_main.c`

- [ ] **Step 1: Write the failing tests**

Add tests for:

```c
void test_semantic_accepts_logical_and_with_bool_operands(void);
void test_semantic_rejects_logical_and_with_non_bool_operand(void);
void test_semantic_accepts_logical_or_with_bool_operands(void);
void test_semantic_rejects_logical_or_with_non_bool_operand(void);
void test_semantic_accepts_equality_with_compatible_operands(void);
void test_semantic_rejects_equality_with_incompatible_operands(void);
void test_semantic_accepts_inequality_with_compatible_operands(void);
void test_semantic_rejects_inequality_with_incompatible_operands(void);
void test_semantic_accepts_unary_not_for_bool_operand(void);
void test_semantic_rejects_unary_not_for_non_bool_operand(void);
void test_semantic_rejects_array_literal_with_incompatible_element_types(void);
void test_semantic_accepts_nested_array_literal_with_matching_shapes(void);
```

- [ ] **Step 2: Run focused tests to verify failure**

Run: `make test`
Expected: the new semantic tests fail because they are not registered or because the current behavior is uncovered and possibly incorrect.

- [ ] **Step 3: Implement minimal production fix only if a real semantic bug is exposed**

If needed, inspect and minimally adjust:

```c
/* semantic/semantic.c */
case AST_BINARY_EQ:
case AST_BINARY_NOT_EQ:
case AST_BINARY_AND:
case AST_BINARY_OR:
/* ... */

case AST_UNARY_NOT:
/* ... */
```

No production change if the behavior already works and only tests were missing.

- [ ] **Step 4: Run focused tests to verify pass**

Run: `make test`
Expected: the new semantic tests pass.

### Task 2: Add IR printer and backend contract tests

**Files:**
- Modify: `tests/ir/test_ir.c`
- Modify: `tests/test_main.c`

- [ ] **Step 1: Write the failing tests**

Add tests for:

```c
void test_ir_printer_emits_float_values_as_double(void);
void test_ir_printer_emits_bool_values_as_i1(void);
void test_ir_printer_escapes_special_characters_in_string_literals(void);
void test_ir_printer_emits_sub_mul_and_div_operations(void);
void test_ir_printer_emits_default_return_for_empty_function(void);
void test_ir_printer_writes_module_to_file(void);
```

- [ ] **Step 2: Run focused tests to verify failure**

Run: `make test`
Expected: the new IR tests fail before implementation or registration is complete.

- [ ] **Step 3: Implement minimal production fix only if a real IR bug is exposed**

If needed, minimally adjust:

```c
/* ir/ir_builder.c */
/* type/literal lowering or default-return lowering paths */

/* ir/ir_printer.c */
/* scalar printing, string escaping, file output paths */
```

Do not broaden backend support beyond what the tests require.

- [ ] **Step 4: Run focused tests to verify pass**

Run: `make test`
Expected: the new IR tests pass.

### Task 3: Add integration tests for supported full-pipeline flows

**Files:**
- Modify: `tests/helpers/test_support.h`
- Modify: `tests/helpers/test_support.c`
- Modify: `tests/integration/test_integration.c`
- Modify: `tests/test_main.c`

- [ ] **Step 1: Write the failing helper and integration tests**

Add a helper if needed for semantic-plus-IR integration, for example:

```c
char *emitLlvmIrFromValidProgram(const char *input);
```

Add integration tests for:

```c
void test_integration_emits_ir_for_globals_function_calls_and_control_flow(void);
void test_integration_emits_ir_for_single_dimension_array_access(void);
void test_integration_emits_ir_for_runtime_print_calls(void);
```

- [ ] **Step 2: Run focused tests to verify failure**

Run: `make test`
Expected: the new integration tests fail before helper wiring or assertions are in place.

- [ ] **Step 3: Implement minimal helper or production fix if required**

If the existing helpers are not enough, add the smallest helper in `tests/helpers/test_support.c` that keeps test setup concise. Avoid changing compiler code unless the integration tests expose a real bug.

- [ ] **Step 4: Run focused tests to verify pass**

Run: `make test`
Expected: the new integration tests pass with stable IR substring assertions.

### Task 4: Add lexer tests for downstream operator contracts

**Files:**
- Modify: `tests/lexer/test_lexer.c`
- Modify: `tests/test_main.c`

- [ ] **Step 1: Write the failing tests**

Add tests for:

```c
void test_lexer_tokenizes_logical_and_equality_operators(void);
void test_lexer_tokenizes_break_and_continue_keywords_together(void);
```

- [ ] **Step 2: Run focused tests to verify failure**

Run: `make test`
Expected: the new lexer tests fail before registration or due to uncovered token contracts.

- [ ] **Step 3: Implement minimal production fix only if a real lexer bug is exposed**

If needed, minimally adjust tokenization in:

```c
/* lexer/lexer.c */
```

- [ ] **Step 4: Run focused tests to verify pass**

Run: `make test`
Expected: the new lexer tests pass.

### Task 5: Full verification

**Files:**
- Modify: none

- [ ] **Step 1: Run the full suite**

Run: `make test`
Expected: all tests pass.

- [ ] **Step 2: Spot-check build compatibility**

Run: `make ast`
Expected: the compiler still builds successfully.

- [ ] **Step 3: Report actual coverage impact qualitatively**

Run if needed: `make test CFLAGS="-Wall -g --coverage" && gcov lexer/lexer.c semantic/semantic.c ir/ir_builder.c ir/ir_printer.c`
Expected: improved exercised paths in the targeted modules, especially semantic operators and IR printer paths.
