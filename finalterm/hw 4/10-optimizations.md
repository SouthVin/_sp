# Chapter 10: Compiler Optimizations

## 10.1 The Role of Optimization

Compiler optimizations transform a program into a semantically equivalent but more efficient form. Efficiency can mean:

- **Speed** — fewer instructions executed, better cache/pipeline utilization
- **Size** — smaller binary footprint
- **Energy** — fewer operations, less memory access
- **Security** — hardening against exploits (stack canaries, CFI)

LLVM optimizations work exclusively on LLVM IR, making them language- and target-independent.

## 10.2 The Optimization Hierarchy

```
Module-level optimizations (interprocedural)
    └──► CGSCC-level (call graph SCC)
         └──► Function-level optimizations
              └──► Loop-level optimizations
                   └──► Basic block / peephole optimizations
```

## 10.3 The New Pass Manager

LLVM 17+ uses the **new pass manager** (NPM). Passes are organized into pipelines:

```cpp
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

void optimizeFunction(llvm::Function& F) {
    llvm::FunctionAnalysisManager FAM;
    llvm::FunctionPassManager FPM;

    // Register analyses
    llvm::PassBuilder PB;
    PB.registerFunctionAnalyses(FAM);

    // Add passes
    FPM.addPass(llvm::InstCombinePass());   // Peephole optimizations
    FPM.addPass(llvm::SimplifyCFGPass());   // Clean up control flow
    FPM.addPass(llvm::GVNPass());           // Global Value Numbering
    FPM.addPass(llvm::SimplifyCFGPass());   // Clean up again after GVN

    // Run the pipeline
    FPM.run(F, FAM);
}
```

### Default Optimization Pipelines

```cpp
llvm::PassBuilder PB(targetMachine);
llvm::ModulePassManager MPM;

// -O0: no optimization
MPM = PB.buildO0DefaultPipeline(llvm::OptimizationLevel::O0);

// -O1: conservative optimization
MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O1);

// -O2: standard optimization (good balance)
MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);

// -O3: aggressive optimization (vectorization, etc.)
MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);

// -Os: optimize for size
MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::Os);

// -Oz: aggressive size optimization
MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::Oz);

// Run the pipeline
MPM.run(*module, MAM);
```

## 10.4 Key Optimization Passes

### 10.4.1 Dead Code Elimination (DCE)

Removes instructions whose results are never used.

```
Before:                     After:
  %x = add i32 %a, %b       (removed — %x is dead)
  %y = mul i32 %c, %d       %y = mul i32 %c, %d
  ret i32 %y                ret i32 %y
```

```cpp
FPM.addPass(llvm::DCEPass());
```

### 10.4.2 Constant Folding and Propagation

Evaluates constant expressions at compile time and propagates constant values.

```
Before:                     After:
  %a = add i32 3, 4         %a = i32 7
  %b = mul i32 %a, 2        %b = mul i32 7, 2     → %b = 14
```

```cpp
FPM.addPass(llvm::InstCombinePass());
FPM.addPass(llvm::SCCPPass());  // Sparse Conditional Constant Propagation
```

### 10.4.3 Common Subexpression Elimination (CSE) / GVN

Eliminates redundant computations that produce the same value.

```
Before:                     After:
  %t1 = add i32 %a, %b      %t1 = add i32 %a, %b
  %t2 = add i32 %a, %b      %t2 = %t1              ; reused
  %t3 = add i32 %t1, %t2    %t3 = add i32 %t1, %t1 ; also reused
```

```cpp
FPM.addPass(llvm::GVNPass());       // Global Value Numbering
FPM.addPass(llvm::NewGVNPass());    // New GVN (more powerful)
```

### 10.4.4 Loop Optimizations

#### Loop Invariant Code Motion (LICM)

Moves computations that don't change per iteration outside the loop.

```
Before:                       After:
  for i = 0..n:                 %inv = load @readonly_var
    x = inv * 2                 for i = 0..n:
    use(x + i)                    use(%inv * 2 + i)
```

```cpp
FPM.addPass(llvm::LICMPass());
```

#### Loop Unrolling

Duplicates the loop body to reduce branch overhead and enable further optimizations.

```
Before:                       After (unroll factor 2):
  for i = 0, i < n, i++:        for i = 0, i < n, i += 2:
    body(i)                        body(i)
                                   body(i+1)
```

```cpp
// Controlled via loop metadata or pass parameters
```

#### Loop Vectorization

Converts scalar loop operations to SIMD vector operations.

```
Before:                       After:
  for i = 0..255:               %vec = load <4 x float>, ptr
    c[i] = a[i] + b[i]          %res = fadd <4 x float>, ...
                                 store <4 x float> %res, ptr
```

```cpp
FPM.addPass(llvm::LoopVectorizePass());
FPM.addPass(llvm::SLPVectorizerPass());  // Superword-Level Parallelism
```

#### Loop Fusion and Distribution

- **Fusion**: Combine adjacent loops with the same iteration space.
- **Distribution**: Split a loop into multiple loops to enable other optimizations.

```cpp
FPM.addPass(llvm::LoopFusePass());
FPM.addPass(llvm::LoopDistributePass());
```

### 10.4.5 Function Inlining

Replaces a function call with the function's body:

```
Before:                       After:
  define i32 @square(i32 %x)    define i32 @calc(i32 %a) {
    mul i32 %x, %x                %result = mul i32 %a, %a
  define i32 @calc(i32 %a) {      ret i32 %result
    call @square(%a)            }
  }
```

```cpp
FPM.addPass(llvm::InlinerPass());
```

### 10.4.6 Tail Call Elimination

Converts tail-recursive calls into loops:

```
Before:                       After:
  define i32 @fact(i32 %n) {    define i32 @fact(i32 %n) {
    if n <= 1: ret 1              %result = 1
    ret n * fact(n-1)             ...
  }                               br label %loop
                                }
```

### 10.4.7 Control Flow Simplification

Simplifies the control flow graph:

```cpp
FPM.addPass(llvm::SimplifyCFGPass());
```

- Removes unreachable basic blocks
- Merges consecutive branches to the same destination
- Converts conditional branch + unconditional branch → conditional branch to final destination
- Eliminates empty basic blocks

## 10.5 Interprocedural Optimizations (IPO)

These work across function boundaries:

```cpp
// Add to module-level pass manager
MPM.addPass(llvm::InlinerPass());
MPM.addPass(llvm::GlobalOptPass());       // Global variable optimization
MPM.addPass(llvm::IPSCCPPass());          // Interprocedural SCCP
MPM.addPass(llvm::GlobalDCEPass());       // Dead global elimination
MPM.addPass(llvm::DeadArgElimPass());     // Remove unused function arguments
```

## 10.6 Target-Specific Optimizations

After IR-level optimization, target-specific optimizations include:

| Optimization | Description |
|-------------|-------------|
| **Instruction Selection** | Choose the best target instructions for IR ops |
| **Peephole Optimizations** | Local pattern-based improvements |
| **Register Allocation** | Assign physical registers to virtual registers |
| **Instruction Scheduling** | Reorder to minimize pipeline stalls |
| **Code Layout** | Arrange basic blocks for better branch prediction |

## 10.7 Profile-Guided Optimization (PGO)

```cpp
// 1. Compile with instrumentation
// 2. Run with representative workload (generates .profraw)
// 3. Convert to .profdata with llvm-profdata
// 4. Recompile using profile data
```

## 10.8 Link-Time Optimization (LTO)

LTO enables cross-module optimization at link time:

```bash
# Full LTO
clang -flto -c file1.c file2.c
clang -flto -o program file1.o file2.o  # LTO happens here

# ThinLTO (scalable)
clang -flto=thin -c file1.c file2.c
clang -flto=thin -o program file1.o file2.o
```

## 10.9 Writing Custom Optimization Passes

```cpp
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

struct MyAlgebraicOptPass : public llvm::PassInfoMixin<MyAlgebraicOptPass> {
    llvm::PreservedAnalyses run(llvm::Function& F,
                                llvm::FunctionAnalysisManager& AM) {
        bool changed = false;

        for (auto& BB : F) {
            for (auto& I : BB) {
                // Match: x * 1 → x
                if (auto* mul = llvm::dyn_cast<llvm::BinaryOperator>(&I)) {
                    if (mul->getOpcode() == llvm::Instruction::Mul) {
                        if (auto* C = llvm::dyn_cast<llvm::ConstantInt>(
                                mul->getOperand(1))) {
                            if (C->isOne()) {
                                mul->replaceAllUsesWith(mul->getOperand(0));
                                mul->eraseFromParent();
                                changed = true;
                            }
                        }
                    }
                }
            }
        }

        return changed ? llvm::PreservedAnalyses::none()
                       : llvm::PreservedAnalyses::all();
    }
};

// Register the pass with the pass builder
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "MyAlgebraicOpt", "v0.1",
            [](llvm::PassBuilder& PB) {
                PB.registerPipelineParsingCallback(
                    [](llvm::StringRef Name, llvm::FunctionPassManager& FPM,
                       llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                        if (Name == "my-algebraic-opt") {
                            FPM.addPass(MyAlgebraicOptPass());
                            return true;
                        }
                        return false;
                    });
            }};
}

// Load with: opt -load-pass-plugin=./MyPass.so -passes=my-algebraic-opt
```

## 10.10 Performance Tuning with LLVM

```bash
# See what passes -O2 runs
clang -O2 -mllvm -debug-pass=Arguments file.c

# Analyze performance
clang -O2 -g -pg file.c     # gprof
clang -O2 -g -fprofile-instr-generate file.c  # PGO

# Print IR before/after each pass
opt -passes=instcombine -print-before-all -print-after-all input.ll
```

## 10.11 Summary

- LLVM optimizes at module, CGSCC, function, loop, and basic block levels.
- Key passes: DCE, InstCombine, GVN, LICM, LoopVectorize, Inliner, SimplifyCFG.
- The new pass manager enables composable, pipeline-based optimization.
- Custom passes can be written as plugins.
- LTO and PGO provide additional optimization opportunities.

---

*Next: [Chapter 11: Building a Complete Compiler](./11-complete-compiler.md)*
