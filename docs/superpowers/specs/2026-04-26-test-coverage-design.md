# PocScript Test Coverage Design

## Context

PocScript already has a broad test suite for lexer, parser, semantic analysis, IR building, IR printing, and a small integration layer. The current gap is not the absence of tests in general, but the lack of protection around some high-value language contracts and backend branches.

The current suite is strongest around:

- parser shape and syntax diagnostics
- core semantic validation for declarations, returns, calls, and simple array indexing
- basic IR lowering for locals, globals, calls, control flow, and single-index arrays
- integration tests that stop at AST serialization

The main weak areas observed from the existing tests and coverage scan are:

- semantic operator validation for logical, equality, and unary-not cases
- semantic validation for incompatible array literal element types and nested array frontend behavior
- IR builder and printer coverage for float, bool, string escaping, default/zero-like emission, and file output
- integration coverage that proves the full supported pipeline beyond parser output

The goal is not to maximize numeric coverage. The goal is to add the tests that most reduce regression risk in the language contracts that matter.

## Scope

This work will add tests for two categories.

### Fully supported end-to-end behavior

These cases may receive integration coverage when the full pipeline is already documented as supported:

- globals and local variables
- function calls
- simple control flow already lowered in the backend
- single-index array access
- builtin runtime calls `printString` and `printInt`
- backend emission for `int`, `float`, `bool`, strings, and supported arithmetic/comparison operations

### Frontend-only behavior

These cases should be tested only at the stage where the contract already exists, not in end-to-end integration:

- parser handling of frontend-only syntax
- semantic validation of frontend-only operators and constructs
- IR unit tests only where there is already a meaningful partial contract that can be asserted without claiming full end-to-end support

This means there will be no integration tests for language behavior explicitly documented as frontend-only.

## Non-Goals

- Chasing every uncovered defensive branch or allocation-failure path
- Forcing integration coverage for backend features that are not yet fully supported
- Refactoring compiler code only to make coverage numbers look better
- Adding broad snapshot-style tests that are hard to diagnose when they fail

## Options Considered

### Option 1: Coverage-first by file percentage

Add tests to the least-covered files until the percentages rise.

Pros:

- easy to target with gcov output
- fast way to improve headline numbers

Cons:

- tends to overvalue low-level defensive branches
- can miss important language regressions if they live in already-covered files

### Option 2: Mostly integration tests

Favor end-to-end tests that exercise more of the pipeline at once.

Pros:

- validates realistic compiler flows
- catches wiring problems between stages

Cons:

- poor fit for frontend-only features
- failures are less local and harder to diagnose
- can hide which stage regressed

### Option 3: Contract-first by stage

Add tests where each language or backend contract is best expressed, and reserve integration tests for features already supported end to end.

Pros:

- protects the most meaningful behavior
- respects the documented frontend/backend boundary
- keeps failures local and actionable

Cons:

- requires more judgment than simply following percentages

## Decision

Use Option 3.

Tests will be selected based on regression value, not raw coverage expansion. A branch is worth testing when it protects a documented language rule, a backend emission contract, or a cross-stage flow already declared as supported.

## Test Design

### Semantic tests

Add focused tests for contracts that are currently underrepresented but central to language correctness.

Primary additions:

- logical `&&` accepts bool operands
- logical `&&` rejects non-bool operands
- logical `||` accepts bool operands
- logical `||` rejects non-bool operands
- equality `==` accepts compatible operands
- equality `==` rejects incompatible operands
- inequality `!=` accepts compatible operands
- inequality `!=` rejects incompatible operands
- unary `!` accepts bool operand
- unary `!` rejects non-bool operand
- array literal rejects incompatible element types
- nested array literals keep frontend semantic behavior stable where already supported by the semantic layer

These tests should assert both acceptance/rejection and the specific semantic diagnostics where the code already emits stable messages.

### IR unit tests

Add unit tests for backend contracts that are already meaningful even without full executable integration.

Primary additions:

- float global or local emission prints `double`-based LLVM IR correctly
- bool global or local emission prints `i1` values correctly
- string literal emission escapes special characters correctly
- supported arithmetic variants `sub`, `mul`, and `sdiv` are emitted when used in valid programs
- supported comparisons already documented in the backend remain emitted correctly
- default return / zero-like return emission for empty functions is stable
- `irPrintModuleToFile(...)` writes the printed module successfully

These tests should stay narrow and assert textual IR substrings or file output rather than broad full-file snapshots unless a fixture adds real clarity.

### Integration tests

Add only full-flow tests for features documented as end-to-end supported.

Primary additions:

- valid program with globals, a user function call, and simple control flow reaches semantic analysis and emits IR successfully
- valid program with one-dimensional array declaration and indexed read emits expected IR fragments
- valid program that uses `printString` and `printInt` emits the expected runtime declarations and calls

Integration tests should stop at successful semantic analysis and stable IR text assertions. They should not attempt to validate frontend-only constructs as if they were backend complete.

### Lexer tests

Only add lexer tests where they protect real downstream contracts that are currently weak.

Primary additions:

- combined tokenization of `==`, `!=`, `&&`, and `||`
- tokenization of `break` and `continue` when used together in a small snippet
- tokenization of strings or types that support the new semantic and IR tests, if not already directly covered

Lexer tests should remain small and structural.

## TDD Execution Strategy

Implementation will follow strict TDD for each new behavior:

1. add one failing test for a single contract
2. run the smallest relevant test target and confirm the failure is the expected one
3. add or adjust minimal production code only if the test reveals a real bug or missing behavior
4. rerun the focused test until it passes
5. move to the next contract

If a test reveals that the behavior already works and only coverage was missing, no production code change is needed. In that case the TDD cycle still holds: the new test must first fail or otherwise prove it is checking a missing contract before it is accepted.

## Verification Strategy

Before completion, verification will include:

- targeted test runs while developing each new case
- the full `make test` suite after all additions
- the repository's required completion checks from local instructions: `npm run lint && npm run typecheck` only if they are actually applicable to this repository; otherwise report that the project is C-based and use its real verification commands

For this repository, the final meaningful verification commands are expected to be the C test suite and any additional compiler build command needed to confirm the tests integrate cleanly with the existing make targets.

## Expected Outcome

The end state should be a test suite that better protects:

- semantic rules for logical, equality, unary-not, and array literal behavior
- backend IR emission for supported scalar types and operators
- full-pipeline success for a few more supported language flows

The result should be better confidence in the compiler's real contracts, not just a higher aggregate percentage.
