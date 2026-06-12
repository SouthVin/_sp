# System Programming — Final Term Portfolio

## AI Usage Declaration

All assignments this semester (hw 1–6 and midterm) were completed with AI assistance using **OpenCode (DeepSeek-V4)** and **Google Gemini**. The AI assisted with code generation, technical documentation (READMEs), and project architecture design. Assignment specifications and prompts were provided by me.

- No code was copied from other students.
- No code was copied from the internet.
- Concept design, specification definition, and testing/verification were done by me.

| Assignment | AI Tool |
|------------|---------|
| hw 1 — p0 Compiler | OpenCode |
| hw 2 — MiniLang Compiler & VM | OpenCode |
| hw 3 — Nexus Games Catalog | OpenCode |
| hw 4 — Let AI Teach You Compilers | OpenCode |
| hw 5 — Threads & Synchronization | Google Gemini |
| hw 6 — Processes & File Descriptors | OpenCode |
| Midterm — ToyEmu-8 CPU Emulator | OpenCode |

---

## Course Info

| Field | Value |
|-------|-------|
| Course | System Programming |
| Semester | 114-2 (Spring 2026) |
| Student | 康晏祥 (Kang Yan-Xiang) |
| University | National Quemoy University, CSIE |

---

## Contents

1. [hw 1 — p0 Compiler](#hw-1--p0-compiler)
2. [hw 2 — MiniLang Compiler & VM](#hw-2--minilang-compiler--vm)
3. [hw 3 — Nexus Games Catalog](#hw-3--nexus-games-catalog)
4. [hw 4 — Let AI Teach You Compilers](#hw-4--let-ai-teach-you-compilers)
5. [hw 5 — Threads & Synchronization](#hw-5--threads--synchronization)
6. [hw 6 — Processes & File Descriptors](#hw-6--processes--file-descriptors)
7. [Midterm — ToyEmu-8 CPU Emulator](#midterm--toyemu-8-cpu-emulator)

---

## hw 1 — p0 Compiler

**Topic: Compiler frontend — lexer, parser, code generation**

A recursive-descent compiler that translates a simplified C subset into virtual Stack Machine assembly. Features:

- Global and local variable declarations
- `while` loop control flow
- Function calls with arguments, activation records, and return values
- Lexical scope lookup (locals shadow globals)
- Arithmetic (`+`, `-`, `*`, `/`) and comparison (`==`)

Outputs a virtual stack machine ISA with `PUSH`, `POP`, `LOAD`, `STORE`, `ADD`, `SUB`, `CMPEQ`, `JMP`, `JZ`, `CALL`, `RET`.

**Files:** `hw 1/compiler.c`

---

## hw 2 — MiniLang Compiler & VM

**Topic: Full compiler + bytecode virtual machine**

A compiler and runtime VM for the MiniLang language. Features:

- `if-else` and `while` control flow
- `print` output statement
- Separate global/local variable storage (`STORE_GLOBAL` / `STORE_LOCAL`)
- Extended comparisons (`>`, `<`)
- Function calls with activation records and return value passing

The VM is a bytecode interpreter implementing a Fetch-Decode-Execute loop with separate Data Stack and Call Stack.

**Files:** `hw 2/MiniLang.py`

---

## hw 3 — Nexus Games Catalog

**Topic: Frontend web development — dynamic game catalog**

A vanilla HTML/CSS/JS game catalog website with modern design:

- Glassmorphism UI using `backdrop-filter` and semi-transparent backgrounds
- Dynamic DOM rendering from a JavaScript game data array
- Real-time search and category filtering (RPG / Cyberpunk / Sci-Fi)
- CSS `@keyframes` animations for staggered card entrance and floating backgrounds
- AI-generated custom game cover images

**Files:** `hw 3/video-game-catalog/` (`index.html`, `style.css`, `script.js`, `images/`)

---

## hw 4 — Let AI Teach You Compilers

**Topic: Compiler textbook (LLVM Edition)**

A 12-chapter compiler textbook written with AI assistance, covering theory to implementation:

| Chapter | Topic |
|---------|-------|
| Ch 1 | Introduction to Compilers |
| Ch 2 | Lexical Analysis (Scanning) |
| Ch 3 | Syntax Analysis (Parsing) |
| Ch 4 | Semantic Analysis |
| Ch 5 | Intermediate Representation |
| Ch 6 | Introduction to LLVM |
| Ch 7 | LLVM Intermediate Representation |
| Ch 8 | Building a Frontend with LLVM |
| Ch 9 | Code Generation with LLVM |
| Ch 10 | Compiler Optimizations |
| Ch 11 | Building a Complete Compiler |
| Ch 12 | Advanced Topics & Future Directions |

**Files:** `hw 4/` (12 `.md` chapter files)

---

## hw 5 — Threads & Synchronization

**Topic: Multithreading, race conditions, mutex, deadlock**

Theory coverage plus three C programs:

| Program | Description | Key Techniques |
|---------|-------------|----------------|
| `bank.c` | Bank deposit/withdrawal simulation | `pthread_mutex_lock` / `unlock` to prevent race conditions |
| `producer-consumer.c` | Producer-consumer problem | Mutex + condition variables (`pthread_cond_wait` / `signal`) |
| `dining-philosophers.c` | Dining philosophers problem | Resource hierarchy to prevent deadlock |

Theory: thread concepts, race condition causes, mutex mutual exclusion, condition variable synchronization, Coffman's four deadlock conditions and prevention strategies.

**Files:** `hw 5/bank.c`, `producer-consumer.c`, `dining-philosophers.c`

**Build:** `gcc -pthread -o <name> <name>.c`

---

## hw 6 — Processes & File Descriptors

**Topic: Unix process management and file descriptor operations**

Covers `fork`, `execvp`, `open/read/write/close`, `dup2` with four example programs:

| Program | Description | Key Techniques |
|---------|-------------|----------------|
| `01-fork.c` | Parent-child process creation | `fork()`, `waitpid()`, zombie prevention |
| `02-file-io.c` | File I/O operations | `open()`, `read()`, `write()`, `close()`, `lseek()` |
| `03-dup2.c` | File descriptor duplication | `dup2()` for stdout/stderr redirection and piping |
| `04-shell.c` | Minimal shell | `fork()` + `execvp()` + `dup2()` combined; supports `>`, `<`, `cd`, `exit` |

Theory: standard file descriptors (0/1/2), process duplication and image replacement, fd inheritance, shell redirection internals.

**Files:** `hw 6/01-fork.c`, `02-file-io.c`, `03-dup2.c`, `04-shell.c`

---

## Midterm — ToyEmu-8 CPU Emulator

**Topic: 8-bit CPU emulator with assembler**

A minimal 8-bit CPU emulator and two-pass assembler:

- **256-byte address space**, von Neumann architecture
- **4 general-purpose registers** (R0–R3) + PC, SP, FLAGS
- **21 instructions**: data transfer, ALU, control flow, stack operations, I/O output
- **Two-pass assembler**: Pass 1 collects labels, Pass 2 generates machine code with forward-reference backpatching
- **6 example programs**: Hello World, Fibonacci, multiplication, countdown, sum, subroutine call

See `midterm/README.md` for full details.

**Files:** `midterm/toyemu.h`, `toyemu.c`, `asm.c`, `main.c`, `Makefile`, `examples/`

---

## Summary

This semester's coursework covers the core topics of System Programming:

1. **Compiler Design (hw1, hw2, hw4)**: Frontend lexing/parsing, VM execution, textbook writing
2. **Web Frontend (hw3)**: Dynamic DOM manipulation and modern CSS
3. **Concurrent Programming (hw5)**: Multithreading, synchronization, deadlock prevention
4. **OS Process Management (hw6)**: Unix system calls, shell implementation
5. **CPU Emulation (Midterm)**: ISA design, assembler, fetch-decode-execute cycle

All assignments were completed with AI assistance. Concepts, specifications, and testing were driven by me. No code was copied from others.
