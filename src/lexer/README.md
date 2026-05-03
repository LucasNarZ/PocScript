# Lexer

The `src/lexer/` directory contains PocScript's lexical analyzer. It transforms the input text into a linked list of tokens consumed by the parser.

## Files

- `include/token.h`: defines `TokenType`, the `Token` struct, and `TokenDefinition`
- `include/lexer.h`: public lexer interface
- `src/lexer/lexer.c`: implementation of tokenization, file reading, token serialization, and memory cleanup

## General Behavior

The lexer works on a `Lexer` struct with:

- `input`: original buffer
- `cursor`: current read position
- `tokens`: head of the generated linked list
- `line` and `column`: current position for diagnostics

The main flow is:

1. `lexerInit` initializes the state and cursor.
2. `lexerScan` walks through the buffer and tries to match each segment against the regex definitions in `types`.
3. When it finds a relevant token, it creates a `Token` with type, value, and position.
4. Whitespace and comments are recognized so the cursor can advance correctly, but they are not included in the final token list.
5. At the end, a single `TOKEN_EOF` is appended.

## Token Format

Each token contains:

- `type`: lexical category
- `value`: original matched text
- `line`: line where the token starts
- `column`: column where the token starts
- `next` and `anterior`: navigation through the linked list

## Token Types

### Literals and identifiers

- `TOKEN_NUMBER`: integer and floating-point numbers
- `TOKEN_STRING`: double-quoted strings
- `TOKEN_CHAR_LITERAL`: single-quoted character literals such as `'a'` and `'\0'`
- `TOKEN_BOOL`: `true` and `false`
- `TOKEN_IDENTIFIER`: variable names, function names, and custom types

### Comments and spacing

- `TOKEN_COMMENT`: line comments starting with `//`
- `TOKEN_WHITESPACE`: spaces, tabs, and line breaks

These two types help the scanner advance correctly, but they do not appear in the final token output.

### Language types

- `TOKEN_TYPE_ARRAY`: `Array`
- `TOKEN_TYPE_INT`: `int`
- `TOKEN_TYPE_FLOAT`: `float`
- `TOKEN_TYPE_CHAR`: `char`
- `TOKEN_TYPE_BOOL`: `bool`
- `TOKEN_TYPE_VOID`: `void`

### Keywords

- `TOKEN_KW_IF`: `if`
- `TOKEN_KW_ELSE`: `else`
- `TOKEN_KW_FOR`: `for`
- `TOKEN_KW_WHILE`: `while`
- `TOKEN_KW_FUNC`: `func`
- `TOKEN_KW_EXTERN`: `extern`
- `TOKEN_KW_RET`: `ret`
- `TOKEN_KW_BREAK`: `break`
- `TOKEN_KW_CONTINUE`: `continue`

### Operators

- comparison: `TOKEN_EQ_EQ`, `TOKEN_GT_EQ`, `TOKEN_LT_EQ`, `TOKEN_NOT_EQ`, `TOKEN_GT`, `TOKEN_LT`
- logical: `TOKEN_AND_AND`, `TOKEN_OR_OR`, `TOKEN_BANG`
- pointer/address: `TOKEN_AMPERSAND`, `TOKEN_STAR`
- assignment: `TOKEN_ASSIGN`, `TOKEN_PLUS_ASSIGN`, `TOKEN_MINUS_ASSIGN`
- function return arrow: `TOKEN_ARROW`
- arithmetic: `TOKEN_PLUS`, `TOKEN_MINUS`, `TOKEN_SLASH`

### Delimiters and separators

- `TOKEN_DOUBLE_COLON`: `::`
- `TOKEN_ARROW`: `->`
- `TOKEN_LPAREN` and `TOKEN_RPAREN`: `(` and `)`
- `TOKEN_LBRACE` and `TOKEN_RBRACE`: `{` and `}`
- `TOKEN_LBRACKET` and `TOKEN_RBRACKET`: `[` and `]`
- `TOKEN_SEMICOLON`: `;`
- `TOKEN_COMMA`: `,`
- `TOKEN_EOF`: end of file

## Most Used API

- `tokenizeString(const char *input)`: tokenizes an in-memory string
- `tokenizeFile(const char *path)`: reads a file and tokenizes its contents
- `tokensToString(Token *head)`: generates a textual token representation
- `tokenizeFileToString(const char *path)`: helper used by fixture-based tests
- `freeTokens(Token *head)`: frees the linked list

## Notes

- The order of regex definitions in `types` matters: more specific tokens must appear before more generic ones.
- The lexer records line and column at the start of each token, which feeds parser error messages.
- The lexer output is intentionally simple: it only classifies text and does not perform semantic validation.
