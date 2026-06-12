# Chapter 6: Introduction to LLVM

## 6.1 What Is LLVM?

**LLVM** is a collection of modular and reusable compiler and toolchain technologies. Despite its name (originally "Low Level Virtual Machine"), it is now a full compiler infrastructure, not a virtual machine. LLVM provides:

- A language-agnostic **intermediate representation** (LLVM IR)
- A powerful **optimizer** framework
- **Code generation** for multiple target architectures
- **JIT compilation** capabilities
- **Debug info** generation (DWARF, PDB)
- Tools for static analysis, instrumentation, and more

## 6.2 LLVM's Modular Architecture

```
┌─────────────────────────────────────────────────────────┐
│                     LLVM Ecosystem                       │
├───────────┬───────────┬───────────┬─────────────────────┤
│  Clang    │   LLD     │   LLDB    │  compiler-rt / libc++ │
│ (C/C++/   │ (Linker)  │ (Debugger)│  (Runtime libraries) │
│  ObjC)    │           │           │                      │
├───────────┴───────────┴───────────┴─────────────────────┤
│                    LLVM Core Libraries                   │
├───────────┬───────────┬───────────┬─────────────────────┤
│   IR      │ Optimizer │  Backend  │   Support/ADT        │
│ (Value,   │  (Passes) │(Selection │ (StringRef,          │
│  Type,    │           │ DAG, Reg  │  SmallVector, etc.)  │
│  Module)  │           │ Alloc)    │                      │
└───────────┴───────────┴───────────┴─────────────────────┘
```

## 6.3 Core LLVM Concepts

### Module

The top-level container for a translation unit. A `Module` holds functions, global variables, and metadata.

```cpp
llvm::LLVMContext context;
auto module = std::make_unique<llvm::Module>("my_module", context);
```

### Function

A function contains basic blocks and arguments.

```cpp
llvm::FunctionType* funcType = llvm::FunctionType::get(
    llvm::Type::getInt32Ty(context),   // return type
    {llvm::Type::getInt32Ty(context)}, // parameter types
    false                              // not variadic
);
llvm::Function* func = llvm::Function::Create(
    funcType,
    llvm::Function::ExternalLinkage,
    "my_function",
    module.get()
);
```

### Basic Block

A sequence of instructions with a single entry and single exit. Terminates with a terminator instruction (branch, return, etc.).

```cpp
llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
llvm::BasicBlock* loop = llvm::BasicBlock::Create(context, "loop", func);
```

### Instruction

The units of computation: arithmetic, memory access, control flow, etc.

```cpp
llvm::IRBuilder<> builder(context);

builder.SetInsertPoint(entry);
llvm::Value* sum = builder.CreateAdd(arg1, arg2, "sum");
builder.CreateCondBr(condition, loop, exit_block);
```

### Value

The base class for everything that can be used as an operand: instructions, constants, function arguments, global variables.

### Type System

LLVM has a rich type system:

| LLVM Type | C++ Representation | Description |
|-----------|-------------------|-------------|
| `i1` | `Type::getInt1Ty(ctx)` | 1-bit integer (boolean) |
| `i8` | `Type::getInt8Ty(ctx)` | 8-bit integer |
| `i32` | `Type::getInt32Ty(ctx)` | 32-bit integer |
| `i64` | `Type::getInt64Ty(ctx)` | 64-bit integer |
| `float` | `Type::getFloatTy(ctx)` | 32-bit IEEE float |
| `double` | `Type::getDoubleTy(ctx)` | 64-bit IEEE float |
| `void` | `Type::getVoidTy(ctx)` | No value |
| `ptr` | `PointerType::get(ctx, AS)` | Opaque pointer (LLVM 17+) |
| `[N x T]` | `ArrayType::get(T, N)` | Fixed-size array |
| `{T1, T2, ...}` | `StructType::get(ctx, {...})` | Structure type |

## 6.4 The IRBuilder

The `IRBuilder` is the primary API for generating LLVM IR programmatically. It manages the insertion point and provides convenience methods for creating instructions.

```cpp
llvm::LLVMContext context;
llvm::IRBuilder<> builder(context);
llvm::Module module("example", context);

// Create a function: int add(int a, int b)
auto* returnType = llvm::Type::getInt32Ty(context);
std::vector<llvm::Type*> paramTypes = {returnType, returnType};
auto* funcType = llvm::FunctionType::get(returnType, paramTypes, false);
auto* func = llvm::Function::Create(
    funcType, llvm::Function::ExternalLinkage, "add", &module);

// Create entry block
auto* entry = llvm::BasicBlock::Create(context, "entry", func);
builder.SetInsertPoint(entry);

// Get function arguments
auto args = func->args().begin();
llvm::Value* a = args++;
llvm::Value* b = args;
a->setName("a");
b->setName("b");

// Generate: return a + b;
llvm::Value* result = builder.CreateAdd(a, b, "sum");
builder.CreateRet(result);

// Print the generated IR
module.print(llvm::outs(), nullptr);
```

### Key IRBuilder Methods

```cpp
// Arithmetic
Value* CreateAdd(Value* L, Value* R, const Twine& Name = "")
Value* CreateSub(Value* L, Value* R, const Twine& Name = "")
Value* CreateMul(Value* L, Value* R, const Twine& Name = "")
Value* CreateSDiv(Value* L, Value* R, const Twine& Name = "")
Value* CreateFAdd(Value* L, Value* R, const Twine& Name = "")

// Memory
Value* CreateAlloca(Type* T, Value* ArraySize = nullptr, const Twine& Name = "")
Value* CreateLoad(Type* T, Value* Ptr, const Twine& Name = "")
Value* CreateStore(Value* Val, Value* Ptr)
Value* CreateGEP(Type* T, Value* Ptr, ArrayRef<Value*> Idx, const Twine& Name = "")

// Control flow
Value* CreateICmpEQ(Value* L, Value* R, const Twine& Name = "")
Value* CreateFCmpOEQ(Value* L, Value* R, const Twine& Name = "")
ReturnInst* CreateRet(Value* V = nullptr)
BranchInst* CreateBr(BasicBlock* Dest)
BranchInst* CreateCondBr(Value* Cond, BasicBlock* True, BasicBlock* False)
InvokeInst* CreateInvoke(...)
SwitchInst* CreateSwitch(Value* V, BasicBlock* Default, unsigned NumCases)

// Function calls
CallInst* CreateCall(FunctionType* FT, Value* Callee, ArrayRef<Value*> Args, const Twine& Name = "")

// Casts
Value* CreateBitCast(Value* V, Type* DestTy, const Twine& Name = "")
Value* CreateIntCast(Value* V, Type* DestTy, bool isSigned, const Twine& Name = "")
Value* CreateFPCast(Value* V, Type* DestTy, const Twine& Name = "")
Value* CreateSIToFP(Value* V, Type* DestTy, const Twine& Name = "")
Value* CreateFPToSI(Value* V, Type* DestTy, const Twine& Name = "")

// Other
Value* CreateSelect(Value* Cond, Value* True, Value* False, const Twine& Name = "")
PHINode* CreatePHI(Type* T, unsigned NumReservedValues, const Twine& Name = "")
```

## 6.5 Linking and Executing LLVM IR

### Running the Optimizer

```cpp
// Create a pass pipeline
llvm::FunctionPassManager fpm;
llvm::FunctionAnalysisManager fam;
// Register passes...

// Run on a function
fpm.run(*func, fam);
```

### JIT Execution

```cpp
// Create JIT compiler
auto jit = llvm::orc::LLJITBuilder().create();
jit->get()->addIRModule(
    llvm::orc::ThreadSafeModule(std::move(module), std::make_unique<llvm::LLVMContext>()));

// Look up and execute function
auto sym = jit->get()->lookup("add");
int (*add_func)(int, int) = sym->getAddress().toPtr<int(*)(int,int)>();
int result = add_func(3, 4);  // returns 7
```

### Emitting Object Code

```cpp
llvm::InitializeAllTargetInfos();
llvm::InitializeAllTargets();
llvm::InitializeAllTargetMCs();
llvm::InitializeAllAsmParsers();
llvm::InitializeAllAsmPrinters();

auto targetTriple = llvm::sys::getDefaultTargetTriple();
std::string error;
auto* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
auto* cpu = "generic";
auto* features = "";
llvm::TargetOptions opt;
auto rm = llvm::Optional<llvm::Reloc::Model>();
auto* targetMachine = target->createTargetMachine(
    targetTriple, cpu, features, opt, rm);

module->setDataLayout(targetMachine->createDataLayout());

std::error_code ec;
llvm::raw_fd_ostream dest("output.o", ec, llvm::sys::fs::OF_None);

llvm::legacy::PassManager pm;
targetMachine->addPassesToEmitFile(pm, dest, nullptr,
    llvm::CGFT_ObjectFile);
pm.run(*module);
```

## 6.6 LLVM Pass Infrastructure

LLVM uses a **pass-based architecture** for analysis and transformation.

### Analysis Passes (read-only)
- `DominatorTree` — computes dominator tree for a function
- `LoopInfo` — identifies loops
- `ScalarEvolution` — analyzes induction variables
- `AliasAnalysis` — determines memory aliasing
- `MemorySSA` — SSA form for memory operations

### Transform Passes (mutate IR)
- `InstCombine` — peephole optimizations
- `GVN` — global value numbering (redundancy elimination)
- `LICM` — loop-invariant code motion
- `Inliner` — function inlining
- `SimplifyCFG` — control flow simplification
- `Mem2Reg` — promote memory to registers (alloca → SSA values)

```cpp
// New PM style (LLVM 17+)
llvm::FunctionPassManager fpm;
fpm.addPass(llvm::InstCombinePass());
fpm.addPass(llvm::SimplifyCFGPass());
fpm.addPass(llvm::GVNPass());
```

## 6.7 LLVM Support Library

LLVM provides excellent utility classes:

```cpp
// String utilities
llvm::StringRef str = "hello";
llvm::Twine combined = str + " world";
std::string s = combined.str();

// Efficient containers
llvm::SmallVector<int, 8> vec;   // stack allocated for up to 8 elements
llvm::DenseMap<int, std::string> map;
llvm::SmallPtrSet<Value*, 16> set;

// Error handling
llvm::Error err = ...;
if (auto e = handleErrors(std::move(err))) {
    llvm::errs() << toString(std::move(e)) << "\n";
}

// Debug output
llvm::outs() << "Debug: " << value << "\n";
llvm::errs() << "Error: " << msg << "\n";
value->print(llvm::outs());
module->dump();  // prints entire module
```

## 6.8 LLVM Project Structure

```
llvm-project/
├── llvm/            # Core libraries (IR, optimizer, backends, tools)
│   ├── include/     # Public headers
│   ├── lib/         # Library implementations
│   │   ├── IR/      # IR data structures
│   │   ├── Transforms/ # Optimization passes
│   │   └── Target/  # Architecture backends (X86, ARM, AArch64, ...)
│   └── tools/       # llc, opt, llvm-link, llvm-dis, etc.
├── clang/           # C/C++/Objective-C frontend
├── lld/             # Linker
├── lldb/            # Debugger
├── compiler-rt/     # Runtime libraries (sanitizers, builtins)
└── libcxx/          # C++ standard library implementation
```

## 6.9 Summary

- LLVM is a modular compiler infrastructure: IR → optimizer → backend.
- The `Module`, `Function`, `BasicBlock`, and `Value` classes form the IR hierarchy.
- `IRBuilder` is the primary API for generating LLVM IR in C++.
- LLVM supports JIT compilation, object file emission, and optimization passes.
- The pass infrastructure enables composable analysis and transformation.

---

*Next: [Chapter 7: LLVM Intermediate Representation](./07-llvm-ir.md)*
