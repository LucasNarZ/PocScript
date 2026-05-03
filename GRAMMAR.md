# PocScript Grammar

This document describes the grammar currently accepted by the parser in `src/parser/parser.c`.
It documents the language as it exists in the implementation today, not as it ideally should be.

## Overview

- The input is a `program` with zero or more top-level declarations until `EOF`.
- Top level accepts variable declarations, function definitions, and external function declarations written as `extern func ...;`.
- `if`, `while`, `for`, and `func` always require a block with `{}`.
- Regular statements end with `;`.
- Declarations use `::`.
- `ret` may appear either as `ret;` or `ret <expr>;`.
- `break` and `continue` are parsed as simple statements and require `;`.
- The parser supports array literals, indexed access, function calls, and types with `Array<...>` and `[]` suffixes.
- Function declarations require an explicit return type after `->`.
- `void` is valid only as a function return type.

## Grammar

```ebnf
program         = { top-level-declaration } EOF ;

top-level-declaration = function-declaration
                      | extern-function-declaration
                      | declaration ";" ;

statement       = if-statement
                | while-statement
                | for-statement
                | simple-statement ";" ;

simple-statement = return-statement
                 | loop-control-statement
                 | assignment-or-declaration-or-expression ;

block           = "{" { statement } "}" ;

if-statement    = "if" "(" logical-expression ")" block [ "else" ( if-statement | block ) ] ;

while-statement = "while" "(" logical-expression ")" block ;

for-statement   = "for" "("
                    [ assignment-or-declaration-or-expression ] ";"
                    [ logical-expression ] ";"
                    [ assignment-or-declaration-or-expression ]
                  ")" block ;

function-declaration = "func" identifier "(" [ parameter { "," parameter } ] ")" "->" return-type block ;

extern-function-declaration = "extern" "func" identifier "(" [ parameter { "," parameter } ] ")" "->" return-type ";" ;

return-type     = "void" | type ;

parameter       = identifier "::" type ;

return-statement = "ret" [ logical-expression ] ;

loop-control-statement = "break" | "continue" ;

assignment-or-declaration-or-expression = declaration
                                        | assignment
                                        | logical-expression ;

declaration     = identifier "::" type [ "=" initializer ] ;

assignment      = assign-target assignment-operator logical-expression ;

assign-target   = identifier { index-suffix }
                | "*" unary-expression ;

assignment-operator = "=" | "+=" | "-=" ;

initializer     = logical-expression ;

logical-expression = or-expression ;

or-expression    = and-expression { "||" and-expression } ;

and-expression   = comparison-expression { "&&" comparison-expression } ;

comparison-expression = additive-expression
                        { ( ">" | "<" | ">=" | "<=" | "==" | "!=" ) additive-expression } ;

additive-expression = multiplicative-expression
                      { ( "+" | "-" ) multiplicative-expression } ;

multiplicative-expression = unary-expression
                            { ( "*" | "/" ) unary-expression } ;

unary-expression = ( "!" | "-" | "&" | "*" ) unary-expression
                 | primary-expression ;

primary-expression = number
                   | string
                   | boolean
                   | array-literal
                   | postfix-expression
                   | "(" logical-expression ")" ;

postfix-expression = identifier { call-suffix | index-suffix } ;

call-suffix     = "(" [ logical-expression { "," logical-expression } ] ")" ;

index-suffix    = "[" logical-expression "]" ;

array-literal   = "{" [ array-element { array-separator array-element } [ array-separator ] ] "}" ;

array-element   = logical-expression ;

array-separator = "," | ";" ;

type            = pointer-type | array-generic-type | named-type ;

pointer-type    = "*" type ;

array-generic-type = "Array" [ "<" type ">" ] array-type-suffixes ;

named-type      = ( "int" | "float" | "char" | "bool" | identifier ) array-type-suffixes ;

array-type-suffixes = { "[" [ logical-expression ] "]" } ;

identifier      = letter-or-underscore { letter | digit | underscore } ;
number          = integer | float ;
boolean         = "true" | "false" ;
string          = quoted-text ;
```

## Expression Precedence

From lowest to highest within expressions:

1. logical OR: `||`
2. logical AND: `&&`
3. comparison: `>`, `<`, `>=`, `<=`, `==`, `!=`
4. addition and subtraction: `+`, `-`
5. multiplication and division: `*`, `/`
6. unary prefix: `!`, unary `-`, `&`, unary `*`
7. postfix: call `()` and indexing `[]`
8. primaries: identifiers, literals, grouping, and array literal

Assignment operators are parsed outside the expression precedence chain, as part of the `assignment` production.

## Lexical Rules

Based on `src/lexer/lexer.c`:

- `identifier`: `[_a-zA-Z][_a-zA-Z0-9]*`
- `number`: integers or decimal floats such as `10` and `1.5`
- `string`: accepts double-quoted text
- `char`: accepts single-quoted character literals with escape support
- `boolean`: `true` and `false`
- `&`: address-of operator
- `//...` comments and whitespace are ignored by the lexer

The EBNF above uses the auxiliary lexical names `letter-or-underscore`, `letter`, `digit`, `underscore`, `integer`, `float`, and `quoted-text` as shorthand for the token categories produced by the lexer. In practice, the lexer produces separate token categories for double-quoted strings and single-quoted character literals.

## Important Syntactic Forms

### Variable declaration

```text
nome::int;
nome::int = 10;
lista::Array<int> = {1, 2, 3};
matriz::int[3][4];
buffer::Array<float>[n];
```

### Function

```text
func soma(a::int, b::int) -> int {
    ret a + b;
}
```

### Control flow

```text
if (condicao) {
    ret 1;
} else if (outra) {
    ret 2;
} else {
    ret 3;
}

while (ativo) {
    x += 1;
}

for(i = 0; i < 10; i += 1) {
    foo(i);
}
```

### Postfix expressions

```text
foo();
foo(1, x + 2);
arr[0];
mat[0][1];
foo(1)[0];
arr[0](x);
```

## Implementation Notes

- Unary `-` is accepted and parsed as a unary expression.
- Pointer types use recursive prefix syntax such as `*int` and `**char`.
- Unary `&` computes the address of an addressable expression.
- Unary `*` dereferences a pointer expression.
- Assignment targets now include dereference expressions such as `*p = 1;`.
- The parser accepts `Array` without a generic parameter, for example `x::Array;`.
- Types can receive repeated `[]` suffixes, with an optional size per dimension, for example `int[][10]`.
- The size inside type `[]` uses the same `logical-expression` rule.
- Array literals accept both `,` and `;` as separators, including mixed usage.
- The parser allows a trailing separator in array literals, for example `{1, 2,}` and `{1; 2;}`.
- `initializer`, `array-element`, and the right-hand side of `assignment` are documented through `logical-expression`, which already includes array literals via `primary-expression`.
- File scope accepts only variable declarations and function declarations.
- Global variable initializers are restricted semantically to direct literal nodes.
- Consecutive indexing is documented as repeated `index-suffix` occurrences in `postfix-expression`, for example `arr[0][1]`.
- The left-hand side of an assignment is restricted syntactically to `assign-target`, which means an identifier followed by zero or more index operations.
- Function declarations require an explicit return type after `->`.
- `void` is valid only as a function return type.
- `ret;` is accepted syntactically.
- `ret;` is valid only in `void` functions.
- `if`, `while`, `for`, and `func` do not accept a single statement without braces.
- `break` and `continue` are accepted syntactically anywhere a simple statement is valid.
- `break` and `continue` are valid semantically only inside `while` and `for`.
- External function declarations are valid only at file scope and are represented in the AST as `AST_FUNC_DECL` nodes with `is_extern = true`.

## Source

The rules above were derived mainly from:

- `src/parser/parser.c`
- `tests/parser/test_parser.c`
- `src/lexer/lexer.c`
