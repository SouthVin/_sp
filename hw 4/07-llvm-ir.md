# Chapter 7: LLVM Intermediate Representation (LLVM IR)

## 7.1 What Makes LLVM IR Special?

LLVM IR is:
- **SSA-based** — each virtual register is assigned exactly once.
- **Strongly typed** — every value has a well-defined type.
- **Three-address code** — each instruction operates on at most two operands.
- **Language-independent** — used by C, C++, Rust, Swift, Julia, Zig, and many others.
- **Human-readable** — can be serialized to `.ll` text or `.bc` bitcode.

## 7.2 LLVM IR Structure

```
; Module level
target triple = "x86_64-pc-linux-gnu"

; Global variables and constants
@global_var = global i32 42
@.str = private constant [14 x i8] c"Hello, world!\00"

; Function declarations
declare i32 @puts(ptr)

; Function definitions
define i32 @main() {
entry:
  ; Local values (virtual registers) start with %
  %x = alloca i32
  store i32 10, ptr %x
  %val = load i32, ptr %x
  %result = add i32 %val, 5
  ret i32 %result
}
```

### Naming Convention

| Prefix | Meaning | Example |
|--------|---------|---------|
| `%` | Local value (virtual register) | `%result`, `%1` |
| `@` | Global value | `@main`, `@global_var` |
| None | Constant / keyword | `i32 42`, `add`, `ret` |

## 7.3 LLVM Types in Depth

### Integer Types

```
i1       ; 1-bit (boolean)
i8       ; 8-bit (byte, char)
i32      ; 32-bit (int)
i64      ; 64-bit (long)
i128     ; 128-bit (for large integers)
```

### Floating Point Types

```
half     ; 16-bit IEEE 754 (rare)
float    ; 32-bit IEEE 754
double   ; 64-bit IEEE 754 (default for C double)
fp128    ; 128-bit IEEE 754 extended
```

### Pointer Types (LLVM 17+ — Opaque Pointers)

```
ptr                          ; opaque pointer (LLVM 17+)
ptr addrspace(1)             ; address space 1 (e.g., GPU global memory)
```

Before LLVM 15, pointers were typed: `i32*`, `i8**`. Opaque pointers simplify the IR.

### Aggregate Types

```
[10 x i32]                   ; array of 10 i32 values
[N x T]                      ; fixed-size array of type T
{i32, double, i8}            ; anonymous struct (packed by default)
<{i32, double}>              ; packed struct (no padding)
<4 x float>                  ; vector of 4 floats (SIMD)
```

### Function Types

```
i32 (i32, i32)               ; function taking two i32, returning i32
void (ptr)                   ; function taking a pointer, returning void
```

### Void Type

```
void                         ; represents "no value"
```

## 7.4 Instruction Reference

### Terminator Instructions (must appear at end of basic block)

```
ret i32 %value               ; return from function with value
ret void                     ; return from void function
br label %dest               ; unconditional branch
br i1 %cond, label %true, label %false  ; conditional branch
switch i32 %val, label %default [ i32 1, label %case1, i32 2, label %case2 ]
indirectbr ptr %addr, [label %dest1, label %dest2]
invoke ... to label %normal unwind label %exception
callbr ... to label %normal [label %indirect_dest1, ...]
unreachable                  ; indicates unreachable code
```

### Binary Operations

```
add  i32 %a, %b              ; integer addition
sub  i32 %a, %b              ; integer subtraction
mul  i32 %a, %b              ; integer multiplication
sdiv i32 %a, %b              ; signed integer division
udiv i32 %a, %b              ; unsigned integer division
srem i32 %a, %b              ; signed remainder
urem i32 %a, %b              ; unsigned remainder

; Bitwise
shl  i32 %a, %b              ; shift left
lshr i32 %a, %b              ; logical shift right
ashr i32 %a, %b              ; arithmetic shift right
and  i32 %a, %b              ; bitwise AND
or   i32 %a, %b              ; bitwise OR
xor  i32 %a, %b              ; bitwise XOR

; Floating point
fadd float %a, %b
fsub double %a, %b
fmul float %a, %b
fdiv double %a, %b
frem float %a, %b
```

### Comparison Operations

```
; Integer comparisons
icmp eq  i32 %a, %b          ; equal
icmp ne  i32 %a, %b          ; not equal
icmp sgt i32 %a, %b          ; signed greater than
icmp sge i32 %a, %b          ; signed greater than or equal
icmp slt i32 %a, %b          ; signed less than
icmp sle i32 %a, %b          ; signed less than or equal
icmp ugt i32 %a, %b          ; unsigned greater than
icmp uge i32 %a, %b          ; unsigned greater or equal
icmp ult i32 %a, %b          ; unsigned less than
icmp ule i32 %a, %b          ; unsigned less or equal

; Floating point comparisons (ordered = no NaN, unordered = NaN allowed)
fcmp oeq  float %a, %b       ; ordered equal
fcmp one  float %a, %b       ; ordered not equal
fcmp ogt  float %a, %b       ; ordered greater than
fcmp oge  float %a, %b       ; ordered greater or equal
fcmp olt  float %a, %b       ; ordered less than
fcmp ole  float %a, %b       ; ordered less or equal
fcmp ueq  float %a, %b       ; unordered equal
fcmp une  float %a, %b       ; unordered not equal
; ... and others (ult, ule, ugt, uge, uno, ord, true, false)
```

### Memory Operations

```
; Opaque pointer era (LLVM 17+)
%ptr = alloca i32                        ; allocate on stack
%val = load i32, ptr %ptr                ; load value from pointer
store i32 42, ptr %ptr                   ; store value to pointer
%elem = getelementptr [10 x i32], ptr %arr, i32 0, i32 %idx  ; address calculation

; Volatile access (prevent optimization)
%v = load volatile i32, ptr %mmio_reg
store volatile i32 %val, ptr %mmio_reg

; Atomic operations
%old = load atomic i32, ptr %ptr seq_cst, align 4
store atomic i32 %val, ptr %ptr seq_cst, align 4
%result = cmpxchg ptr %ptr, i32 %expected, i32 %desired seq_cst seq_cst
%old = atomicrmw add ptr %ptr, i32 1 seq_cst  ; fetch-and-add
```

### Conversion (Cast) Operations

```
%y = trunc i64 %x to i32              ; integer truncation
%z = zext i8 %x to i32                ; zero extension
%s = sext i8 %x to i32                ; sign extension
%f = sitofp i32 %x to float           ; signed int to float
%u = uitofp i32 %x to float           ; unsigned int to float
%i = fptosi float %x to i32           ; float to signed int
%u = fptoui float %x to i32           ; float to unsigned int
%f2 = fptrunc double %x to float      ; float truncation
%f3 = fpext float %x to double        ; float extension
%p = ptrtoint ptr %x to i64           ; pointer to integer
%q = inttoptr i64 %x to ptr           ; integer to pointer
%b = bitcast i32* %x to float*        ; bit reinterpretation (pre-LLVM 17)
```

### Other Instructions

```
%val = select i1 %cond, i32 %t, i32 %f   ; conditional selection (ternary)
%x = phi i32 [ %v1, %label1 ], [ %v2, %label2 ]  ; SSA phi node
%call = call i32 @foo(i32 %a, i32 %b)    ; function call (known target)
%call = call i32 %fptr(i32 %a)           ; indirect call (function pointer)
%land = landingpad {ptr, i32} cleanup    ; exception landing pad
%v = extractvalue {i32, float} %s, 0     ; extract from aggregate
%s2 = insertvalue {i32, float} %s, i32 42, 0  ; insert into aggregate
%v = extractelement <4 x float> %vec, i32 2   ; extract from vector
```

## 7.5 Common LLVM IR Patterns

### Local Variable (with alloca)

```
define i32 @example() {
entry:
  %x = alloca i32            ; declare local x
  store i32 42, ptr %x       ; x = 42
  %val = load i32, ptr %x    ; read x
  %result = mul i32 %val, 2  ; x * 2
  ret i32 %result
}
```

### If-Else

```
define i32 @max(i32 %a, i32 %b) {
entry:
  %cmp = icmp sgt i32 %a, %b
  br i1 %cmp, label %if.then, label %if.else

if.then:
  ret i32 %a

if.else:
  ret i32 %b
}
```

### While Loop (factorial)

```
define i32 @factorial(i32 %n) {
entry:
  %result = alloca i32
  store i32 1, ptr %result
  %i = alloca i32
  store i32 1, ptr %i
  br label %loop

loop:
  %i_val = load i32, ptr %i
  %n_val = load i32, ptr %n_ptr  ; if n is by pointer
  %done = icmp sgt i32 %i_val, %n
  br i1 %done, label %exit, label %body

body:
  %result_val = load i32, ptr %result
  %new_result = mul i32 %result_val, %i_val
  store i32 %new_result, ptr %result
  %next_i = add i32 %i_val, 1
  store i32 %next_i, ptr %i
  br label %loop

exit:
  %final = load i32, ptr %result
  ret i32 %final
}
```

### Struct Access with GEP

```
%struct.Point = type { float, float }

define float @get_x(ptr %point) {
entry:
  ; GEP: get address of first element (index 0) of struct
  %x_ptr = getelementptr %struct.Point, ptr %point, i32 0, i32 0
  %x = load float, ptr %x_ptr
  ret float %x
}
```

### Array Access

```
define i32 @get_element(ptr %arr, i32 %idx) {
entry:
  ; GEP: get address of arr[idx]
  ; First index 0 dereferences the array pointer
  ; Second index is the element offset
  %elem_ptr = getelementptr [10 x i32], ptr %arr, i32 0, i32 %idx
  %val = load i32, ptr %elem_ptr
  ret i32 %val
}
```

## 7.6 Intrinsics

LLVM provides many **intrinsics** — built-in functions the compiler treats specially:

```llvm
; Math
declare float @llvm.sqrt.f32(float)           ; sqrt
declare float @llvm.sin.f32(float)            ; sine
declare float @llvm.cos.f32(float)            ; cosine
declare float @llvm.fma.f32(float, float, float) ; fused multiply-add

; Memory
declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)  ; memcpy
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)      ; memset
declare void @llvm.memmove.p0.p0.i64(ptr, ptr, i64, i1) ; memmove

; Overflow-checked arithmetic
declare {i32, i1} @llvm.sadd.with.overflow.i32(i32, i32)  ; add with overflow flag
declare {i32, i1} @llvm.ssub.with.overflow.i32(i32, i32)  ; sub with overflow flag

; Expect / branch hints
declare i1 @llvm.expect.i1(i1, i1)    ; __builtin_expect

; Assumption
declare void @llvm.assume(i1)          ; optimizer assumption

; Debug info
declare void @llvm.dbg.declare(metadata, metadata, metadata)
declare void @llvm.dbg.value(metadata, metadata, metadata)

; Stack protection
declare void @llvm.stackprotector(ptr, ptr)
```

## 7.7 Metadata

```llvm
; Debug information
!0 = !DIFile(filename: "example.c", directory: "/path/")
!1 = !DILexicalBlock(...)

; Optimization hints
; Branch weights for profile-guided optimization
br i1 %cond, label %likely, label %unlikely, !prof !0
!0 = !{!"branch_weights", i32 99, i32 1}

; Loop metadata
br label %loop, !llvm.loop !1
!1 = !{!1, !2}
!2 = !{!"llvm.loop.unroll.enable"}
```

## 7.8 Reading and Writing LLVM IR

```cpp
// Parse IR from string
llvm::SMDiagnostic err;
auto module = llvm::parseIR(
    llvm::MemoryBufferRef(irString, "test"),
    err, context);

// Parse IR from file
llvm::SMDiagnostic err;
auto module = llvm::parseIRFile("input.ll", err, context);

// Write IR to string
std::string str;
llvm::raw_string_ostream os(str);
module->print(os, nullptr);

// Write bitcode (.bc)
llvm::WriteBitcodeToFile(*module, llvm::raw_fd_ostream("output.bc", ec));

// Dump to stderr
module->dump();
```

## 7.9 Verifying LLVM IR

Always verify generated IR to catch errors early:

```cpp
if (llvm::verifyModule(*module, &llvm::errs())) {
    llvm::errs() << "Module verification failed!\n";
    module->dump();
    return;
}

// Or for a single function
if (llvm::verifyFunction(*func, &llvm::errs())) {
    llvm::errs() << "Function verification failed!\n";
    func->dump();
    return;
}
```

## 7.10 Summary

- LLVM IR is an SSA-based, strongly-typed intermediate representation.
- Values are local (`%name`) or global (`@name`); types include integers, floats, pointers, arrays, and structs.
- Instructions cover arithmetic, memory access, control flow, casts, and intrinsics.
- `getelementptr` (GEP) is the canonical way to compute addresses of aggregate elements.
- Use `verifyModule` / `verifyFunction` to validate IR correctness.
- LLVM IR can be serialized as text (`.ll`) or binary bitcode (`.bc`).

---

*Next: [Chapter 8: Building a Frontend with LLVM](./08-frontend-with-llvm.md)*
