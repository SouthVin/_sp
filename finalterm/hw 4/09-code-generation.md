# Chapter 9: Code Generation with LLVM

## 9.1 The Backend Pipeline

Once the frontend produces LLVM IR, the **backend** translates it into machine code. LLVM's backend pipeline includes:

```
LLVM IR
    │
    ▼
┌─────────────────┐
│ Instruction      │  Map IR operations to target-specific nodes
│ Selection        │  using SelectionDAG or GlobalISel
└────────┬────────┘
         ▼
┌─────────────────┐
│ Instruction      │  Peephole optimizations, macro fusion
│ Scheduling       │  Reorder instructions for pipeline efficiency
└────────┬────────┘
         ▼
┌─────────────────┐
│ Register         │  Assign virtual registers to physical registers
│ Allocation       │  (greedy, basic, PBQP algorithms)
└────────┬────────┘
         ▼
┌─────────────────┐
│ Code Emission    │  Emit assembly text or object file (ELF, Mach-O, COFF)
└─────────────────┘
```

## 9.2 Setting Up LLVM Targets

```cpp
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

// Initialize all target support (call once at program start)
void initializeTargets() {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
}
```

## 9.3 Selecting a Target

```cpp
// Get the native target triple
std::string targetTriple = llvm::sys::getDefaultTargetTriple();

// Or specify manually
// targetTriple = "x86_64-pc-linux-gnu"
// targetTriple = "aarch64-apple-darwin"
// targetTriple = "armv7-unknown-linux-gnueabihf"

std::string error;
const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
if (!target) {
    llvm::errs() << "Target lookup error: " << error << "\n";
    return;
}
```

## 9.4 Creating a TargetMachine

The `TargetMachine` encapsulates everything needed for a specific architecture:

```cpp
llvm::TargetOptions options;
// Set target-specific options
options.UnsafeFPMath = false;
options.NoInfsFPMath = false;
options.NoNaNsFPMath = false;
options.NoSignedZerosFPMath = false;

std::string cpu = "generic";       // "skylake", "cortex-a72", etc.
std::string features = "";          // "+sse2,+avx" or "-sse4.1"
llvm::Reloc::Model relocModel = llvm::Reloc::PIC_;
llvm::CodeModel::Model codeModel = llvm::CodeModel::Small;
llvm::CodeGenOptLevel optLevel = llvm::CodeGenOptLevel::Default;

std::unique_ptr<llvm::TargetMachine> targetMachine(
    target->createTargetMachine(
        targetTriple, cpu, features,
        options, relocModel, codeModel, optLevel));

// Apply data layout to module
module->setDataLayout(targetMachine->createDataLayout());
module->setTargetTriple(targetTriple);
```

### Common CPU Names

| Architecture | CPU Names |
|-------------|-----------|
| x86_64 | `generic`, `core2`, `nehalem`, `haswell`, `skylake`, `znver3`, `alderlake` |
| AArch64 | `generic`, `cortex-a53`, `cortex-a72`, `apple-a14`, `apple-m1` |
| ARM | `generic`, `cortex-a8`, `cortex-a9`, `cortex-a15` |

### Relocation Models

| Model | Description |
|-------|-------------|
| `Static` | No relocations, absolute addressing |
| `PIC_` | Position-independent code (shared libraries) |
| `DynamicNoPIC` | Relocatable but not position-independent |
| `ROPI` | Read-only position-independent (embedded) |
| `RWPI` | Read-write position-independent (embedded) |

### Code Models

| Model | Description |
|-------|-------------|
| `Tiny` | Code + data < 1MB |
| `Small` | Code + data < 2GB (default) |
| `Kernel` | Kernel code model (negative 2GB) |
| `Medium` | Code < 2GB, data unlimited |
| `Large` | No restrictions on code or data |

## 9.5 Emitting Object Files

```cpp
void emitObjectFile(llvm::Module& module, const std::string& filename) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

    if (ec) {
        llvm::errs() << "Could not open file: " << ec.message() << "\n";
        return;
    }

    llvm::legacy::PassManager pm;

    // Request object file output
    llvm::CodeGenFileType fileType = llvm::CGFT_ObjectFile;
    if (targetMachine->addPassesToEmitFile(pm, dest, nullptr, fileType)) {
        llvm::errs() << "TargetMachine can't emit a file of this type\n";
        return;
    }

    pm.run(module);
    dest.flush();
}
```

## 9.6 Emitting Assembly Code

```cpp
void emitAssembly(llvm::Module& module, const std::string& filename) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

    llvm::legacy::PassManager pm;
    llvm::CodeGenFileType fileType = llvm::CGFT_AssemblyFile;

    if (targetMachine->addPassesToEmitFile(pm, dest, nullptr, fileType)) {
        llvm::errs() << "TargetMachine can't emit assembly\n";
        return;
    }

    pm.run(module);
    dest.flush();
}
```

## 9.7 JIT Compilation

LLVM's ORC (On-Request Compilation) JIT framework enables runtime code generation and execution:

### Simple ORC JIT Setup

```cpp
#include "llvm/ExecutionEngine/Orc/LLJIT.h"

class SimpleJIT {
    std::unique_ptr<llvm::orc::LLJIT> JIT;

public:
    SimpleJIT(std::unique_ptr<llvm::orc::LLJIT> JIT) : JIT(std::move(JIT)) {}

    static llvm::Expected<std::unique_ptr<SimpleJIT>> Create() {
        auto JIT = llvm::orc::LLJITBuilder().create();
        if (!JIT) return JIT.takeError();
        return std::make_unique<SimpleJIT>(std::move(*JIT));
    }

    llvm::Error addModule(llvm::orc::ThreadSafeModule TSM,
                          llvm::orc::ResourceTrackerSP RT = nullptr) {
        if (!RT)
            RT = JIT->getMainJITDylib().createResourceTracker();
        return JIT->addIRModule(RT, std::move(TSM));
    }

    llvm::Expected<llvm::JITEvaluatedSymbol> lookup(llvm::StringRef Name) {
        return JIT->lookup(Name);
    }
};
```

### Using the JIT

```cpp
auto jit = SimpleJIT::Create();
if (!jit) {
    llvm::errs() << "Failed to create JIT\n";
    return 1;
}

// Create a module with a function
auto context = std::make_unique<llvm::LLVMContext>();
auto module = std::make_unique<llvm::Module>("jit_module", *context);

// Build a function: int add(int a, int b) { return a + b; }
llvm::IRBuilder<> builder(*context);
auto* returnType = llvm::Type::getInt32Ty(*context);
auto* funcType = llvm::FunctionType::get(returnType, {returnType, returnType}, false);
auto* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "add", module.get());

auto* entry = llvm::BasicBlock::Create(*context, "entry", func);
builder.SetInsertPoint(entry);
auto* a = func->getArg(0);
auto* b = func->getArg(1);
auto* sum = builder.CreateAdd(a, b, "sum");
builder.CreateRet(sum);

// Add module to JIT
auto tsm = llvm::orc::ThreadSafeModule(std::move(module), std::move(context));
if (auto err = (*jit)->addModule(std::move(tsm))) {
    llvm::errs() << "Failed to add module\n";
    return 1;
}

// Look up and call the function
auto sym = (*jit)->lookup("add");
if (!sym) {
    llvm::errs() << "Failed to lookup function\n";
    return 1;
}

using AddFunc = int (*)(int, int);
auto* addFn = sym->getAddress().toPtr<AddFunc>();
int result = addFn(3, 4);  // result = 7
printf("3 + 4 = %d\n", result);
```

### JIT with Lazy Compilation

```cpp
auto JIT = llvm::orc::LLLazyJITBuilder().create();
// Functions are compiled only when first called
```

## 9.8 Optimization Pipeline for CodeGen

Before code generation, run LLVM optimizations:

```cpp
void optimizeModule(llvm::Module& module, llvm::TargetMachine* TM) {
    // Create analysis managers
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    // Register passes
    llvm::PassBuilder PB(TM);

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // Create the optimization pipeline
    llvm::ModulePassManager MPM;

    // Use the default -O2 pipeline
    llvm::OptimizationLevel level = llvm::OptimizationLevel::O2;
    if (auto Err = PB.parsePassPipeline(MPM, "default<O2>")) {
        llvm::errs() << "Failed to parse pass pipeline\n";
        return;
    }

    MPM.run(module, MAM);
}
```

## 9.9 Cross-Compilation

LLVM makes cross-compilation straightforward:

```cpp
// Compile for ARM from x86_64
std::string targetTriple = "armv7-unknown-linux-gnueabihf";
std::string cpu = "cortex-a9";
module->setTargetTriple(targetTriple);

std::string error;
const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
auto* targetMachine = target->createTargetMachine(targetTriple, cpu, "", {}, llvm::Reloc::Static);

module->setDataLayout(targetMachine->createDataLayout());
// ... emit object file as usual
```

Common target triples:

| Platform | Triple |
|----------|--------|
| Linux x86_64 | `x86_64-pc-linux-gnu` |
| macOS x86_64 | `x86_64-apple-darwin` |
| macOS ARM64 | `arm64-apple-darwin` |
| Linux ARM64 | `aarch64-unknown-linux-gnu` |
| Linux ARM32 | `armv7-unknown-linux-gnueabihf` |
| Windows x86_64 | `x86_64-pc-windows-msvc` |
| WebAssembly | `wasm32-unknown-unknown` |

## 9.10 Debug Information

Generate DWARF debug info for source-level debugging:

```cpp
#include "llvm/IR/DIBuilder.h"

void addDebugInfo(llvm::Module& module, const std::string& filename) {
    llvm::DIBuilder diBuilder(module);

    // Create compile unit
    auto* file = diBuilder.createFile(filename, ".");
    auto* cu = diBuilder.createCompileUnit(
        llvm::dwarf::DW_LANG_C, file, "MyCompiler", false, "", 0);

    // Create a type for 'int'
    auto* intType = diBuilder.createBasicType("int", 32, llvm::dwarf::DW_ATE_signed);

    // For each function, create subprogram and scope
    // auto* subprogram = diBuilder.createFunction(
    //     fileScope, funcName, linkageName, file, lineNo, funcType,
    //     scopeLine, flags, spFlags);
    //
    // func->setSubprogram(subprogram);

    diBuilder.finalize();
}
```

## 9.11 Object File Formats

LLVM supports three major object file formats:

| Format | Platform | Extension |
|--------|----------|-----------|
| **ELF** | Linux, BSD, embedded | `.o`, `.so` |
| **Mach-O** | macOS, iOS | `.o`, `.dylib` |
| **COFF** | Windows | `.obj`, `.dll` |

The format is automatically selected based on the target triple.

## 9.12 Summary

- The LLVM backend translates IR to machine code via instruction selection, scheduling, register allocation, and code emission.
- `TargetMachine` encapsulates the target architecture's characteristics.
- LLVM supports object files, assembly, and JIT compilation.
- Cross-compilation uses target triples to specify the target architecture.
- The ORC JIT framework enables efficient runtime code generation.
- Debug information (DWARF) can be generated through `DIBuilder`.

---

*Next: [Chapter 10: Compiler Optimizations](./10-optimizations.md)*
