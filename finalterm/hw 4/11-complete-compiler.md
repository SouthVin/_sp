# Chapter 11: Building a Complete Compiler — A Step-by-Step Example

## 11.1 The "TinyLang" Language

To put everything together, we'll build a complete compiler for **TinyLang** — a minimal C-like language that compiles to native code via LLVM.

### Language Features
- Integer and floating-point types: `int`, `float`
- Variables, assignment
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Control flow: `if`/`else`, `while`
- Functions with parameters and return values
- I/O: `print(x)`

### Example Program

```c
int add(int a, int b) {
    return a + b;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int x = 5;
    int y = 3;
    print(add(x, y));
    print(factorial(x));
    return 0;
}
```

## 11.2 Project Structure

```
tinylang/
├── CMakeLists.txt
├── src/
│   ├── main.cpp          # Driver
│   ├── lexer.hpp          # Lexer
│   ├── lexer.cpp
│   ├── token.hpp          # Token definitions
│   ├── ast.hpp            # AST definitions
│   ├── parser.hpp         # Parser
│   ├── parser.cpp
│   ├── sema.hpp           # Semantic analysis (symbol table, type checker)
│   ├── sema.cpp
│   ├── codegen.hpp        # LLVM code generation
│   └── codegen.cpp
├── test/
│   └── test.tl
└── README.md
```

## 11.3 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(TinyLang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM 17 REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs
    core irreader support nativecodegen
    orcjit passes transformutils)

add_executable(tinylang
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/sema.cpp
    src/codegen.cpp
)

target_link_libraries(tinylang ${llvm_libs})
```

## 11.4 Token Definitions

```cpp
// token.hpp
#pragma once
#include <string>
#include <variant>

enum class TokenKind {
    // Keywords
    KW_INT, KW_FLOAT, KW_VOID,
    KW_IF, KW_ELSE, KW_WHILE, KW_RETURN, KW_PRINT,
    // Literals
    INT_LITERAL, FLOAT_LITERAL,
    // Identifiers
    IDENTIFIER,
    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN,
    EQ, NE, LT, GT, LE, GE,
    LPAREN, RPAREN, LBRACE, RBRACE,
    SEMICOLON, COMMA,
    // Special
    END_OF_FILE, UNKNOWN
};

struct Token {
    TokenKind kind;
    std::string lexeme;
    int line, column;

    // Value storage for literals
    std::variant<int64_t, double, std::monostate> value;

    Token(TokenKind k, std::string lex, int l, int c)
        : kind(k), lexeme(std::move(lex)), line(l), column(c), value(std::monostate{}) {}

    int64_t asInt() const { return std::get<int64_t>(value); }
    double asFloat() const { return std::get<double>(value); }
    void setInt(int64_t v) { value = v; }
    void setFloat(double v) { value = v; }

    std::string kindStr() const;
};
```

## 11.5 Lexer

```cpp
// lexer.hpp
#pragma once
#include "token.hpp"
#include <vector>
#include <string>

class Lexer {
    std::string source;
    size_t pos = 0;
    int line = 1, column = 1;

    char peek() const;
    char peekAhead(int n) const;
    char advance();
    void skipWhitespace();
    void skipLineComment();
    Token lexNumber();
    Token lexIdentifierOrKeyword();

public:
    explicit Lexer(std::string source);
    Token nextToken();
    std::vector<Token> lexAll();
};
```

```cpp
// lexer.cpp
#include "lexer.hpp"
#include <cctype>
#include <unordered_map>

Lexer::Lexer(std::string src) : source(std::move(src)) {}

char Lexer::peek() const {
    return pos < source.size() ? source[pos] : '\0';
}

char Lexer::peekAhead(int n) const {
    return pos + n < source.size() ? source[pos + n] : '\0';
}

char Lexer::advance() {
    char c = source[pos++];
    if (c == '\n') { line++; column = 1; }
    else { column++; }
    return c;
}

void Lexer::skipWhitespace() {
    while (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r')
        advance();
}

void Lexer::skipLineComment() {
    while (peek() != '\n' && peek() != '\0') advance();
}

Token Lexer::nextToken() {
    while (true) {
        skipWhitespace();

        if (pos >= source.size())
            return Token(TokenKind::END_OF_FILE, "", line, column);

        // Skip comments
        if (peek() == '/' && peekAhead(1) == '/') {
            advance(); advance();
            skipLineComment();
            continue;
        }

        break;
    }

    char c = advance();

    if (c == '=' && peek() == '=') { advance(); return Token(TokenKind::EQ, "==", line, column); }
    if (c == '!' && peek() == '=') { advance(); return Token(TokenKind::NE, "!=", line, column); }
    if (c == '<' && peek() == '=') { advance(); return Token(TokenKind::LE, "<=", line, column); }
    if (c == '>' && peek() == '=') { advance(); return Token(TokenKind::GE, ">=", line, column); }

    switch (c) {
        case '+': return Token(TokenKind::PLUS, "+", line, column);
        case '-': return Token(TokenKind::MINUS, "-", line, column);
        case '*': return Token(TokenKind::STAR, "*", line, column);
        case '/': return Token(TokenKind::SLASH, "/", line, column);
        case '%': return Token(TokenKind::PERCENT, "%", line, column);
        case '=': return Token(TokenKind::ASSIGN, "=", line, column);
        case '<': return Token(TokenKind::LT, "<", line, column);
        case '>': return Token(TokenKind::GT, ">", line, column);
        case '(': return Token(TokenKind::LPAREN, "(", line, column);
        case ')': return Token(TokenKind::RPAREN, ")", line, column);
        case '{': return Token(TokenKind::LBRACE, "{", line, column);
        case '}': return Token(TokenKind::RBRACE, "}", line, column);
        case ';': return Token(TokenKind::SEMICOLON, ";", line, column);
        case ',': return Token(TokenKind::COMMA, ",", line, column);
    }

    if (std::isdigit(c)) return lexNumber();
    if (std::isalpha(c) || c == '_') return lexIdentifierOrKeyword();

    return Token(TokenKind::UNKNOWN, std::string(1, c), line, column);
}

Token Lexer::lexNumber() {
    size_t start = pos - 1;
    bool isFloat = false;
    while (std::isdigit(peek())) advance();
    if (peek() == '.' && std::isdigit(peekAhead(1))) {
        isFloat = true;
        advance();
        while (std::isdigit(peek())) advance();
    }

    std::string lexeme = source.substr(start, pos - start);
    Token tok(isFloat ? TokenKind::FLOAT_LITERAL : TokenKind::INT_LITERAL,
              lexeme, line, column);
    if (isFloat) tok.setFloat(std::stod(lexeme));
    else tok.setInt(std::stoll(lexeme));
    return tok;
}

Token Lexer::lexIdentifierOrKeyword() {
    size_t start = pos - 1;
    while (std::isalnum(peek()) || peek() == '_') advance();
    std::string lexeme = source.substr(start, pos - start);

    static const std::unordered_map<std::string, TokenKind> keywords = {
        {"int", TokenKind::KW_INT}, {"float", TokenKind::KW_FLOAT},
        {"void", TokenKind::KW_VOID}, {"if", TokenKind::KW_IF},
        {"else", TokenKind::KW_ELSE}, {"while", TokenKind::KW_WHILE},
        {"return", TokenKind::KW_RETURN}, {"print", TokenKind::KW_PRINT},
    };

    auto it = keywords.find(lexeme);
    TokenKind kind = (it != keywords.end()) ? it->second : TokenKind::IDENTIFIER;
    return Token(kind, lexeme, line, column);
}

std::vector<Token> Lexer::lexAll() {
    std::vector<Token> tokens;
    Token tok(TokenKind::UNKNOWN, "", 0, 0);
    do {
        tok = nextToken();
        tokens.push_back(tok);
    } while (tok.kind != TokenKind::END_OF_FILE);
    return tokens;
}
```

## 11.6 AST Definitions

```cpp
// ast.hpp
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <llvm/IR/Value.h>

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void print(int indent = 0) const = 0;
    virtual llvm::Value* codegen() = 0;
};

// Types
enum class TypeKind { VOID, INT, FLOAT };

struct Type {
    TypeKind kind;
    std::string toString() const {
        switch (kind) {
            case TypeKind::VOID: return "void";
            case TypeKind::INT: return "int";
            case TypeKind::FLOAT: return "float";
        }
        return "unknown";
    }
};

// Expressions
struct ExprAST : ASTNode {
    Type inferredType;
};

struct IntExpr : ExprAST {
    int64_t value;
    explicit IntExpr(int64_t v) : value(v) { inferredType = {TypeKind::INT}; }
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};

struct FloatExpr : ExprAST {
    double value;
    explicit FloatExpr(double v) : value(v) { inferredType = {TypeKind::FLOAT}; }
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};

struct VariableExpr : ExprAST {
    std::string name;
    explicit VariableExpr(std::string n) : name(std::move(n)) {}
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};

struct BinaryExpr : ExprAST {
    enum Op { ADD, SUB, MUL, DIV, MOD, EQ, NE, LT, GT, LE, GE };
    Op op;
    std::unique_ptr<ExprAST> lhs, rhs;
    std::string opStr() const;
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};

struct CallExpr : ExprAST {
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};

// Statements
struct ExprStmt : ASTNode {
    std::unique_ptr<ExprAST> expr;
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};

struct VarDeclStmt : ASTNode {
    Type type;
    std::string name;
    std::unique_ptr<ExprAST> init;  // optional
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};

struct IfStmt : ASTNode {
    std::unique_ptr<ExprAST> condition;
    std::unique_ptr<ASTNode> thenBlock;
    std::unique_ptr<ASTNode> elseBlock;
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};

struct WhileStmt : ASTNode {
    std::unique_ptr<ExprAST> condition;
    std::unique_ptr<ASTNode> body;
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};

struct ReturnStmt : ASTNode {
    std::unique_ptr<ExprAST> expr;  // nullptr for void return
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};

struct BlockStmt : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};

struct FunctionDecl : ASTNode {
    std::string name;
    Type returnType;
    std::vector<std::pair<std::string, Type>> params;
    std::unique_ptr<BlockStmt> body;
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};

struct Program : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> declarations;
    void print(int indent = 0) const override;
    llvm::Value* codegen() override;
};
```

## 11.7 Parser

```cpp
// parser.hpp
#pragma once
#include "ast.hpp"
#include "token.hpp"
#include <vector>

class Parser {
    std::vector<Token> tokens;
    size_t pos = 0;

    Token peek() const { return tokens[pos]; }
    Token peekAhead(int n) const { return tokens[pos + n]; }
    Token advance() { return tokens[pos++]; }
    Token expect(TokenKind kind);
    bool match(TokenKind kind);

    void error(const std::string& msg);

    // Grammar rules
    std::unique_ptr<Program> parseProgram();
    std::unique_ptr<FunctionDecl> parseFunction();
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<ExprAST> parseExpression();
    std::unique_ptr<ExprAST> parseAssignment();
    std::unique_ptr<ExprAST> parseComparison();
    std::unique_ptr<ExprAST> parseAddition();
    std::unique_ptr<ExprAST> parseMultiplication();
    std::unique_ptr<ExprAST> parseUnary();
    std::unique_ptr<ExprAST> parsePrimary();
    Type parseType();

public:
    explicit Parser(std::vector<Token> toks) : tokens(std::move(toks)) {}
    std::unique_ptr<Program> parse();
};
```

```cpp
// parser.cpp (key parts)
#include "parser.hpp"
#include <stdexcept>

Token Parser::expect(TokenKind kind) {
    if (peek().kind != kind) {
        error("Expected " + /* token string */ " but got " + peek().lexeme);
    }
    return advance();
}

void Parser::error(const std::string& msg) {
    auto& tok = peek();
    throw std::runtime_error("Parse error at line " +
        std::to_string(tok.line) + ":" + std::to_string(tok.column) + " - " + msg);
}

std::unique_ptr<Program> Parser::parse() {
    return parseProgram();
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();

    while (peek().kind != TokenKind::END_OF_FILE) {
        if (peek().kind == TokenKind::KW_INT || peek().kind == TokenKind::KW_FLOAT ||
            peek().kind == TokenKind::KW_VOID) {
            program->declarations.push_back(parseFunction());
        } else {
            error("Expected function declaration at top level");
        }
    }

    return program;
}

std::unique_ptr<ExprAST> Parser::parsePrimary() {
    if (peek().kind == TokenKind::INT_LITERAL) {
        auto tok = advance();
        return std::make_unique<IntExpr>(tok.asInt());
    }
    if (peek().kind == TokenKind::FLOAT_LITERAL) {
        auto tok = advance();
        return std::make_unique<FloatExpr>(tok.asFloat());
    }
    if (peek().kind == TokenKind::IDENTIFIER) {
        auto tok = advance();
        // Check for function call
        if (peek().kind == TokenKind::LPAREN) {
            advance(); // consume '('
            auto call = std::make_unique<CallExpr>(tok.lexeme);
            if (peek().kind != TokenKind::RPAREN) {
                call->args.push_back(parseExpression());
                while (peek().kind == TokenKind::COMMA) {
                    advance();
                    call->args.push_back(parseExpression());
                }
            }
            expect(TokenKind::RPAREN);
            return call;
        }
        return std::make_unique<VariableExpr>(tok.lexeme);
    }
    if (peek().kind == TokenKind::LPAREN) {
        advance();
        auto expr = parseExpression();
        expect(TokenKind::RPAREN);
        return expr;
    }
    error("Expected expression");
    return nullptr;
}
```

## 11.8 Code Generation

```cpp
// codegen.hpp
#pragma once
#include "ast.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPMOptimizer.h>
#include <map>
#include <memory>

class CodeGenerator {
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::map<std::string, llvm::AllocaInst*> namedValues;

    // Symbol table: maps function names to their LLVM Function
    std::map<std::string, llvm::Function*> functionTable;

    // Helper to create alloca at the entry of a function
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* func,
                                              const std::string& name,
                                              llvm::Type* type);

public:
    CodeGenerator(const std::string& moduleName);

    llvm::Module* getModule() const { return module.get(); }
    llvm::LLVMContext* getContext() const { return context.get(); }

    void generate(Program& program);
    llvm::Function* generateFunction(FunctionDecl& decl);
    llvm::Value* generateExpr(ExprAST& expr);
    llvm::Value* generateStmt(ASTNode& stmt);
    llvm::Value* generateBlock(BlockStmt& block);

    void printIR(const std::string& filename);
    void runOptimizations();
    void compileToObjectFile(const std::string& filename);

    void dump() const { module->print(llvm::outs(), nullptr); }
};
```

```cpp
// codegen.cpp (key methods)
#include "codegen.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <iostream>

CodeGenerator::CodeGenerator(const std::string& moduleName)
    : context(std::make_unique<llvm::LLVMContext>()),
      module(std::make_unique<llvm::Module>(moduleName, *context)),
      builder(std::make_unique<llvm::IRBuilder<>>(*context))
{
    // Register built-in print function
    auto* printType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(*context),
        {llvm::Type::getInt32Ty(*context)},
        false);
    llvm::Function::Create(printType, llvm::Function::ExternalLinkage,
                           "print_int", module.get());
}

llvm::AllocaInst* CodeGenerator::createEntryBlockAlloca(
    llvm::Function* func, const std::string& name, llvm::Type* type) {
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(),
                                  func->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, name);
}

void CodeGenerator::generate(Program& program) {
    for (auto& decl : program.declarations) {
        if (auto* funcDecl = dynamic_cast<FunctionDecl*>(decl.get())) {
            generateFunction(*funcDecl);
        }
    }
}

llvm::Function* CodeGenerator::generateFunction(FunctionDecl& decl) {
    // Determine LLVM types
    llvm::Type* retType;
    switch (decl.returnType.kind) {
        case TypeKind::VOID: retType = llvm::Type::getVoidTy(*context); break;
        case TypeKind::INT: retType = llvm::Type::getInt32Ty(*context); break;
        case TypeKind::FLOAT: retType = llvm::Type::getDoubleTy(*context); break;
    }

    std::vector<llvm::Type*> paramTypes;
    for (auto& p : decl.params) {
        paramTypes.push_back(llvm::Type::getInt32Ty(*context));
    }

    auto* funcType = llvm::FunctionType::get(retType, paramTypes, false);
    auto* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                        decl.name, module.get());

    // Set parameter names
    unsigned idx = 0;
    for (auto& arg : func->args())
        arg.setName(decl.params[idx++].first);

    // Create entry block
    auto* entry = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entry);

    // Allocate parameters
    namedValues.clear();
    for (auto& arg : func->args()) {
        auto* alloca = createEntryBlockAlloca(func,
            std::string(arg.getName()), arg.getType());
        builder->CreateStore(&arg, alloca);
        namedValues[std::string(arg.getName())] = alloca;
    }

    // Generate body
    generateBlock(*decl.body);

    if (decl.returnType.kind == TypeKind::VOID)
        builder->CreateRetVoid();

    llvm::verifyFunction(*func);
    return func;
}

llvm::Value* CodeGenerator::generateExpr(ExprAST& expr) {
    if (auto* intExpr = dynamic_cast<IntExpr*>(&expr)) {
        return llvm::ConstantInt::get(*context, llvm::APInt(32, intExpr->value, true));
    }
    if (auto* floatExpr = dynamic_cast<FloatExpr*>(&expr)) {
        return llvm::ConstantFP::get(*context, llvm::APFloat(floatExpr->value));
    }
    if (auto* varExpr = dynamic_cast<VariableExpr*>(&expr)) {
        auto* alloca = namedValues[varExpr->name];
        if (!alloca) {
            std::cerr << "Unknown variable: " << varExpr->name << "\n";
            return nullptr;
        }
        return builder->CreateLoad(alloca->getAllocatedType(), alloca, varExpr->name);
    }
    if (auto* binExpr = dynamic_cast<BinaryExpr*>(&expr)) {
        llvm::Value* lhs = generateExpr(*binExpr->lhs);
        llvm::Value* rhs = generateExpr(*binExpr->rhs);
        if (!lhs || !rhs) return nullptr;

        switch (binExpr->op) {
            case BinaryExpr::ADD: return builder->CreateAdd(lhs, rhs, "addtmp");
            case BinaryExpr::SUB: return builder->CreateSub(lhs, rhs, "subtmp");
            case BinaryExpr::MUL: return builder->CreateMul(lhs, rhs, "multmp");
            case BinaryExpr::DIV: return builder->CreateSDiv(lhs, rhs, "divtmp");
            case BinaryExpr::EQ:  return builder->CreateICmpEQ(lhs, rhs, "eqtmp");
            case BinaryExpr::NE:  return builder->CreateICmpNE(lhs, rhs, "netmp");
            case BinaryExpr::LT:  return builder->CreateICmpSLT(lhs, rhs, "lttmp");
            case BinaryExpr::GT:  return builder->CreateICmpSGT(lhs, rhs, "gttmp");
            case BinaryExpr::LE:  return builder->CreateICmpSLE(lhs, rhs, "letmp");
            case BinaryExpr::GE:  return builder->CreateICmpSGE(lhs, rhs, "getmp");
            default: return nullptr;
        }
    }
    if (auto* callExpr = dynamic_cast<CallExpr*>(&expr)) {
        llvm::Function* callee = module->getFunction(callExpr->callee);
        if (!callee) {
            std::cerr << "Unknown function: " << callExpr->callee << "\n";
            return nullptr;
        }
        std::vector<llvm::Value*> args;
        for (auto& arg : callExpr->args) {
            args.push_back(generateExpr(*arg));
            if (!args.back()) return nullptr;
        }
        return builder->CreateCall(callee, args, "calltmp");
    }
    return nullptr;
}

llvm::Value* CodeGenerator::generateStmt(ASTNode& stmt) {
    if (auto* exprStmt = dynamic_cast<ExprStmt*>(&stmt)) {
        return generateExpr(*exprStmt->expr);
    }
    if (auto* varDecl = dynamic_cast<VarDeclStmt*>(&stmt)) {
        llvm::Function* func = builder->GetInsertBlock()->getParent();
        llvm::Type* type = llvm::Type::getInt32Ty(*context);
        auto* alloca = createEntryBlockAlloca(func, varDecl->name, type);

        if (varDecl->init) {
            auto* initVal = generateExpr(*varDecl->init);
            if (!initVal) return nullptr;
            builder->CreateStore(initVal, alloca);
        }
        namedValues[varDecl->name] = alloca;
        return alloca;
    }
    if (auto* ifStmt = dynamic_cast<IfStmt*>(&stmt)) {
        llvm::Value* cond = generateExpr(*ifStmt->condition);
        if (!cond) return nullptr;

        llvm::Function* func = builder->GetInsertBlock()->getParent();
        auto* thenBB = llvm::BasicBlock::Create(*context, "then", func);
        auto* elseBB = llvm::BasicBlock::Create(*context, "else");
        auto* mergeBB = llvm::BasicBlock::Create(*context, "ifcont");

        builder->CreateCondBr(cond, thenBB, elseBB);

        builder->SetInsertPoint(thenBB);
        generateStmt(*ifStmt->thenBlock);
        builder->CreateBr(mergeBB);

        func->insert(func->end(), elseBB);
        builder->SetInsertPoint(elseBB);
        if (ifStmt->elseBlock)
            generateStmt(*ifStmt->elseBlock);
        builder->CreateBr(mergeBB);

        func->insert(func->end(), mergeBB);
        builder->SetInsertPoint(mergeBB);
        return nullptr;
    }
    if (auto* whileStmt = dynamic_cast<WhileStmt*>(&stmt)) {
        llvm::Function* func = builder->GetInsertBlock()->getParent();
        auto* condBB = llvm::BasicBlock::Create(*context, "whilecond", func);
        auto* bodyBB = llvm::BasicBlock::Create(*context, "whilebody");
        auto* endBB  = llvm::BasicBlock::Create(*context, "whileend");

        builder->CreateBr(condBB);
        builder->SetInsertPoint(condBB);

        llvm::Value* cond = generateExpr(*whileStmt->condition);
        if (!cond) return nullptr;
        builder->CreateCondBr(cond, bodyBB, endBB);

        func->insert(func->end(), bodyBB);
        builder->SetInsertPoint(bodyBB);
        generateStmt(*whileStmt->body);
        builder->CreateBr(condBB);

        func->insert(func->end(), endBB);
        builder->SetInsertPoint(endBB);
        return nullptr;
    }
    if (auto* retStmt = dynamic_cast<ReturnStmt*>(&stmt)) {
        if (retStmt->expr) {
            auto* val = generateExpr(*retStmt->expr);
            if (!val) return nullptr;
            return builder->CreateRet(val);
        }
        return builder->CreateRetVoid();
    }
    if (auto* block = dynamic_cast<BlockStmt*>(&stmt)) {
        return generateBlock(*block);
    }
    return nullptr;
}

llvm::Value* CodeGenerator::generateBlock(BlockStmt& block) {
    for (auto& stmt : block.statements) {
        generateStmt(*stmt);
    }
    return nullptr;
}
```

## 11.9 Main Driver

```cpp
// main.cpp
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Cannot open file: " << filename << "\n";
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: tinylang <input.tl>\n";
        return 1;
    }

    std::string source = readFile(argv[1]);

    // Lexical analysis
    Lexer lexer(source);
    auto tokens = lexer.lexAll();

    // Parsing
    Parser parser(tokens);
    std::unique_ptr<Program> ast;
    try {
        ast = parser.parse();
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return 1;
    }

    // Code generation
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    CodeGenerator cg("tinylang_module");
    cg.generate(*ast);

    std::cout << "=== Generated LLVM IR ===\n";
    cg.dump();

    // Optimize
    cg.runOptimizations();

    std::cout << "\n=== Optimized LLVM IR ===\n";
    cg.dump();

    // Compile to object file
    cg.compileToObjectFile("output.o");
    std::cout << "\nCompiled to output.o\n";

    return 0;
}
```

## 11.10 Compiling and Running

```bash
# Build the compiler
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/usr/lib/llvm-17 ..
make

# Compile a TinyLang program
echo '
int add(int a, int b) {
    return a + b;
}

int main() {
    print(add(10, 20));
    return 0;
}
' > test.tl

./tinylang test.tl

# Link with a C runtime that provides print_int
clang output.o print_runtime.c -o program
./program
# Output: 30
```

## 11.11 Summary

This chapter demonstrated a complete compiler pipeline:
1. **Lexer** — converts source text to tokens.
2. **Parser** — builds an AST using recursive descent.
3. **Code generator** — walks the AST and emits LLVM IR using `IRBuilder`.
4. **Optimizer** — runs LLVM passes on the generated IR.
5. **Backend** — emits native machine code via `TargetMachine`.

The complete TinyLang compiler is approximately 800 lines of C++, yet produces optimized native code rivaling hand-written C for equivalent programs.

---

*Next: [Chapter 12: Advanced Topics & Future Directions](./12-advanced-topics.md)*
