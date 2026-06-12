# Chapter 2: Lexical Analysis (Scanning)

## 2.1 What Is Lexical Analysis?

**Lexical analysis** (scanning) is the first phase of compilation. The scanner reads the raw source text character by character and groups them into a stream of **tokens** — the atomic units of the programming language.

```
Input:  "int x = 42 + y;"
Output: [INT, IDENT("x"), EQ, INTEGER(42), PLUS, IDENT("y"), SEMICOLON]
```

## 2.2 Tokens, Patterns, and Lexemes

| Term | Definition | Example |
|------|-----------|---------|
| **Token** | A named category of lexical unit | `INTEGER`, `IDENTIFIER`, `PLUS` |
| **Pattern** | The rule describing how a token is formed | A digit sequence for `INTEGER` |
| **Lexeme** | The actual character sequence matching a pattern | `42`, `myVar` |

### Token Structure

Each token typically carries:
- A **token type** (e.g., `TOK_INT`, `TOK_PLUS`)
- A **lexeme** (the source text)
- **Location information** (line, column) for error reporting
- An optional **literal value** (e.g., the integer `42`, the string `"hello"`)

```cpp
struct Token {
    enum Kind {
        // Keywords
        KW_IF, KW_ELSE, KW_WHILE, KW_RETURN, KW_INT, KW_FLOAT,
        // Literals
        INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL,
        // Identifiers
        IDENTIFIER,
        // Operators
        PLUS, MINUS, STAR, SLASH, ASSIGN,
        EQ, NE, LT, GT, LE, GE,
        // Delimiters
        LPAREN, RPAREN, LBRACE, RBRACE, SEMICOLON, COMMA,
        // Special
        END_OF_FILE, UNKNOWN
    };

    Kind kind;
    std::string lexeme;
    int line, column;

    // For literals
    union {
        int64_t intValue;
        double floatValue;
    };
};
```

## 2.3 Regular Expressions — The Language of Lexers

Tokens are defined using **regular expressions**. For example:

| Token | Regular Expression |
|-------|-------------------|
| `IDENTIFIER` | `[a-zA-Z_][a-zA-Z0-9_]*` |
| `INTEGER` | `[0-9]+` |
| `FLOAT` | `[0-9]+\.[0-9]+` |
| `WHITESPACE` | `[ \t\n\r]+` (usually skipped) |
| `COMMENT` | `//.*` or `/\*[\s\S]*?\*/` |

### From Regular Expressions to Finite Automata

Regular expressions can be mechanically converted to **finite automata**:
1. **Thompson's construction** — converts a regex to a Non-deterministic Finite Automaton (NFA).
2. **Subset construction** — converts an NFA to a Deterministic Finite Automaton (DFA).
3. **DFA minimization** — produces a minimal DFA for efficient scanning.

```
Regex: a(b|c)*
  NFA:    b
       ┌──►(q1)──┐
  (q0)─┤         ├──►(qf)
       └──►(q2)──┘
          c
```

## 2.4 Hand-Written Lexer Example

For educational purposes and full control, many production compilers use hand-written lexers. Here is a simple lexer in C++:

```cpp
class Lexer {
    std::string source;
    size_t pos = 0;
    int line = 1, column = 1;

    char peek() {
        return pos < source.size() ? source[pos] : '\0';
    }

    char advance() {
        char c = source[pos++];
        if (c == '\n') { line++; column = 1; }
        else { column++; }
        return c;
    }

    void skipWhitespace() {
        while (isspace(peek())) advance();
    }

    Token lexNumber() {
        size_t start = pos - 1;
        while (isdigit(peek())) advance();
        if (peek() == '.' && isdigit(source[pos + 1])) {
            advance(); // consume '.'
            while (isdigit(peek())) advance();
            return {Token::FLOAT_LITERAL, source.substr(start, pos - start),
                    line, column, {.floatValue = std::stod(source.substr(start, pos - start))}};
        }
        return {Token::INT_LITERAL, source.substr(start, pos - start),
                line, column, {.intValue = std::stoll(source.substr(start, pos - start))}};
    }

    Token lexIdentifierOrKeyword() {
        size_t start = pos - 1;
        while (isalnum(peek()) || peek() == '_') advance();
        std::string lexeme = source.substr(start, pos - start);

        // Keyword lookup
        static const std::unordered_map<std::string, Token::Kind> keywords = {
            {"if", Token::KW_IF}, {"else", Token::KW_ELSE},
            {"while", Token::KW_WHILE}, {"return", Token::KW_RETURN},
            {"int", Token::KW_INT}, {"float", Token::KW_FLOAT},
        };
        auto it = keywords.find(lexeme);
        Token::Kind kind = (it != keywords.end()) ? it->second : Token::IDENTIFIER;

        return {kind, lexeme, line, column};
    }

public:
    Lexer(const std::string& src) : source(src) {}

    Token nextToken() {
        skipWhitespace();
        if (pos >= source.size()) return {Token::END_OF_FILE, "", line, column};

        char c = advance();

        // Single-character tokens
        switch (c) {
            case '+': return {Token::PLUS, "+", line, column};
            case '-': return {Token::MINUS, "-", line, column};
            case '*': return {Token::STAR, "*", line, column};
            case '/': return {Token::SLASH, "/", line, column};
            case '(': return {Token::LPAREN, "(", line, column};
            case ')': return {Token::RPAREN, ")", line, column};
            case '{': return {Token::LBRACE, "{", line, column};
            case '}': return {Token::RBRACE, "}", line, column};
            case ';': return {Token::SEMICOLON, ";", line, column};
            case ',': return {Token::COMMA, ",", line, column};
        }

        // Two-character tokens
        if (c == '=' && peek() == '=') { advance(); return {Token::EQ, "==", line, column}; }
        if (c == '!' && peek() == '=') { advance(); return {Token::NE, "!=", line, column}; }
        if (c == '<' && peek() == '=') { advance(); return {Token::LE, "<=", line, column}; }
        if (c == '>' && peek() == '=') { advance(); return {Token::GE, ">=", line, column}; }
        if (c == '=') return {Token::ASSIGN, "=", line, column};
        if (c == '<') return {Token::LT, "<", line, column};
        if (c == '>') return {Token::GT, ">", line, column};

        if (isdigit(c)) return lexNumber();
        if (isalpha(c) || c == '_') return lexIdentifierOrKeyword();

        return {Token::UNKNOWN, std::string(1, c), line, column};
    }

    std::vector<Token> lexAll() {
        std::vector<Token> tokens;
        Token tok;
        do {
            tok = nextToken();
            tokens.push_back(tok);
        } while (tok.kind != Token::END_OF_FILE);
        return tokens;
    }
};
```

## 2.5 Using Flex/Lex — Generator-Based Lexing

For more complex languages, tools like **Flex** generate lexers from declarative specifications:

```lex
%{
#include "parser.tab.h"
int line_num = 1;
%}

DIGIT    [0-9]
ID       [a-zA-Z_][a-zA-Z0-9_]*

%%
{DIGIT}+           { yylval.intVal = atoi(yytext); return INT_LITERAL; }
{ID}               { yylval.strVal = strdup(yytext);  return IDENTIFIER; }
"if"               { return IF; }
"else"             { return ELSE; }
"while"            { return WHILE; }
"return"           { return RETURN; }
"+"                { return PLUS; }
"-"                { return MINUS; }
"="                { return ASSIGN; }
";"                { return SEMICOLON; }
[ \t\n]            { /* skip whitespace */ }
"//".*             { /* skip comment */ }
.                  { fprintf(stderr, "Unexpected character: %s\n", yytext); }
%%
```

## 2.6 Error Handling in Lexers

Common lexical errors and recovery strategies:

| Error | Recovery Strategy |
|-------|-------------------|
| Unrecognized character | Skip and emit warning |
| Unterminated string | Insert synthetic closing quote |
| Unterminated comment | Treat to end of line/file |
| Numeric overflow | Clamp to max/min or emit error token |

## 2.7 Lexer Performance Considerations

- **DFA-based lexers** run in O(n) time where n is the input length.
- **Keyword recognition** via hash table lookup is O(1) average.
- **Buffer management** — read source in chunks to minimize I/O overhead.
- **Symbol interning** — store identifiers in a string table to deduplicate memory.

## 2.8 Summary

- The lexer converts a character stream into a token stream.
- Regular expressions describe token patterns; finite automata implement them efficiently.
- Both hand-written and generator-based lexers are viable; choose based on complexity and maintenance needs.
- Error recovery is crucial for a user-friendly compiler.

---

*Next: [Chapter 3: Syntax Analysis](./03-syntax-analysis.md)*
