# Chapter 3: Syntax Analysis (Parsing)

## 3.1 What Is Parsing?

**Syntax analysis** (parsing) takes the stream of tokens from the lexer and builds an **Abstract Syntax Tree (AST)** according to the language's grammar. The parser checks that the token sequence conforms to the language's syntactic rules.

```
Token Stream   ──► [ Parser ] ──► Abstract Syntax Tree (AST)
                              ──► Syntax Errors
```

## 3.2 Context-Free Grammars

Programming language syntax is specified using **Context-Free Grammars (CFGs)**. A CFG consists of:

- **Terminals** — the tokens from the lexer (e.g., `+`, `if`, `42`, `identifier`)
- **Non-terminals** — syntactic variables (e.g., `Expression`, `Statement`)
- **Productions** — rules mapping non-terminals to sequences of terminals and non-terminals
- **Start symbol** — the top-level non-terminal (e.g., `Program`)

### BNF Notation (Backus-Naur Form)

```
<program>      ::= <statement-list>
<statement-list> ::= <statement> | <statement> <statement-list>
<statement>    ::= <expression> ";"
                 | "if" "(" <expression> ")" <statement>
                 | "if" "(" <expression> ")" <statement> "else" <statement>
                 | "while" "(" <expression> ")" <statement>
                 | "{" <statement-list> "}"
                 | "return" <expression> ";"
<expression>   ::= <assignment>
<assignment>   ::= <identifier> "=" <assignment> | <comparison>
<comparison>   ::= <addition> (("==" | "!=" | "<" | ">" | "<=" | ">=") <addition>)*
<addition>     ::= <multiplication> (("+" | "-") <multiplication>)*
<multiplication> ::= <unary> (("*" | "/") <unary>)*
<unary>        ::= ("-" | "!") <unary> | <primary>
<primary>      ::= <identifier> | <integer> | <float> | "(" <expression> ")"
```

## 3.3 Abstract Syntax Tree (AST)

The AST is a tree representation that captures the **structure** of the program, discarding syntactic details like parentheses and semicolons.

```
Input:  "x = 3 + 4 * 5;"

AST:
      Assign
      /     \
   ID(x)    Add
           /   \
       Int(3)  Mul
              /   \
          Int(4)  Int(5)
```

### AST Node Definitions

```cpp
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void print(int indent = 0) const = 0;
};

// Expressions
struct IntExpr : ASTNode {
    int64_t value;
    IntExpr(int64_t v) : value(v) {}
};

struct FloatExpr : ASTNode {
    double value;
    FloatExpr(double v) : value(v) {}
};

struct VariableExpr : ASTNode {
    std::string name;
    VariableExpr(const std::string& n) : name(n) {}
};

struct BinaryExpr : ASTNode {
    enum Op { ADD, SUB, MUL, DIV, EQ, NE, LT, GT, LE, GE };
    Op op;
    std::unique_ptr<ASTNode> lhs, rhs;
    BinaryExpr(Op o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
        : op(o), lhs(std::move(l)), rhs(std::move(r)) {}
};

struct UnaryExpr : ASTNode {
    enum Op { NEG, NOT };
    Op op;
    std::unique_ptr<ASTNode> operand;
};

struct AssignExpr : ASTNode {
    std::string name;
    std::unique_ptr<ASTNode> value;
};

struct CallExpr : ASTNode {
    std::string callee;
    std::vector<std::unique_ptr<ASTNode>> args;
};

// Statements
struct ExprStmt : ASTNode {
    std::unique_ptr<ASTNode> expr;
};

struct IfStmt : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenBranch;
    std::unique_ptr<ASTNode> elseBranch; // may be null
};

struct WhileStmt : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
};

struct ReturnStmt : ASTNode {
    std::unique_ptr<ASTNode> expr;
};

struct BlockStmt : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
};

struct FunctionDecl : ASTNode {
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<ASTNode> body;
};

struct Program : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> declarations;
};
```

## 3.4 Parsing Techniques

### 3.4.1 Top-Down Parsing (Recursive Descent)

Recursive descent is the simplest parsing technique to implement by hand. Each non-terminal corresponds to a parsing function. This requires an **LL(k)** grammar (no left recursion).

```cpp
class Parser {
    std::vector<Token> tokens;
    size_t pos = 0;

    Token peek() { return tokens[pos]; }
    Token advance() { return tokens[pos++]; }
    Token expect(Token::Kind kind) {
        if (peek().kind != kind) error("Expected token");
        return advance();
    }

    void error(const std::string& msg) {
        auto& tok = peek();
        throw std::runtime_error(msg + " at line " +
            std::to_string(tok.line) + ":" + std::to_string(tok.column));
    }

public:
    Parser(const std::vector<Token>& toks) : tokens(toks) {}

    // expression ::= assignment
    std::unique_ptr<ASTNode> parseExpression() {
        return parseAssignment();
    }

    // assignment ::= identifier "=" assignment | comparison
    std::unique_ptr<ASTNode> parseAssignment() {
        auto expr = parseComparison();
        if (peek().kind == Token::ASSIGN) {
            if (auto* var = dynamic_cast<VariableExpr*>(expr.get())) {
                advance();
                auto rhs = parseAssignment();
                return std::make_unique<AssignExpr>(var->name, std::move(rhs));
            }
            error("Left side of assignment must be a variable");
        }
        return expr;
    }

    // comparison ::= addition (("==" | "!=" | "<" | ...) addition)*
    std::unique_ptr<ASTNode> parseComparison() {
        auto expr = parseAddition();
        while (true) {
            BinaryExpr::Op op;
            switch (peek().kind) {
                case Token::EQ: op = BinaryExpr::EQ; break;
                case Token::NE: op = BinaryExpr::NE; break;
                case Token::LT: op = BinaryExpr::LT; break;
                case Token::GT: op = BinaryExpr::GT; break;
                case Token::LE: op = BinaryExpr::LE; break;
                case Token::GE: op = BinaryExpr::GE; break;
                default: return expr;
            }
            advance();
            auto rhs = parseAddition();
            expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(rhs));
        }
    }

    // addition ::= multiplication (("+" | "-") multiplication)*
    std::unique_ptr<ASTNode> parseAddition() {
        auto expr = parseMultiplication();
        while (peek().kind == Token::PLUS || peek().kind == Token::MINUS) {
            auto op = (peek().kind == Token::PLUS) ? BinaryExpr::ADD : BinaryExpr::SUB;
            advance();
            auto rhs = parseMultiplication();
            expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(rhs));
        }
        return expr;
    }

    // multiplication ::= unary (("*" | "/") unary)*
    std::unique_ptr<ASTNode> parseMultiplication() {
        auto expr = parseUnary();
        while (peek().kind == Token::STAR || peek().kind == Token::SLASH) {
            auto op = (peek().kind == Token::STAR) ? BinaryExpr::MUL : BinaryExpr::DIV;
            advance();
            auto rhs = parseUnary();
            expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(rhs));
        }
        return expr;
    }

    // unary ::= ("-" | "!") unary | primary
    std::unique_ptr<ASTNode> parseUnary() {
        if (peek().kind == Token::MINUS) {
            advance();
            return std::make_unique<UnaryExpr>(UnaryExpr::NEG, parseUnary());
        }
        // ...
        return parsePrimary();
    }

    // primary ::= integer | float | identifier | "(" expression ")"
    std::unique_ptr<ASTNode> parsePrimary() {
        auto tok = advance();
        switch (tok.kind) {
            case Token::INT_LITERAL: return std::make_unique<IntExpr>(tok.intValue);
            case Token::FLOAT_LITERAL: return std::make_unique<FloatExpr>(tok.floatValue);
            case Token::IDENTIFIER: return std::make_unique<VariableExpr>(tok.lexeme);
            case Token::LPAREN: {
                auto expr = parseExpression();
                expect(Token::RPAREN);
                return expr;
            }
            default: error("Unexpected token: " + tok.lexeme);
        }
        return nullptr;
    }
};
```

### 3.4.2 Handling Left Recursion

Left-recursive rules like `E ::= E + T` cause infinite recursion in recursive descent. Fix by rewriting:

```
Before:  E ::= E + T | T
After:   E  ::= T E'
         E' ::= + T E' | ε
```

### 3.4.3 Bottom-Up Parsing (LR Parsing)

For complex grammars, **LR parsers** (generated by Yacc/Bison) are more powerful. LR parsing works by **shift-reduce** — reading tokens onto a stack and reducing them when a rule's right-hand side is matched.

## 3.5 Operator Precedence Parsing

For expression parsing, **Pratt parsing** (top-down operator precedence) is elegant and efficient:

```cpp
std::unique_ptr<ASTNode> parseExpression(int minPrecedence = 0) {
    auto left = parseAtom(); // parse prefix/integer/identifier

    while (true) {
        int precedence = getPrecedence(peek().kind);
        if (precedence < minPrecedence) break;

        auto op = advance(); // consume operator
        auto right = parseExpression(precedence + (isLeftAssoc(op.kind) ? 1 : 0));
        left = std::make_unique<BinaryExpr>(opToBinary(op.kind),
                                            std::move(left), std::move(right));
    }
    return left;
}

// Precedence table
int getPrecedence(Token::Kind kind) {
    switch (kind) {
        case Token::ASSIGN: return 1;
        case Token::EQ: case Token::NE: return 2;
        case Token::LT: case Token::GT: case Token::LE: case Token::GE: return 3;
        case Token::PLUS: case Token::MINUS: return 4;
        case Token::STAR: case Token::SLASH: return 5;
        default: return 0;
    }
}
```

## 3.6 Using Bison/Yacc

```yacc
%{
#include "ast.h"
%}

%token INT_LITERAL FLOAT_LITERAL IDENTIFIER
%token PLUS MINUS STAR SLASH ASSIGN
%token IF ELSE WHILE RETURN
%token LPAREN RPAREN LBRACE RBRACE SEMICOLON

%left ASSIGN
%left EQ NE
%left LT GT LE GE
%left PLUS MINUS
%left STAR SLASH

%%
program:    statement_list
          ;

statement_list:
            statement
          | statement_list statement
          ;

statement:  expression SEMICOLON                     { $$ = makeExprStmt($1); }
          | IF LPAREN expression RPAREN statement    { $$ = makeIfStmt($3, $5, nullptr); }
          | IF LPAREN expression RPAREN statement ELSE statement
                                                     { $$ = makeIfStmt($3, $5, $7); }
          | WHILE LPAREN expression RPAREN statement { $$ = makeWhileStmt($3, $5); }
          | RETURN expression SEMICOLON              { $$ = makeReturnStmt($2); }
          | LBRACE statement_list RBRACE             { $$ = $2; }
          ;

expression: IDENTIFIER ASSIGN expression   { $$ = makeAssignExpr($1, $3); }
          | comparison                    { $$ = $1; }
          ;

comparison:  addition
          |  comparison EQ addition       { $$ = makeBinaryExpr(EQ, $1, $3); }
          |  comparison NE addition       { $$ = makeBinaryExpr(NE, $1, $3); }
          ;

addition:    multiplication
          |  addition PLUS multiplication { $$ = makeBinaryExpr(ADD, $1, $3); }
          |  addition MINUS multiplication{ $$ = makeBinaryExpr(SUB, $1, $3); }
          ;

multiplication: primary
          |  multiplication STAR primary  { $$ = makeBinaryExpr(MUL, $1, $3); }
          |  multiplication SLASH primary { $$ = makeBinaryExpr(DIV, $1, $3); }
          ;

primary:    INT_LITERAL          { $$ = makeIntExpr($1); }
          | FLOAT_LITERAL        { $$ = makeFloatExpr($1); }
          | IDENTIFIER           { $$ = makeVarExpr($1); }
          | LPAREN expression RPAREN { $$ = $2; }
          ;
%%
```

## 3.7 Error Recovery in Parsers

| Strategy | Description |
|----------|-------------|
| **Panic mode** | Skip tokens until a synchronizing token (e.g., `;`, `}`) is found |
| **Phrase-level recovery** | Insert or delete a token to fix the input locally |
| **Error productions** | Add grammar rules for common mistakes |
| **Global correction** | Find the minimal sequence of changes to produce a valid program (theoretically optimal, practically expensive) |

## 3.8 Summary

- Parsing builds an AST from a token stream based on a context-free grammar.
- Recursive descent (LL) is simple and hand-written; LR parsers (Bison) handle more complex grammars.
- Pratt parsing elegantly handles operator precedence.
- A well-designed AST is the foundation for all subsequent compiler phases.

---

*Next: [Chapter 4: Semantic Analysis](./04-semantic-analysis.md)*
