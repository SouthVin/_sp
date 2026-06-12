# Chapter 4: Semantic Analysis

## 4.1 What Is Semantic Analysis?

While syntax analysis checks that a program is **grammatically correct**, semantic analysis verifies that it is **meaningful**. It answers questions like:

- Is this variable declared before use?
- Are the types in this expression compatible?
- Does this function call have the right number of arguments?
- Is the return type consistent with the function declaration?

```
Decorated AST ──► [ Semantic Analyzer ] ──► Validated AST + Symbol Table
                                          ──► Semantic Errors
```

## 4.2 The Symbol Table

The **symbol table** is the central data structure for semantic analysis. It maps names (identifiers) to their declarations and tracks scoping information.

### Symbol Table Structure

```cpp
struct SymbolInfo {
    std::string name;
    enum Kind { VARIABLE, FUNCTION, PARAMETER } kind;
    std::string type;         // "int", "float", "void"
    bool isDefined = false;
    int scopeLevel;
    // For functions
    std::vector<std::string> paramTypes;
};

class SymbolTable {
    struct Scope {
        std::unordered_map<std::string, SymbolInfo> symbols;
    };

    std::vector<Scope> scopes;

public:
    void enterScope() {
        scopes.push_back(Scope{});
    }

    void exitScope() {
        scopes.pop_back();
    }

    bool define(const std::string& name, SymbolInfo info) {
        if (scopes.back().symbols.count(name)) return false; // redefinition
        info.scopeLevel = scopes.size();
        scopes.back().symbols[name] = info;
        return true;
    }

    SymbolInfo* lookup(const std::string& name) {
        // Search from innermost to outermost scope
        for (int i = scopes.size() - 1; i >= 0; --i) {
            auto it = scopes[i].symbols.find(name);
            if (it != scopes[i].symbols.end()) return &it->second;
        }
        return nullptr;
    }

    bool isDefinedInCurrentScope(const std::string& name) {
        return scopes.back().symbols.count(name);
    }
};
```

### Scope Rules

```c
int x = 1;        // Global scope: x declared

int foo(int y) {  // Function scope: y is a parameter
    int x = 2;    // Local scope: shadows global x
    {
        int z = 3;  // Nested block scope
        // x refers to local x (2), y is accessible, z is accessible
    }
    // z is out of scope here
    return x;      // returns 2
}
```

## 4.3 Type Checking

### Type System

A **type system** defines rules for how types interact. Our simple language has:

```cpp
struct Type {
    enum Kind { INT, FLOAT, VOID, BOOL, FUNCTION, ARRAY, USER_DEFINED } kind;

    // For functions
    Type returnType;
    std::vector<Type> paramTypes;

    bool operator==(const Type& other) const {
        return kind == other.kind;
    }

    bool isNumeric() const { return kind == INT || kind == FLOAT; }
};
```

### Type Checking Expressions

```cpp
Type typeCheck(ASTNode* node, SymbolTable& symtab) {
    if (auto* intExpr = dynamic_cast<IntExpr*>(node)) {
        return Type{Type::INT};
    }
    if (auto* floatExpr = dynamic_cast<FloatExpr*>(node)) {
        return Type{Type::FLOAT};
    }
    if (auto* var = dynamic_cast<VariableExpr*>(node)) {
        auto* sym = symtab.lookup(var->name);
        if (!sym) throw std::runtime_error("Undeclared variable: " + var->name);
        var->symbol = sym; // link AST node to symbol
        return sym->type;
    }
    if (auto* bin = dynamic_cast<BinaryExpr*>(node)) {
        Type lhs = typeCheck(bin->lhs.get(), symtab);
        Type rhs = typeCheck(bin->rhs.get(), symtab);

        if (bin->op == BinaryExpr::EQ || bin->op == BinaryExpr::NE ||
            bin->op == BinaryExpr::LT || bin->op == BinaryExpr::GT ||
            bin->op == BinaryExpr::LE || bin->op == BinaryExpr::GE) {
            return Type{Type::BOOL};  // comparison yields boolean
        }
        if (!lhs.isNumeric() || !rhs.isNumeric())
            throw std::runtime_error("Arithmetic requires numeric operands");
        // Type coercion: if either is float, result is float
        if (lhs.kind == Type::FLOAT || rhs.kind == Type::FLOAT)
            return Type{Type::FLOAT};
        return Type{Type::INT};
    }
    // ... handle other nodes
}
```

### Implicit Type Conversion (Coercion)

```cpp
Type promoteType(Type a, Type b) {
    if (a == b) return a;
    if (a.kind == Type::INT && b.kind == Type::FLOAT) return Type{Type::FLOAT};
    if (a.kind == Type::FLOAT && b.kind == Type::INT) return Type{Type::FLOAT};
    throw std::runtime_error("Incompatible types");
}
```

## 4.4 Statement-Level Checks

### Variable Declaration

```cpp
void checkDeclaration(ASTNode* node, SymbolTable& symtab) {
    // varDecl ::= type ident ["=" expr] ";"
    // Check: variable not already declared in this scope
    // Check: if initializer exists, type matches
}

void checkAssignment(ASTNode* node, SymbolTable& symtab) {
    // assign ::= ident "=" expr
    // Check: variable is declared
    // Check: expression type is assignable to variable type
}
```

### Function Declarations and Calls

```cpp
void checkFunctionDecl(ASTNode* node, SymbolTable& symtab) {
    // Check: function not already declared (or matches previous declaration)
    // Check: parameter names are unique
    // Check: return statements match declared return type
}

void checkFunctionCall(ASTNode* node, SymbolTable& symtab) {
    // call ::= ident "(" [expr ("," expr)*] ")"
    // Check: function is declared
    // Check: argument count matches parameter count
    // Check: each argument type matches or is convertible to parameter type
}
```

## 4.5 Visitor Pattern for AST Traversal

The **visitor pattern** separates algorithms from the AST data structure, making it easy to add new analyses:

```cpp
class ASTVisitor {
public:
    virtual void visit(IntExpr& node) = 0;
    virtual void visit(FloatExpr& node) = 0;
    virtual void visit(VariableExpr& node) = 0;
    virtual void visit(BinaryExpr& node) = 0;
    virtual void visit(UnaryExpr& node) = 0;
    virtual void visit(AssignExpr& node) = 0;
    virtual void visit(CallExpr& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(WhileStmt& node) = 0;
    virtual void visit(ReturnStmt& node) = 0;
    virtual void visit(BlockStmt& node) = 0;
    virtual void visit(FunctionDecl& node) = 0;
    virtual void visit(Program& node) = 0;
};

class TypeChecker : public ASTVisitor {
    SymbolTable& symtab;
    Type currentFunctionReturn;
    bool hasError = false;

public:
    TypeChecker(SymbolTable& st) : symtab(st) {}

    void visit(IntExpr& node) override { node.inferredType = Type{Type::INT}; }
    void visit(FloatExpr& node) override { node.inferredType = Type{Type::FLOAT}; }
    // ... implement all visit methods
};
```

## 4.6 Attributes and Annotations

During semantic analysis, the AST is **decorated** (annotated) with additional information:

| Annotation | Where | Purpose |
|------------|-------|---------|
| `inferredType` | Expression nodes | The computed type of the expression |
| `symbol` | Variable/Call nodes | Link to the symbol table entry |
| `scope` | Block nodes | Reference to the scope |
| `isLValue` | Expression nodes | Whether the expression can appear on the left of `=` |

## 4.7 Semantic Errors

Common semantic errors and how to report them:

```cpp
// Undeclared variable
if (!sym) {
    reportError("Undeclared variable '" + name + "'", line, col);
}

// Type mismatch
if (!isCompatible(expected, actual)) {
    reportError("Type mismatch: expected '" + typeToString(expected) +
                "' but got '" + typeToString(actual) + "'", line, col);
}

// Redeclaration
if (symtab.isDefinedInCurrentScope(name)) {
    reportError("Redeclaration of '" + name + "'", line, col);
}

// Return type mismatch
if (funcReturnType != Type{Type::VOID} && !matches) {
    reportError("Return type mismatch in function '" + funcName + "'", line, col);
}
```

## 4.8 Summary

- Semantic analysis enforces the language's meaning rules beyond syntax.
- The symbol table tracks declarations through nested scopes.
- Type checking ensures operations are applied to compatible types.
- The visitor pattern cleanly separates AST structure from analysis algorithms.
- A decorated AST carries all the information needed for code generation.

---

*Next: [Chapter 5: Intermediate Representation](./05-intermediate-representation.md)*
