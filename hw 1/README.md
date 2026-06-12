# Technical Documentation: The p0 Compiler Project

Welcome to the official technical documentation for the **p0 compiler**. This document outlines the architecture, grammar specification, implementation details, and execution mechanics of our minimalist, recursive-descent compiler.

The p0 compiler translates a structured, high-level subset of the C language into a virtual **Stack Machine Intermediate Representation (IR)** / Assembly language.

---

## 1. Architectural Overview

The p0 compiler is built using a classic monolithic frontend architecture consisting of three tightly integrated components:

* **The Lexical Analyzer (Lexer):** Tokenizes raw source code character streams into discrete, syntactically meaningful units (`TOKEN_ID`, `TOKEN_NUM`, `TOKEN_WHILE`, etc.).
* **The Symbol Table Manager:** Tracks identifiers, variables, scopes, and memory references. It handles block scoping by distinguishing between fixed globals and stack-allocated locals.
* **The Recursive-Descent Parser & Code Generator:** Enforces structural grammar rules while emitting virtual assembly instructions on the fly (single-pass compilation).

---

## 2. EBNF Grammar Specification

The syntax of the language processed by the p0 compiler is formally defined by the following Extended Backus-Naur Form (EBNF) grammar rules:

```ebnf
Program      ::= ( GlobalDecl | FunctionDecl )*
GlobalDecl   ::= "int" Identifier ";"
FunctionDecl ::= "int" Identifier "(" ")" "{" LocalDecl* Stmt* "}"
LocalDecl    ::= "int" Identifier ";"
Stmt         ::= WhileStmt | AssignStmt | ReturnStmt | BlockStmt
WhileStmt    ::= "while" "(" Expr ")" Stmt
AssignStmt   ::= Identifier "=" Expr ";"
ReturnStmt   ::= "return" Expr ";"
BlockStmt    ::= "{" Stmt* "}"
Expr         ::= Term ( ( "+" | "-" | "==" ) Term )*
Term         ::= Factor ( ( "*" | "/" ) Factor )*
Factor       ::= Number | Identifier | Identifier "(" Args? ")" | "(" Expr ")"
Args         ::= Expr ( "," Expr )*

```

---

## 3. Core Feature Implementation Deep Dives

### A. The `while` Loop Control Flow

The `while` loop uses conditional execution and backward looping via unique assembly labels.

#### Logic Flow:

1. **Label Generation:** The parser dynamically requests two unique labels (`start_lbl` and `end_lbl`) from the state generator.
2. **Condition Assessment:** The code block kicks off with the entry label. The expression engine evaluates the condition and pushes the outcome to the runtime stack.
3. **Conditional Branching:** The `JZ` (Jump if Zero) instruction inspects the stack. If the condition evaluates to `0` (False), execution branches to the loop exit label.
4. **Statement Body Evaluation:** The inner statements run sequentially.
5. **Re-evaluation Loop:** An unconditional `JMP` points straight back to the entry label to re-verify the state of the expression.

```
   +--> L_START:
   |      Evaluate Condition
   |      JZ L_END  -------------------+
   |      Execute Loop Body Statements |
   +----  JMP L_START                  |
        L_END: <-----------------------+

```

---

### B. Function Calls & Activation Records

The compiler implements function execution frames utilizing an explicit runtime call stack.

```
High Memory
+-----------------------+
|  Arguments (Optional) |  <- Pushed by Caller
+-----------------------+
|  Return Address       |  <- Pushed automatically by CALL instruction
+-----------------------+
|  Saved Frame Pointer  |  <- Pushed by Prologue (Old BP)
+-----------------------+  <-- Current Base Pointer (BP)
|  Local Variable 1     |  <- Subtracted from SP during declaration
+-----------------------+
|  Local Variable 2     |  <- [BP - 8]
+-----------------------+  <-- Current Stack Pointer (SP)
Low Memory

```

#### The Function Workflow Lifecycle:

* **Argument Evaluation:** Arguments are evaluated from left to right, pushing results sequentially onto the stack.
* **The Invitation (`CALL`):** The `CALL` instruction transfers control to the function subroutine label while storing the program counter register as the return destination.
* **The Function Prologue:** Every function sets up its execution framework securely:
```nasm
PUSH BP        ; Save the caller's frame orientation base
MOV BP, SP     ; Establish our current activation baseline frame
SUB SP, N      ; Dynamically reserve byte space for local structures

```


* **The Function Epilogue & Return:** Upon reaching a `return` token, the evaluation value is stored in the Accumulator (`AX`) register. The frame is then safely taken down:
```nasm
MOV SP, BP     ; Drop all internal local variable space allocations
POP BP         ; Restore original parent calling environment
RET            ; Bounce control back to the original caller address

```



---

### C. Variable Lookup Strategy & Scoping

The p0 compiler handles scoping using a **Lexical Stack Lookup** mechanism within the symbol table.

| Feature / Property | Local Scope Variables | Global Scope Variables |
| --- | --- | --- |
| **Storage Area** | Runtime Thread Call Stack Frame | Fixed Static Memory Region |
| **Lookup Order** | Checked **First** (Shadows Globals) | Checked **Second** (Fallback) |
| **Addressing Method** | Base Pointer Relative Offset (`[BP - offset]`) | Symbolic Direct Reference (`[variable_name]`) |
| **Life Cycle** | Transient (Destroyed when scope exits) | Persistent (Lasts for the whole run) |

#### Resolving Offsets:

When an identifier is parsed, the symbol table is searched backwards (from the newest entries to the oldest).

* If found in **local scope**, its address is calculated via an offset below the base pointer (`[BP - offset]`), because the stack grows downwards.
* If found in **global scope**, it is referenced directly via its global identifier name.

---

## 4. Virtual Instruction Set Architecture (ISA)

The p0 compiler outputs instructions designed for a virtual stack machine. Below is the behavioral reference table for the emitted code:

| Instruction | Arguments | Operation Description |
| --- | --- | --- |
| `PUSH` | `Value / Reg` | Push a constant literal value or register contents onto the stack frame pointer. |
| `POP` | `Register` | Pop the top item off the stack and store it in the specified target register. |
| `LOAD` | `[Address]` | Copy data from a specified local offset or global memory location onto the stack. |
| `STORE` | `[Address]` | Pop the top value off the stack and write it to a local offset or global variable. |
| `ADD` / `SUB` | None | Pop the top two stack values, compute the result, and push it back onto the stack. |
| `CMPEQ` | None | Pop the top two stack values. Push `1` if they are equal, or `0` if they are not. |
| `JMP` | `Label` | Unconditionally jump to the target code label. |
| `JZ` | `Label` | Pop the top stack value. If it is `0` (False), jump to the specified label. |
| `CALL` | `Subroutine` | Push the current return address onto the stack and branch to the function. |
| `RET` | None | Pop the return address off the stack and jump back to the caller. |