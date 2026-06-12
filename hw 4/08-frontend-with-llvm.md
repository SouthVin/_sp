# Chapter 8: Building a Frontend with LLVM

## 8.1 Frontend Responsibilities

A **frontend** for LLVM translates a source language into LLVM IR. It encompasses:

1. **Lexical analysis** — tokenizing source text.
2. **Syntax analysis** — building an AST.
3. **Semantic analysis** — type checking and symbol resolution.
4. **IR code generation** — walking the validated AST and emitting LLVM IR.

## 8.2 The Kaleidoscope Language

To demonstrate building a frontend, we'll use a simple language called **Kaleidoscope** — a toy language that supports:

- Function definitions
- Arithmetic expressions
- If-then-else conditionals
- For loops
- External function declarations (calling C standard library)

```
# Compute the nth Fibonacci number
def fib(n)
  if n < 3 then
    1
  else
    fib(n-1) + fib(n-2);

# External declaration
extern sin(x);

# Top-level expression (REPL-style)
fib(10);
```

## 8.3 AST for Kaleidoscope

```cpp
// Forward declarations
class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual llvm::Value* codegen() = 0;
};

// Number: 1.0, 42.0
class NumberExprAST : public ExprAST {
    double Val;
public:
    NumberExprAST(double Val) : Val(Val) {}
    llvm::Value* codegen() override;
};

// Variable: x, foo
class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST(const std::string& Name) : Name(Name) {}
    llvm::Value* codegen() override;
    const std::string& getName() const { return Name; }
};

// Binary expression: a + b, x * y
class BinaryExprAST : public ExprAST {
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;
public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    llvm::Value* codegen() override;
};

// Function call: foo(1, 2 + 3)
class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;
public:
    CallExprAST(const std::string& Callee,
                std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
    llvm::Value* codegen() override;
};

// If expression: if x < 3 then 1 else 2
class IfExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Cond, Then, Else;
public:
    IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
              std::unique_ptr<ExprAST> Else)
        : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
    llvm::Value* codegen() override;
};

// For expression: for i = 0, i < n, 1 in body
class ForExprAST : public ExprAST {
    std::string VarName;
    std::unique_ptr<ExprAST> Start, End, Step, Body;
public:
    ForExprAST(const std::string& VarName, std::unique_ptr<ExprAST> Start,
               std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
               std::unique_ptr<ExprAST> Body)
        : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
          Step(std::move(Step)), Body(std::move(Body)) {}
    llvm::Value* codegen() override;
};

// Function prototype: foo(a, b)
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;
public:
    PrototypeAST(const std::string& Name, std::vector<std::string> Args)
        : Name(Name), Args(std::move(Args)) {}
    const std::string& getName() const { return Name; }
    llvm::Function* codegen();
};

// Function definition: def foo(a, b) a + b
class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;
public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
    llvm::Function* codegen();
};
```

## 8.4 Code Generation on the AST

```cpp
static std::unique_ptr<llvm::LLVMContext> TheContext;
static std::unique_ptr<llvm::Module> TheModule;
static std::unique_ptr<llvm::IRBuilder<>> Builder;
static std::map<std::string, llvm::AllocaInst*> NamedValues;

llvm::Value* NumberExprAST::codegen() {
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(Val));
}

llvm::Value* VariableExprAST::codegen() {
    llvm::AllocaInst* A = NamedValues[Name];
    if (!A)
        return LogErrorV("Unknown variable name: " + Name);
    return Builder->CreateLoad(A->getAllocatedType(), A, Name.c_str());
}

llvm::Value* BinaryExprAST::codegen() {
    llvm::Value* L = LHS->codegen();
    llvm::Value* R = RHS->codegen();
    if (!L || !R) return nullptr;

    switch (Op) {
        case '+': return Builder->CreateFAdd(L, R, "addtmp");
        case '-': return Builder->CreateFSub(L, R, "subtmp");
        case '*': return Builder->CreateFMul(L, R, "multmp");
        case '<':
            L = Builder->CreateFCmpULT(L, R, "cmptmp");
            // Convert bool 0/1 to double 0.0/1.0
            return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext), "booltmp");
        default:
            return LogErrorV("Invalid binary operator");
    }
}

llvm::Value* CallExprAST::codegen() {
    llvm::Function* CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
        return LogErrorV("Unknown function: " + Callee);

    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # of arguments passed");

    std::vector<llvm::Value*> ArgsV;
    for (auto& Arg : Args) {
        ArgsV.push_back(Arg->codegen());
        if (!ArgsV.back()) return nullptr;
    }

    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

llvm::Value* IfExprAST::codegen() {
    llvm::Value* CondV = Cond->codegen();
    if (!CondV) return nullptr;

    // Convert condition to boolean by comparing != 0.0
    CondV = Builder->CreateFCmpONE(
        CondV, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)), "ifcond");

    llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* ThenBB  = llvm::BasicBlock::Create(*TheContext, "then", TheFunction);
    llvm::BasicBlock* ElseBB  = llvm::BasicBlock::Create(*TheContext, "else");
    llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(*TheContext, "ifcont");

    Builder->CreateCondBr(CondV, ThenBB, ElseBB);

    // Emit then block
    Builder->SetInsertPoint(ThenBB);
    llvm::Value* ThenV = Then->codegen();
    if (!ThenV) return nullptr;
    Builder->CreateBr(MergeBB);
    // Codegen of 'Then' can change the current block; update ThenBB for PHI
    ThenBB = Builder->GetInsertBlock();

    // Emit else block
    TheFunction->insert(TheFunction->end(), ElseBB);
    Builder->SetInsertPoint(ElseBB);
    llvm::Value* ElseV = Else->codegen();
    if (!ElseV) return nullptr;
    Builder->CreateBr(MergeBB);
    ElseBB = Builder->GetInsertBlock();

    // Emit merge block with PHI node
    TheFunction->insert(TheFunction->end(), MergeBB);
    Builder->SetInsertPoint(MergeBB);
    llvm::PHINode* PN = Builder->CreatePHI(llvm::Type::getDoubleTy(*TheContext), 2, "iftmp");
    PN->addIncoming(ThenV, ThenBB);
    PN->addIncoming(ElseV, ElseBB);
    return PN;
}

llvm::Value* ForExprAST::codegen() {
    llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();

    // Create alloca for the loop variable
    llvm::AllocaInst* Alloca = CreateEntryBlockAlloca(
        TheFunction, VarName, llvm::Type::getDoubleTy(*TheContext));

    // Emit start value
    llvm::Value* StartVal = Start->codegen();
    if (!StartVal) return nullptr;
    Builder->CreateStore(StartVal, Alloca);

    llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(*TheContext, "loop", TheFunction);
    llvm::BasicBlock* AfterBB = llvm::BasicBlock::Create(*TheContext, "afterloop");

    // Insert explicit fall-through from current block to LoopBB
    Builder->CreateBr(LoopBB);
    Builder->SetInsertPoint(LoopBB);

    // Save old binding so we can restore it after the loop
    llvm::AllocaInst* OldVal = NamedValues[VarName];
    NamedValues[VarName] = Alloca;

    // Emit body
    if (!Body->codegen()) return nullptr;

    // Emit step
    llvm::Value* StepVal = nullptr;
    if (Step) {
        StepVal = Step->codegen();
        if (!StepVal) return nullptr;
    } else {
        StepVal = llvm::ConstantFP::get(*TheContext, llvm::APFloat(1.0));
    }

    // Compute next value: cur = cur + step
    llvm::Value* CurVar = Builder->CreateLoad(
        Alloca->getAllocatedType(), Alloca, VarName.c_str());
    llvm::Value* NextVar = Builder->CreateFAdd(CurVar, StepVal, "nextvar");
    Builder->CreateStore(NextVar, Alloca);

    // Convert condition to bool (compare != 0.0)
    llvm::Value* EndCond = End->codegen();
    if (!EndCond) return nullptr;
    EndCond = Builder->CreateFCmpONE(
        EndCond, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)), "loopcond");

    Builder->CreateCondBr(EndCond, LoopBB, AfterBB);

    // End loop
    TheFunction->insert(TheFunction->end(), AfterBB);
    Builder->SetInsertPoint(AfterBB);

    // Restore old variable binding
    if (OldVal)
        NamedValues[VarName] = OldVal;
    else
        NamedValues.erase(VarName);

    // Loop always returns 0.0 in Kaleidoscope
    return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*TheContext));
}
```

## 8.5 Function Definitions and Prototypes

```cpp
llvm::Function* PrototypeAST::codegen() {
    // Make the function type: double(double, double, ...)
    std::vector<llvm::Type*> Doubles(Args.size(),
        llvm::Type::getDoubleTy(*TheContext));
    llvm::FunctionType* FT = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*TheContext), Doubles, false);
    llvm::Function* F = llvm::Function::Create(
        FT, llvm::Function::ExternalLinkage, Name, TheModule.get());

    // Set names for arguments
    unsigned Idx = 0;
    for (auto& Arg : F->args())
        Arg.setName(Args[Idx++]);

    return F;
}

llvm::Function* FunctionAST::codegen() {
    // Check for existing function from previous 'extern' declaration
    llvm::Function* TheFunction = TheModule->getFunction(Proto->getName());

    if (!TheFunction)
        TheFunction = Proto->codegen();

    if (!TheFunction) return nullptr;
    if (!TheFunction->empty())
        return (llvm::Function*)LogErrorV("Function cannot be redefined.");

    // Create entry basic block
    llvm::BasicBlock* BB = llvm::BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

    // Record function arguments in NamedValues map
    NamedValues.clear();
    for (auto& Arg : TheFunction->args()) {
        llvm::AllocaInst* Alloca = CreateEntryBlockAlloca(
            TheFunction, std::string(Arg.getName()),
            llvm::Type::getDoubleTy(*TheContext));
        Builder->CreateStore(&Arg, Alloca);
        NamedValues[std::string(Arg.getName())] = Alloca;
    }

    if (llvm::Value* RetVal = Body->codegen()) {
        Builder->CreateRet(RetVal);
        llvm::verifyFunction(*TheFunction);
        return TheFunction;
    }

    TheFunction->eraseFromParent();
    return nullptr;
}
```

## 8.6 The Complete Driver (REPL)

```cpp
static void HandleDefinition() {
    if (auto FnAST = ParseDefinition()) {
        if (auto* FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read function definition:\n");
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    } else {
        // Skip token for error recovery
        getNextToken();
    }
}

static void HandleExtern() {
    if (auto ProtoAST = ParseExtern()) {
        if (auto* FnIR = ProtoAST->codegen()) {
            fprintf(stderr, "Read extern:\n");
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    } else {
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    if (auto FnAST = ParseTopLevelExpr()) {
        if (auto* FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read top-level expression:\n");
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");

            // JIT the expression
            auto ExprSymbol = TheJIT->lookup("__anon_expr");
            assert(ExprSymbol && "Function not found");
            double (*FP)() = ExprSymbol->getAddress().toPtr<double(*)()>();
            fprintf(stderr, "Evaluated to %f\n", FP());

            // Remove the anonymous expression
            FnIR->eraseFromParent();
        }
    } else {
        getNextToken();
    }
}

int main() {
    // Initialize LLVM targets
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // Prime the first token
    fprintf(stderr, "ready> ");
    getNextToken();

    // Initialize JIT
    TheJIT = llvm::orc::KaleidoscopeJIT::Create();

    // Initialize module and passes
    InitializeModuleAndPassManager();

    // Run the main interpreter loop
    while (true) {
        switch (CurTok) {
            case tok_eof:   return 0;
            case ';':       getNextToken(); break;  // ignore top-level semicolons
            case tok_def:   HandleDefinition(); break;
            case tok_extern: HandleExtern(); break;
            default:        HandleTopLevelExpression(); break;
        }
    }
    return 0;
}
```

## 8.7 Error Handling

```cpp
std::unique_ptr<ExprAST> LogError(const char* Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char* Str) {
    LogError(Str);
    return nullptr;
}

llvm::Value* LogErrorV(const char* Str) {
    LogError(Str);
    return nullptr;
}
```

## 8.8 Summary

- A frontend translates source language to LLVM IR through lexing → parsing → semantic analysis → code generation.
- Each AST node implements a `codegen()` method that emits the corresponding LLVM IR.
- `IRBuilder` is the key API: `CreateFAdd`, `CreateLoad`, `CreateCondBr`, `CreatePHI`, etc.
- Memory for local variables uses `alloca` + `load`/`store`; control flow uses basic blocks with branches.
- The Kaleidoscope tutorial demonstrates a minimal but complete frontend in ~1000 lines of C++.

---

*Next: [Chapter 9: Code Generation with LLVM](./09-code-generation.md)*
