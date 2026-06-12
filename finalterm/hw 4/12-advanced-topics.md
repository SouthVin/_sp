# Chapter 12: Advanced Topics & Future Directions

## 12.1 Garbage Collection Integration

Many modern languages (Java, Go, Haskell) require garbage collection. LLVM supports GC via:

- **GC strategy plugins** — custom GC implementations
- **Stack map generation** — for precise root finding
- **Read/write barriers** — for concurrent GC

```cpp
// Declare a GC strategy
func->setGC("my-gc");

// Generate GC roots
llvm::Value* root = builder->CreateAlloca(type);
// LLVM tracks these as GC roots

// Statepoint intrinsics for cooperative GC
llvm::CallInst* call = builder->CreateCall(...);
// Wrap with gc.statepoint for safe points
```

## 12.2 Exception Handling

LLVM supports both Itanium C++ ABI (DWARF-based) and Windows SEH exceptions.

```llvm
define void @may_throw() personality ptr @__gxx_personality_v0 {
entry:
  %result = invoke i32 @risky_function()
      to label %continue unwind label %catch

continue:
  ret void

catch:
  %lp = landingpad {ptr, i32}
      catch ptr @type_info_for_exception
  ; Extract exception pointer and selector
  %exn = extractvalue {ptr, i32} %lp, 0
  %sel = extractvalue {ptr, i32} %lp, 1
  ; ... handle exception
  ret void
}
```

```cpp
// C++ API
auto* normalDest = llvm::BasicBlock::Create(ctx, "normal", func);
auto* unwindDest = llvm::BasicBlock::Create(ctx, "unwind", func);

builder->CreateInvoke(callee, normalDest, unwindDest, args);
```

## 12.3 Coroutines

LLVM supports coroutines via a set of intrinsics and a transformation pass:

```llvm
define ptr @my_coro(i32 %n) {
entry:
  %id = call token @llvm.coro.id(...)
  %size = call i32 @llvm.coro.size.i32()
  %alloc = call ptr @malloc(i32 %size)
  %hdl = call ptr @llvm.coro.begin(token %id, ptr %alloc)

  ; ... coroutine body with suspend/resume points ...

  %save = call i8 @llvm.coro.suspend(token %save_token, i1 false)
  switch i8 %save, label %suspend [i8 0, label %resume, i8 1, label %cleanup]

suspend:
  call void @llvm.coro.end(ptr %hdl, i1 false)
  ret ptr %hdl

resume:
  ; ... continue execution ...

cleanup:
  ; ... clean up and destroy ...
}
```

## 12.4 GPU and Heterogeneous Compilation

LLVM supports compilation for GPUs through backends and language extensions:

### Supported GPU Architectures
- **AMDGPU** — AMD RDNA/CDNA GPUs (OpenCL, HIP)
- **NVPTX** — NVIDIA CUDA GPUs
- **SPIR-V** — Vulkan compute shaders

```cpp
// Target triples for GPU
"amdgcn-amd-amdhsa"     // AMD HSA runtime
"nvptx64-nvidia-cuda"   // NVIDIA 64-bit CUDA

// Address spaces in LLVM IR
@buffer = addrspace(1) global [1024 x float] zeroinitializer  // global memory
float addrspace(3)* @shared  // shared memory (LDS)
float addrspace(5)* @private  // private (register)
```

## 12.5 WebAssembly

Compile to WebAssembly (WASM) to run in browsers or standalone runtimes:

```bash
clang --target=wasm32-unknown-unknown -nostdlib -Wl,--no-entry \
      -Wl,--export-all -o output.wasm input.c
```

```cpp
module->setTargetTriple("wasm32-unknown-unknown");
// WASM-specific: tables, memory, imports/exports
```

## 12.6 Source-Level Debugging

Full debug information for interactive debugging:

```cpp
llvm::DIBuilder diBuilder(*module);

// Create file and compile unit
auto* file = diBuilder.createFile("test.tl", ".");
auto* cu = diBuilder.createCompileUnit(
    llvm::dwarf::DW_LANG_C, file, "TinyLang", true, "", 0);

// Basic type
auto* intType = diBuilder.createBasicType("int", 32, llvm::dwarf::DW_ATE_signed);

// Function with debug info
auto* subprogram = diBuilder.createFunction(
    file, funcName, linkageName, file, lineNum,
    diBuilder.createSubroutineType(diBuilder.getOrCreateTypeArray({})),
    scopeLine, llvm::DINode::FlagPrototyped,
    llvm::DISubprogram::SPFlagDefinition);
func->setSubprogram(subprogram);

// Push current scope
auto* lexicalBlock = diBuilder.createLexicalBlock(subprogram, file, lineNum, 0);
builder->SetCurrentDebugLocation(llvm::DILocation::get(
    *context, lineNum, colNum, subprogram));

// ... emit instructions ...

diBuilder.finalize();
```

## 12.7 Sanitizers and Program Analysis

LLVM provides runtime checking tools:

| Sanitizer | What It Detects | Flag |
|-----------|-----------------|------|
| AddressSanitizer (ASan) | Use-after-free, buffer overflow | `-fsanitize=address` |
| MemorySanitizer (MSan) | Uninitialized memory reads | `-fsanitize=memory` |
| ThreadSanitizer (TSan) | Data races | `-fsanitize=thread` |
| UndefinedBehaviorSanitizer (UBSan) | Integer overflow, null deref, etc. | `-fsanitize=undefined` |
| LeakSanitizer (LSan) | Memory leaks | (part of ASan) |

### Instrumentation via LLVM

```cpp
// Insert bounds check before every array access
void insertBoundsCheck(llvm::Value* pointer, llvm::Value* index,
                       llvm::Value* arraySize) {
    auto* inBounds = builder->CreateICmpULT(index, arraySize);
    auto* failBB = llvm::BasicBlock::Create(ctx, "bounds_fail", func);
    auto* contBB = llvm::BasicBlock::Create(ctx, "bounds_ok", func);

    builder->CreateCondBr(inBounds, contBB, failBB);

    builder->SetInsertPoint(failBB);
    // Call error handler
    builder->CreateCall(errorHandler);
    builder->CreateUnreachable();
    // ...
}
```

## 12.8 MLIR — Multi-Level Intermediate Representation

**MLIR** (built on LLVM) raises the abstraction level, enabling:

- High-level dialect-specific optimizations (e.g., tensor operations)
- Progressive lowering through multiple dialects
- Domain-specific compilers (ML, hardware, etc.)

```
TensorFlow/HLO ──► MHLO dialect ──► Linalg dialect ──► LLVM dialect ──► LLVM IR
NumPy/Python  ──► NumPy dialect ──┘
```

```mlir
// Example MLIR
func.func @matmul(%A: tensor<128x128xf32>, %B: tensor<128x128xf32>)
    -> tensor<128x128xf32> {
  %0 = linalg.matmul ins(%A, %B : tensor<128x128xf32>, tensor<128x128xf32>)
                     outs(%empty : tensor<128x128xf32>) -> tensor<128x128xf32>
  return %0 : tensor<128x128xf32>
}
```

## 12.9 Formal Verification of Compilers

Techniques for ensuring compiler correctness:

### CompCert
A formally verified C compiler (in Coq) that guarantees the compiled binary behaves exactly as specified by C semantics. It produces assembly for PowerPC, ARM, RISC-V, and x86.

### Alive2
An LLVM-based tool for formal verification of LLVM optimizations:

```bash
# Verify that an optimization is correct
alive-tv src.ll tgt.ll
```

### Translation Validation
After each optimization pass, verify the transformed IR is semantically equivalent to the input.

```bash
# Use Alive2 as a translation validator
opt -load-pass-plugin=./Alive2.so -passes=my-pass input.ll -S
```

## 12.10 Incremental Compilation

Modern language servers (LSP) require incremental compilation for instant feedback:

```cpp
// Maintain a "compilation database" of parsed modules
struct ModuleState {
    std::unique_ptr<llvm::Module> ir;
    std::map<std::string, llvm::Function*> functions;
    std::map<std::string, Type> globals;
};

// On file change:
// 1. Re-lex and re-parse only the changed region
// 2. Diff the new AST against the old AST
// 3. Only re-codegen the changed functions
// 4. Use an incremental linker to update the JIT'd code
```

## 12.11 Alternative Backend: Cranelift and QBE

While LLVM is the most mature compiler backend, alternatives exist:

| Backend | Speed | Optimization | Use Case |
|---------|-------|-------------|----------|
| **LLVM** | Slow to compile, fast output | Excellent | Production compilers |
| **Cranelift** | Fast compilation | Good | JIT, WASM runtimes |
| **QBE** | Fast compilation | Decent | Small/medium compilers |
| **C--** | Moderate | Basic | Teaching/experimentation |

### Example: QBE IR

```qbe
function w $add(w %a, w %b) {
@start
    %result =w add %a, %b
    ret %result
}
```

## 12.12 Rust Compiler Architecture (rustc)

As a case study of a production LLVM-based compiler:

```
Source (.rs) ──► Lexer ──► Parser ──► AST
                                      │
                                      ▼
                                 HIR (High-level IR)
                                      │
                         ┌────────────┼────────────┐
                         │ type checking           │ borrow checking
                         │ trait resolution        │ lifetime analysis
                         ▼                         ▼
                                 MIR (Mid-level IR)
                                      │
                         ┌────────────┼────────────┐
                         │ monomorphization        │ optimization
                         │ MIR optimizations       │
                         ▼                         │
                                 LLVM IR ◄─────────┘
                                      │
                                      ▼
                                 Machine Code
```

Key insight: Rust adds **multiple IR layers** between AST and LLVM IR, each enabling specific analyses.

## 12.13 Future Directions

### AI-Assisted Compilation
- ML-driven optimization heuristics (inlining decisions, register allocation)
- Learned cost models for instruction selection
- Automated bug finding in compiler passes

### Quantum Computing Compilers
- QIR (Quantum Intermediate Representation) based on LLVM
- Compilation for quantum-classical hybrid architectures

### Energy-Aware Compilation
- Optimize for minimal energy consumption, not just speed
- Specialized for battery-constrained devices (IoT, mobile)

### Verified and Secure Compilation
- Formal guarantees across the full pipeline
- Constant-time guarantees for cryptographic code
- Compiler-enforced security policies

## 12.14 Recommended Resources

### Books
- *Engineering a Compiler* — Cooper & Torczon
- *Compilers: Principles, Techniques, and Tools* (Dragon Book) — Aho, Lam, Sethi, Ullman
- *Modern Compiler Implementation* (Tiger Book) — Andrew Appel
- *Getting Started with LLVM Core Libraries* — Lopes & Auler

### Online Resources
- [LLVM Documentation](https://llvm.org/docs/)
- [LLVM Language Reference Manual](https://llvm.org/docs/LangRef.html)
- [Kaleidoscope Tutorial](https://llvm.org/docs/tutorial/)
- [Compiler Explorer (godbolt.org)](https://godbolt.org/)

## 12.15 Summary

- LLVM supports advanced features: garbage collection, exception handling, coroutines, and GPU compilation.
- Sanitizers and formal verification tools ensure correctness.
- MLIR extends LLVM's paradigm to higher abstraction levels.
- Architectural patterns from rustc demonstrate multi-IR compiler design.
- The future of compilers includes AI assistance, formal verification, and new hardware targets.

---

*This concludes the book. Thank you for reading — now go build a compiler!*
