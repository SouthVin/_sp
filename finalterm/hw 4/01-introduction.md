# Chapter 1: Introduction to Compilers

## 1.1 What Is a Compiler?

A **compiler** is a program that translates source code written in a high-level programming language into machine code or another lower-level representation. More formally: a compiler reads a program in a *source language*, analyzes it, and produces an equivalent program in a *target language*.

```
Source Program  ──► [ Compiler ] ──► Target Program
                          │
                    Error Messages
```

## 1.2 Why Study Compilers?

- **Understand how languages work** — Knowing how compilers parse and analyze code makes you a better programmer.
- **Build domain-specific languages (DSLs)** — Custom languages for specific problem domains.
- **Master program analysis** — Static analysis, optimization, and verification techniques apply broadly.
- **Appreciate language design** — Learn trade-offs between expressiveness, safety, and performance.

## 1.3 The Compilation Pipeline

A typical compiler proceeds through the following phases:

### Phase 1: Frontend
| Phase | Input | Output | Description |
|-------|-------|--------|-------------|
| **Lexical Analysis** | Source text | Tokens | Groups characters into meaningful tokens |
| **Syntax Analysis** | Tokens | Abstract Syntax Tree (AST) | Builds tree structure per grammar rules |
| **Semantic Analysis** | AST | Decorated AST | Type checking, scoping, meaning verification |

### Phase 2: Middle-End
| Phase | Input | Output | Description |
|-------|-------|--------|-------------|
| **IR Generation** | Decorated AST | Intermediate Representation | Language-independent representation |
| **Optimization** | IR | Optimized IR | Platform-independent optimizations |

### Phase 3: Backend
| Phase | Input | Output | Description |
|-------|-------|--------|-------------|
| **Instruction Selection** | Optimized IR | Assembly-like IR | Maps IR to target machine instructions |
| **Register Allocation** | Assembly-like IR | Colored IR | Assigns physical registers to virtual ones |
| **Code Emission** | Colored IR | Machine code / assembly | Emits final binary or assembly |

```
Source Code
    │
    ▼
┌──────────────────┐
│  Lexical Analyzer │  ◄── Scanner / Tokenizer
└────────┬─────────┘
         │ token stream
         ▼
┌──────────────────┐
│  Syntax Analyzer  │  ◄── Parser
└────────┬─────────┘
         │ AST
         ▼
┌──────────────────┐
│ Semantic Analyzer │  ◄── Type checker, symbol table
└────────┬─────────┘
         │ decorated AST
         ▼
┌──────────────────┐
│  IR Generator     │
└────────┬─────────┘
         │ IR
         ▼
┌──────────────────┐
│   Optimizer       │
└────────┬─────────┘
         │ optimized IR
         ▼
┌──────────────────┐
│  Code Generator   │  ◄── Instruction selection, register allocation
└────────┬─────────┘
         │
         ▼
    Object Code
```

## 1.4 Interpreters vs. Compilers

| | Compiler | Interpreter |
|---|---|---|
| **Execution** | Translates entire program before execution | Translates and executes line-by-line |
| **Output** | Produces standalone executable | No separate output file |
| **Speed** | Faster execution | Slower execution |
| **Examples** | GCC, Clang, Rustc | Python (CPython), Ruby (MRI), JavaScript (V8 does both!) |

Modern language runtimes often blend both: **Just-In-Time (JIT)** compilation compiles hot code paths at runtime.

## 1.5 A Brief History of LLVM

LLVM began as a research project by Chris Lattner at the University of Illinois in 2000. Originally standing for "Low Level Virtual Machine", LLVM has evolved into a comprehensive compiler infrastructure:

- **2003**: First public release of LLVM 1.0
- **2005**: Apple begins contributing, later adopts LLVM for macOS/iOS toolchains
- **2007**: Clang frontend development begins
- **2012**: LLVM 3.1 — mature C/C++/ObjC support
- **2019**: LLVM 9 — official monorepo migration
- **2023+**: LLVM 17+, opaque pointer migration, new pass manager default

LLVM's key innovation is its **language-agnostic intermediate representation** (LLVM IR) that serves as the interface between frontends and backends. Write one frontend for your language, and LLVM handles optimization and code generation for dozens of target architectures.

## 1.6 Toolchain Overview

```
┌──────────┐  ┌──────────┐  ┌──────────┐
│  Clang   │  │  Rustc   │  │  Swiftc  │  ... many frontends
│ (C/C++)  │  │  (Rust)  │  │  (Swift) │
└────┬─────┘  └────┬─────┘  └────┬─────┘
     │              │              │
     ▼              ▼              ▼
┌─────────────────────────────────────────┐
│              LLVM IR                     │
├─────────────────────────────────────────┤
│          LLVM Optimizer                  │
├─────────────────────────────────────────┤
│      LLVM Backend (x86, ARM, ...)        │
└─────────────────────────────────────────┘
```

## 1.7 Setting Up Your Environment

```bash
# Ubuntu/Debian
sudo apt install llvm-17-dev clang-17 cmake build-essential

# macOS (Homebrew)
brew install llvm@17 cmake

# Verify installation
clang-17 --version
llvm-config-17 --version
```

### Basic CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyCompiler)

find_package(LLVM 17 REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(mycompiler main.cpp)
llvm_map_components_to_libnames(llvm_libs core irreader support)
target_link_libraries(mycompiler ${llvm_libs})
```

## 1.8 Summary

- A compiler translates source code through a multi-phase pipeline: scanning → parsing → semantic analysis → IR generation → optimization → code generation.
- LLVM provides reusable, production-quality components for the middle-end and backend.
- Understanding compilers deepens your knowledge of how programming languages really work.

---

*Next: [Chapter 2: Lexical Analysis](./02-lexical-analysis.md)*
