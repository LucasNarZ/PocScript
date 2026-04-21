# PocScript Grammar

This document describes the grammar currently accepted by the parser in `parser/parser.c`.
It documents the language as it exists in the implementation today, not as it ideally should be.

## Overview

- The input is a `program` with zero or more `statement`s until `EOF`.
- `if`, `while`, `for`, and `func` always require a block with `{}`.
- Regular statements end with `;`.
- Declarations use `::`.
- `ret` always requires an expression.
- The parser supports array literals, indexed access, function calls, and types with `Array<...>` and `[]` suffixes.

## Grammar

```ebnf
program         = { statement } EOF ;

statement       = if-statement
                | while-statement
                | for-statement
                | function-declaration
                | simple-statement ";" ;

simple-statement = return-statement
                 | assignment-or-declaration-or-expression ;

block           = "{" { statement } "}" ;

if-statement    = "if" "(" logical-expression ")" block [ "else" ( if-statement | block ) ] ;

while-statement = "while" "(" logical-expression ")" block ;

for-statement   = "for" "("
                    [ assignment-or-declaration-or-expression ] ";"
                    [ logical-expression ] ";"
                    [ assignment-or-declaration-or-expression ]
                  ")" block ;

function-declaration = "func" identifier "(" [ parameter { "," parameter } ] ")" block ;

parameter       = identifier "::" type ;

return-statement = "ret" logical-expression ;

assignment-or-declaration-or-expression = declaration
                                        | assignment
                                        | logical-expression ;

declaration     = identifier "::" type [ "=" initializer ] ;

assignment      = assign-target assignment-operator logical-expression ;

assign-target   = identifier { index-suffix } ;

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

unary-expression = "!" unary-expression
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

type            = array-generic-type | named-type ;

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
6. negation: `!`
7. postfix: call `()` and indexing `[]`
8. primaries: identifiers, literals, grouping, and array literal

Assignment operators are parsed outside the expression precedence chain, as part of the `assignment` production.

## Lexical Rules

Based on `lexer/lexer.c`:

- `identifier`: `[_a-zA-Z][_a-zA-Z0-9]*`
- `number`: integers or decimal floats such as `10` and `1.5`
- `string`: accepts double-quoted or single-quoted text
- `boolean`: `true` and `false`
- `//...` comments and whitespace are ignored by the lexer

The EBNF above uses the auxiliary lexical names `letter-or-underscore`, `letter`, `digit`, `underscore`, `integer`, `float`, and `quoted-text` as shorthand for the token categories produced by the lexer. In practice, string literals are produced as a single lexer category whether they use single quotes or double quotes.

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
func soma(a::int, b::int) {
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

- There is no unary `-` operator. `-x` is not recognized as a unary expression.
- The parser accepts `Array` without a generic parameter, for example `x::Array;`.
- Types can receive repeated `[]` suffixes, with an optional size per dimension, for example `int[][10]`.
- The size inside type `[]` uses the same `logical-expression` rule.
- Array literals accept both `,` and `;` as separators, including mixed usage.
- The parser allows a trailing separator in array literals, for example `{1, 2,}` and `{1; 2;}`.
- `initializer`, `array-element`, and the right-hand side of `assignment` are documented through `logical-expression`, which already includes array literals via `primary-expression`.
- Consecutive indexing is documented as repeated `index-suffix` occurrences in `postfix-expression`, for example `arr[0][1]`.
- The left-hand side of an assignment is restricted syntactically to `assign-target`, which means an identifier followed by zero or more index operations.
- Function declarations do not have a syntactic return type, even though the AST has a field for it.
- `ret` without an expression is not accepted.
- `if`, `while`, `for`, and `func` do not accept a single statement without braces.

## Source

The rules above were derived mainly from:

- `parser/parser.c`
- `tests/parser/test_parser.c`
- `lexer/lexer.c`
