# Chapter 5: Intermediate Representation

## 5.1 Why Intermediate Representation?

An **Intermediate Representation (IR)** is the bridge between the frontend and backend of a compiler. It decouples language-specific parsing from machine-specific code generation:

```
Frontend 1 (C)     ──┐
Frontend 2 (Rust)  ──┼──► [ IR ] ──► Backend 1 (x86)
Frontend 3 (Swift) ──┘              ──► Backend 2 (ARM)
                                    ──► Backend 3 (RISC-V)
```

With N frontends and M backends, IR reduces complexity from O(N × M) to O(N + M).

## 5.2 IR Design Trade-offs

| Property | Description |
|----------|-------------|
| **Level** | High-level (close to source) vs. Low-level (close to machine) |
| **Structure** | Linear (sequence of instructions) vs. Graphical (CFG, SSA) vs. Hybrid |
| **Typing** | Strongly typed vs. Untyped |
| **Expressiveness** | General-purpose vs. Specialized for certain analyses |

## 5.3 Three-Address Code (TAC)

**Three-Address Code** is a linear IR where each instruction has at most three operands. It's simple, well-understood, and forms the basis for many real IRs.

```
Format:  result = operand1 operator operand2

Examples:
  t1 = a + b
  t2 = t1 * c
  t3 = t2 - 5
  if t3 > 0 goto L1
  goto L2
```

### TAC Instruction Set

```cpp
enum class TACOp {
    // Arithmetic
    ADD, SUB, MUL, DIV, NEG,
    // Comparison
    EQ, NE, LT, GT, LE, GE,
    // Control flow
    JMP, JMP_IF_TRUE, JMP_IF_FALSE,
    // Memory
    LOAD, STORE,
    // Function calls
    CALL, RETURN,
    // Assignment
    MOV,
    // Special
    LABEL, PARAM, PHI
};

struct TACInstr {
    TACOp op;
    std::string result;  // destination
    std::string arg1;    // left operand
    std::string arg2;    // right operand

    std::string toString() const {
        switch (op) {
            case TACOp::ADD: return result + " = " + arg1 + " + " + arg2;
            case TACOp::SUB: return result + " = " + arg1 + " - " + arg2;
            case TACOp::MUL: return result + " = " + arg1 + " * " + arg2;
            case TACOp::DIV: return result + " = " + arg1 + " / " + arg2;
            case TACOp::JMP: return "goto " + result;
            case TACOp::JMP_IF_TRUE: return "if " + arg1 + " goto " + result;
            case TACOp::LABEL: return result + ":";
            case TACOp::RETURN: return "return " + arg1;
            // ... etc
        }
        return "";
    }
};
```

## 5.4 Converting AST to TAC

```cpp
class TACGenerator {
    std::vector<TACInstr> instructions;
    int tempCounter = 0;
    int labelCounter = 0;

    std::string newTemp() { return "%t" + std::to_string(tempCounter++); }
    std::string newLabel() { return "L" + std::to_string(labelCounter++); }

public:
    std::string genExpr(ASTNode* node) {
        if (auto* intExpr = dynamic_cast<IntExpr*>(node)) {
            std::string t = newTemp();
            instructions.push_back({TACOp::MOV, t, std::to_string(intExpr->value), ""});
            return t;
        }
        if (auto* var = dynamic_cast<VariableExpr*>(node)) {
            std::string t = newTemp();
            instructions.push_back({TACOp::LOAD, t, var->name, ""});
            return t;
        }
        if (auto* bin = dynamic_cast<BinaryExpr*>(node)) {
            std::string lhs = genExpr(bin->lhs.get());
            std::string rhs = genExpr(bin->rhs.get());
            std::string t = newTemp();
            TACOp op;
            switch (bin->op) {
                case BinaryExpr::ADD: op = TACOp::ADD; break;
                case BinaryExpr::SUB: op = TACOp::SUB; break;
                case BinaryExpr::MUL: op = TACOp::MUL; break;
                case BinaryExpr::DIV: op = TACOp::DIV; break;
                case BinaryExpr::EQ:  op = TACOp::EQ; break;
                case BinaryExpr::LT:  op = TACOp::LT; break;
                // ...
            }
            instructions.push_back({op, t, lhs, rhs});
            return t;
        }
        if (auto* assign = dynamic_cast<AssignExpr*>(node)) {
            std::string val = genExpr(assign->value.get());
            instructions.push_back({TACOp::STORE, assign->name, val, ""});
            return assign->name;
        }
        return "";
    }

    void genStmt(ASTNode* node) {
        if (auto* exprStmt = dynamic_cast<ExprStmt*>(node)) {
            genExpr(exprStmt->expr.get());
        }
        else if (auto* ifStmt = dynamic_cast<IfStmt*>(node)) {
            std::string cond = genExpr(ifStmt->condition.get());
            std::string elseLabel = newLabel();
            std::string endLabel = newLabel();

            instructions.push_back({TACOp::JMP_IF_FALSE, elseLabel, cond, ""});
            genStmt(ifStmt->thenBranch.get());
            instructions.push_back({TACOp::JMP, endLabel, "", ""});
            instructions.push_back({TACOp::LABEL, elseLabel, "", ""});
            if (ifStmt->elseBranch)
                genStmt(ifStmt->elseBranch.get());
            instructions.push_back({TACOp::LABEL, endLabel, "", ""});
        }
        else if (auto* whileStmt = dynamic_cast<WhileStmt*>(node)) {
            std::string startLabel = newLabel();
            std::string endLabel = newLabel();

            instructions.push_back({TACOp::LABEL, startLabel, "", ""});
            std::string cond = genExpr(whileStmt->condition.get());
            instructions.push_back({TACOp::JMP_IF_FALSE, endLabel, cond, ""});
            genStmt(whileStmt->body.get());
            instructions.push_back({TACOp::JMP, startLabel, "", ""});
            instructions.push_back({TACOp::LABEL, endLabel, "", ""});
        }
    }

    std::vector<TACInstr> getInstructions() { return instructions; }
};
```

## 5.5 Control Flow Graph (CFG)

A **Control Flow Graph** organizes TAC instructions into **basic blocks** connected by control flow edges.

### Basic Block

A **basic block** is a maximal sequence of instructions where:
- Execution enters only at the first instruction.
- Execution exits only at the last instruction.

```cpp
struct BasicBlock {
    std::string label;
    std::vector<TACInstr> instructions;
    std::vector<BasicBlock*> successors;
    std::vector<BasicBlock*> predecessors;
};

class CFGBuilder {
public:
    std::vector<BasicBlock*> build(const std::vector<TACInstr>& instructions) {
        // Step 1: Find leaders (first instruction of each basic block)
        // Leaders are: first instruction, targets of jumps, and instructions after jumps
        // Step 2: Group instructions into blocks from leader to next leader
        // Step 3: Add edges based on jumps
        std::vector<BasicBlock*> blocks;
        // ... implementation
        return blocks;
    }
};
```

```
CFG for: if (x > 0) y = 1; else y = 2; return y;

    ┌──────────┐
    │  Entry    │
    │ t1 = x>0  │
    └─────┬─────┘
          │
    ┌─────▼──────┐
    │ if t1 goto │──── true ──► ┌──────────┐
    │   else     │              │  y = 1    │
    └─────┬──────┘              └─────┬─────┘
          │ false                      │
    ┌─────▼──────┐                     │
    │  y = 2     │◄────────────────────┘
    └─────┬──────┘
          │
    ┌─────▼──────┐
    │ return y   │
    └────────────┘
```

## 5.6 Static Single Assignment (SSA) Form

**SSA** is a property of IR where each variable is assigned exactly once. This simplifies many analyses and optimizations.

```
Before SSA:            After SSA:
  x = 1                  x1 = 1
  y = x + 2              y1 = x1 + 2
  x = 3                  x2 = 3
  z = x + 4              z1 = x2 + 4
```

### Phi Functions

When control flow merges, **φ (phi) functions** select the correct value:

```
  if (cond)           x1 = 1
    x = 1      →      goto merge
  else           x2 = 2
    x = 2             goto merge
  use(x)         merge:
                      x3 = φ(x1, x2)
                      use(x3)
```

### SSA Construction Algorithm

1. **Insert φ-functions** at join points using dominance frontiers.
2. **Rename variables** to ensure single-assignment property.

```cpp
class SSAConstructor {
    DominatorTree domTree;
    std::map<BasicBlock*, std::set<std::string>> phiNeeded;

public:
    void insertPhis(CFG& cfg) {
        // For each variable v:
        //   1. Find all blocks that define v
        //   2. Compute iterated dominance frontier
        //   3. Insert φ for v at each block in the frontier
    }

    void renameVariables(CFG& cfg) {
        // Walk dominator tree in preorder
        // Maintain a stack of current version for each variable
        // Rename definitions and uses
    }
};
```

## 5.7 LLVM IR — A Production IR

LLVM IR is SSA-based, strongly typed, and designed for both high-level analysis and low-level optimization. We'll explore it in depth in Chapter 7, but here's a preview:

```llvm
define i32 @factorial(i32 %n) {
entry:
  %cmp = icmp sle i32 %n, 1
  br i1 %cmp, label %return, label %recurse

recurse:
  %sub = sub i32 %n, 1
  %call = call i32 @factorial(i32 %sub)
  %mul = mul i32 %n, %call
  ret i32 %mul

return:
  ret i32 1
}
```

## 5.8 Summary

- IR bridges the frontend and backend, enabling language and target independence.
- Three-Address Code is a simple, linear IR suitable for teaching and small compilers.
- Control Flow Graphs organize instructions into basic blocks for analysis.
- SSA form simplifies optimization by ensuring single-definition semantics.
- LLVM IR is a production-quality SSA-based IR used by Clang, Rust, Swift, and many others.

---

*Next: [Chapter 6: Introduction to LLVM](./06-introduction-to-llvm.md)*
