# ToyEmu-8: A Minimal 8-bit CPU Emulator with Assembler

## AI Usage Declaration

This project was developed with the assistance of **OpenCode** (powered by DeepSeek-V4). The AI assisted with:
- Generating all C source code (`toyemu.h`, `toyemu.c`, `asm.c`, `main.c`) and the `Makefile`.
- Writing all six example assembly programs.
- Writing this README report in its entirety.

No code was copied from other students or external sources. The CPU architecture design, instruction set, assembler grammar, and example programs were conceived specifically for this project. The AI generated all implementation details based on the architectural specifications provided in the conversation.

The conversation record with the AI is available in the OpenCode session history.

---

## 1. Project Overview

**ToyEmu-8** is a minimal 8-bit CPU emulator and assembler written in C. It simulates a von Neumann architecture machine with 256 bytes of addressable memory, four general-purpose registers, a stack, and a custom instruction set encoding. The project includes:

- **`toyemu.c`** — The CPU emulator implementing the fetch-decode-execute cycle.
- **`asm.c`** — A two-pass assembler that translates human-readable assembly into machine code.
- **`main.c`** — A command-line interface that assembles and runs `.asm` files.
- **`examples/`** — Six assembly programs demonstrating arithmetic, loops, branching, subroutines, and I/O.

This project connects directly to the **System Programming** course by exercising low-level concepts: instruction encoding, program counter management, stack-based call/return mechanics, flag registers, memory addressing, and the assembler-to-machine-code translation pipeline — all of which are foundational to understanding compilers, operating systems, and CPU architecture.

---

## 2. CPU Architecture

### Memory Model

| Property | Value |
|----------|-------|
| Architecture | Von Neumann (unified code + data) |
| Address space | 256 bytes (0x00–0xFF) |
| Word size | 8 bits |
| Stack location | Top of memory (SP initialized to 0xFF) |
| Stack direction | Grows downward (decrement SP on push, increment on pop) |

Programs are loaded starting at address `0x00`. The stack lives at the high end of memory and grows toward lower addresses. No memory protection or segmentation is implemented.

### Register Set

| Register | Width | Purpose |
|----------|-------|---------|
| R0 | 8-bit | General-purpose |
| R1 | 8-bit | General-purpose |
| R2 | 8-bit | General-purpose |
| R3 | 8-bit | General-purpose |
| PC | 8-bit | Program Counter — address of next instruction |
| SP | 8-bit | Stack Pointer — initialized to `0xFF` |
| FLAGS | 8-bit | Status register (only bits 0 and 1 used) |

All general-purpose registers are initialized to `0` at program start.

### Flag Register

| Bit | Name | Description |
|-----|------|-------------|
| 0 | Z (Zero) | Set when an arithmetic/logical result is `0` |
| 1 | C (Carry) | Set on arithmetic overflow (>255) or borrow |
| 2–7 | — | Unused (reserved) |

Flags are affected by: `ADD`, `SUB`, `CMP`, `INC`, `DEC`, `AND`, `OR`, `XOR`. The `CMP` instruction sets flags identically to `SUB` but discards the result.

---

## 3. Instruction Set Architecture (ISA)

### Encoding Format

Instructions use variable-length encoding (1, 2, or 3 bytes):

- **1-byte instructions:** `NOP`, `HLT`, `RET`
- **2-byte instructions:** Most ALU, register, and control flow operations. Byte 1 encodes registers or an address.
- **3-byte instructions:** `MVI` (load immediate), `LD` (load from memory), `ST` (store to memory). Bytes 1–2 encode a register and an immediate value/address.

For register-register instructions, the second byte encodes two registers:
```
Byte 1 = (destination_register << 4) | source_register
```

### Complete Opcode Table

| Opcode | Mnemonic | Bytes | Operands | Description |
|--------|----------|-------|----------|-------------|
| `0x00` | `NOP` | 1 | — | No operation |
| `0x01` | `HLT` | 1 | — | Halt the CPU |
| `0x02` | `MOV` | 2 | `Rd, Rs` | Copy Rs into Rd |
| `0x03` | `MVI` | 3 | `Rd, imm` | Load 8-bit immediate into Rd |
| `0x04` | `LD` | 3 | `Rd, addr` | Load from memory address into Rd |
| `0x05` | `ST` | 3 | `Rs, addr` | Store Rs to memory address |
| `0x06` | `ADD` | 2 | `Rd, Rs` | Rd = Rd + Rs (sets flags) |
| `0x07` | `SUB` | 2 | `Rd, Rs` | Rd = Rd − Rs (sets flags) |
| `0x08` | `CMP` | 2 | `Rd, Rs` | Compare: compute Rd − Rs, set flags only |
| `0x09` | `INC` | 2 | `Rd` | Rd = Rd + 1 |
| `0x0A` | `DEC` | 2 | `Rd` | Rd = Rd − 1 |
| `0x0B` | `AND` | 2 | `Rd, Rs` | Rd = Rd & Rs (bitwise) |
| `0x0C` | `OR` | 2 | `Rd, Rs` | Rd = Rd \| Rs (bitwise) |
| `0x0D` | `XOR` | 2 | `Rd, Rs` | Rd = Rd ^ Rs (bitwise) |
| `0x0E` | `JMP` | 2 | `addr` | Unconditional jump: PC = addr |
| `0x0F` | `JZ` | 2 | `addr` | Jump to addr if Zero flag is set |
| `0x10` | `JNZ` | 2 | `addr` | Jump to addr if Zero flag is clear |
| `0x11` | `CALL` | 2 | `addr` | Push return address (PC), then PC = addr |
| `0x12` | `RET` | 1 | — | Pop return address into PC |
| `0x13` | `PUSH` | 2 | `Rs` | Push Rs onto the stack |
| `0x14` | `POP` | 2 | `Rd` | Pop from stack into Rd |
| `0x15` | `OUT` | 2 | `Rd` | Print the decimal value of Rd |
| `0x16` | `OUTC` | 2 | `Rd` | Print Rd as an ASCII character |

### I/O Model

The CPU uses two output instructions that write directly to `stdout`:

- **`OUT Rd`** — Prints the integer value of register Rd in the format `[OUT] Rd = <value>`.
- **`OUTC Rd`** — Writes the register value as a raw ASCII byte (used for string output).

There is no input instruction in this implementation. The focus is on output-driven verification of computation.

---

## 4. Fetch-Decode-Execute Cycle

The core of the emulator is the `cpu_step()` function in `toyemu.c`. Each step performs:

```
1. FETCH:   Read memory[cpu->pc], increment PC
2. DECODE:  Switch on the opcode byte
3. FETCH OPERANDS: Read additional bytes from memory as needed
4. EXECUTE: Perform the operation (ALU, memory, control flow)
```

For control-flow instructions (`JMP`, `JZ`, `JNZ`, `CALL`, `RET`), the PC is modified directly rather than advancing sequentially. The `CALL` instruction pushes the address of the instruction *after* the call (PC after consuming the call's operand byte) onto the stack, enabling correct return with `RET`.

The execution loop (`cpu_run`) runs `cpu_step` repeatedly until `cpu->running` becomes `0` (triggered by `HLT` or an illegal opcode). A safety step limit of 10,000 prevents infinite loops during development.

---

## 5. Assembler Design

The assembler (`asm.c`) implements a classic **two-pass** algorithm:

### Pass 1 — Symbol Collection
- Scans the source file line by line.
- Strips comments (text after `;`).
- When a label is encountered (identifier followed by `:`), records the label name and the current code address counter.
- Tracks the instruction size (1, 2, or 3 bytes) to advance the address counter correctly without generating code.

### Pass 2 — Code Generation
- Re-scans the source file.
- For each instruction, validates the mnemonic and operands.
- Emits the opcode and operand bytes into the code buffer.
- When a label reference is encountered (in `JMP`, `JZ`, `JNZ`, `CALL` operands), the assembler looks up the label address. If the label was defined earlier in the file (backward reference), the address is emitted immediately. If it is a **forward reference**, a placeholder (`0xFF`) is emitted and recorded in the patch table.

### Patch Resolution
After pass 2, all entries in the patch table are resolved by looking up their label names and overwriting the placeholder bytes with the correct addresses. This enables forward jumps.

### Assembly Syntax

```
; Comments start with semicolon and extend to end of line

LABEL:                          ; Label definition
    MNEMONIC  operand1, operand2  ; Instruction (comma-separated operands)
    MNEMONIC  operand             ; Single-operand instruction
    MNEMONIC                      ; Zero-operand instruction
```

**Registers:** `R0`, `R1`, `R2`, `R3`

**Immediates:** Decimal integers (0–255), character literals (`'A'`, `'\n'`)

---

## 6. Example Programs

### 6.1 Hello World (`hello.asm`)

Outputs the string `"Hi"` followed by a newline, demonstrating character I/O via `OUTC`.

```
    MVI R0, 'H'
    OUTC R0
    MVI R0, 'i'
    OUTC R0
    MVI R0, '\n'
    OUTC R0
    HLT
```

**Expected output:** `Hi` (with newline)

### 6.2 Fibonacci (`fib.asm`)

Computes the 10th Fibonacci number using an iterative loop.

```
    MVI R0, 10      ; loop counter
    MVI R1, 0       ; F(0) = 0
    MVI R2, 1       ; F(1) = 1
LOOP:
    DEC R0
    JZ DONE
    MOV R3, R2
    ADD R2, R1
    MOV R1, R3
    JMP LOOP
DONE:
    OUT R2          ; F(10) = 55
    HLT
```

**Expected output:** `[OUT] R2 = 55`

### 6.3 Multiplication via Repeated Addition (`multiply.asm`)

Multiplies 7 × 9 by adding 7 to an accumulator 9 times.

```
    MVI R0, 7       ; multiplicand
    MVI R1, 9       ; multiplier
    MVI R2, 0       ; result
LOOP:
    CMP R1, R3      ; R3 is 0 (default initialization)
    JZ DONE
    ADD R2, R0
    DEC R1
    JMP LOOP
DONE:
    OUT R2          ; 63
    HLT
```

**Expected output:** `[OUT] R2 = 63`

### 6.4 Countdown (`countdown.asm`)

Counts down from 10 to 1, demonstrating a simple conditional loop.

```
    MVI R0, 10
LOOP:
    OUT R0
    DEC R0
    JNZ LOOP
    HLT
```

**Expected output:** Values 10 through 1 printed sequentially.

### 6.5 Sum of 1..N (`sum.asm`)

Computes the sum of integers from 1 to 10 using accumulation.

```
    MVI R0, 0       ; sum = 0
    MVI R1, 10      ; counter = 10
LOOP:
    ADD R0, R1
    DEC R1
    JNZ LOOP
    OUT R0          ; 55
    HLT
```

**Expected output:** `[OUT] R0 = 55`

### 6.6 Subroutine Call (`subroutine.asm`)

Demonstrates `CALL`/`RET` by implementing a `square(n)` function that computes n² via repeated addition, with proper register saving/restoring using `PUSH`/`POP`.

```
    MVI R0, 5       ; argument
    CALL SQUARE
    OUT R0          ; 25
    HLT

SQUARE:
    PUSH R1         ; save caller's R1
    PUSH R2         ; save caller's R2
    MOV R1, R0      ; multiplicand = n
    MOV R2, R0      ; counter = n
    MVI R0, 0       ; result = 0
SQ_LOOP:
    CMP R2, R3
    JZ SQ_DONE
    ADD R0, R1
    DEC R2
    JMP SQ_LOOP
SQ_DONE:
    POP R2          ; restore R2
    POP R1          ; restore R1
    RET
```

**Expected output:** `[OUT] R0 = 25`

---

## 7. Build and Usage

### Prerequisites

- GCC (or any C99-compatible compiler)
- GNU Make (optional — manual compilation instructions provided)

### Compilation

**With Make:**
```bash
make
```

**Without Make:**
```bash
gcc -Wall -Wextra -std=c99 -pedantic -o toyemu main.c toyemu.c asm.c
```

### Running

```bash
./toyemu examples/fib.asm
```

The program assembles the `.asm` file to machine code, loads it into the emulated CPU's memory, and executes it.

### Running All Tests

**With Make:**
```bash
make test
```

**Without Make:**
```bash
for f in examples/*.asm; do ./toyemu "$f"; done
```

### Expected Test Results

| Program | Expected Output |
|---------|----------------|
| `hello.asm` | `Hi` (with newline) |
| `fib.asm` | `[OUT] R2 = 55` |
| `multiply.asm` | `[OUT] R2 = 63` |
| `countdown.asm` | Values 10 down to 1 |
| `sum.asm` | `[OUT] R0 = 55` |
| `subroutine.asm` | `[OUT] R0 = 25` |

---

## 8. Connection to System Programming Course

This project connects to multiple topics covered in the System Programming course:

| Course Topic | Project Connection |
|-------------|-------------------|
| **Compiler Construction (HW1, HW2, HW4)** | The assembler is a mini-compiler: lexing (tokenizing mnemonics/registers), parsing (two-pass with label resolution), and code generation (opcode encoding). This mirrors the p0 and MiniLang compiler pipelines. |
| **Instruction Set Architecture (HW1)** | The CPU implements a custom ISA with opcodes, register encoding, and flag semantics — directly analogous to the p0 virtual stack machine ISA. |
| **Virtual Machines (HW2)** | The emulator is a VM: it interprets bytecode instructions using a fetch-decode-execute loop, maintaining PC, SP, and a data stack, just like the MiniLang VM. |
| **Stack & Calling Conventions (HW1, HW2)** | `CALL`/`RET`/`PUSH`/`POP` implement a stack-based activation record mechanism identical to the one documented in HW1 and HW2. |
| **Processes (HW6)** | Understanding PC, stack pointer, and register state is directly relevant to how `fork()` preserves and restores execution context. |
| **Low-level C Programming (HW5, HW6)** | The project is written entirely in C99 with manual memory management, struct-based state, and bit-level operations — reinforcing the systems programming skills practiced throughout the course. |

---

## 9. Design Decisions and Limitations

### Design Decisions

- **8-bit architecture:** Keeps the ISA small enough to fully document while remaining expressive enough for meaningful programs (loops, conditionals, subroutines).
- **Variable-length encoding:** Trade-off between code density and implementation complexity. 3-byte immediate instructions avoid squeezing operands into bit fields.
- **Two-pass assembler:** Enables forward label references (jumping to labels defined later in the source), which is essential for natural control-flow patterns.
- **Flat memory (no segments):** Simplifies the model for educational purposes. No distinction between code and data sections.
- **Safety step limit:** Prevents infinite loops during development without requiring a debugger.

### Known Limitations

- **No input instruction:** The CPU can output but not read input. Adding an `IN` instruction would require integrating with `stdin`.
- **No indirect addressing:** `LD` and `ST` use absolute addresses. Register-indirect addressing (`LD Rd, [Rs]`) is not supported.
- **4 registers only:** Complex programs may struggle with register pressure (e.g., implementing a full sorting algorithm).
- **8-bit overflow:** Operations wrap at 255. No overflow detection beyond the Carry flag.
- **No interrupt support:** The CPU runs synchronously until `HLT`.

---

## 10. What I Learned

Building ToyEmu-8 reinforced several key system programming concepts:

1. **Instruction encoding is a design exercise:** Choosing between fixed-length and variable-length encoding, allocating bits for opcodes vs. operands, and deciding which operations deserve dedicated opcodes all involve trade-offs in complexity, density, and extensibility.

2. **The fetch-decode-execute cycle is the heart of every CPU:** Implementing it directly demystified how a processor actually "runs" a program — every instruction is just a sequence of memory reads and state mutations.

3. **Two-pass assembly solves the forward-reference problem elegantly:** The assembler taught me how real assemblers handle labels defined after their first use, a problem encountered in every compiler's code generation phase.

4. **Stack discipline is universal:** The `CALL`/`RET` mechanism with saved return addresses on the stack is identical in principle to x86 `call`/`ret`, RISC-V `jal`/`jalr`, and the function call convention in the MiniLang VM.

5. **Debugging at the ISA level is humbling:** A single off-by-one error in the Fibonacci loop (using `R0=9` instead of `R0=10`) produced an unexpected result that required tracing through every iteration — a reminder that even "simple" programs demand careful reasoning when working close to the hardware.

---

## 11. References

- Silberschatz, Galvin, Gagne. *Operating System Concepts*, 10th Edition. Chapter 1 (Computer-System Architecture).
- Patterson, Hennessy. *Computer Organization and Design*, RISC-V Edition.
- Course materials: [cpu2os](https://github.com/ccc114b/cpu2os) — CPU emulation and OS concepts.
- Course HW1: p0 Compiler — Stack machine ISA and code generation.
- Course HW2: MiniLang Compiler & VM — Bytecode compilation and stack-machine execution.
- Course HW5: Threads & Synchronization — Low-level C programming with shared state (relevant to emulator design patterns).
