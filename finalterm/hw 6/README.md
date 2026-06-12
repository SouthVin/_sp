# Homework 6 — Processes & File Descriptors

**Topic**: `fork`, `execvp`, `close`, `open`, `read`, `write`, `dup2`, and the standard file descriptors (`stdin=0`, `stdout=1`, `stderr=2`).

---

## Table of Contents

1. [Standard File Descriptors](#1-standard-file-descriptors-0-1-2)
2. [Fork — Creating a Child Process](#2-fork--creating-a-child-process)
3. [Execvp — Replacing the Process Image](#3-execvp--replacing-the-process-image)
4. [Open, Read, Write, Close — File I/O](#4-open-read-write-close--file-io)
5. [Dup2 — Duplicating File Descriptors](#5-dup2--duplicating-file-descriptors)
6. [Putting It All Together — A Simple Shell](#6-putting-it-all-together--a-simple-shell)
7. [Programs Included](#7-programs-included)
8. [How to Build & Run](#8-how-to-build--run)

---

## 1. Standard File Descriptors (0, 1, 2)

Every Unix process starts with three open file descriptors provided by the kernel:

| FD number | Constant    | Name            | Default target |
|-----------|-------------|-----------------|----------------|
| `0`       | `STDIN_FILENO`  | Standard input  | Keyboard |
| `1`       | `STDOUT_FILENO` | Standard output | Terminal |
| `2`       | `STDERR_FILENO` | Standard error  | Terminal |

Key insight: **stdout and stderr both write to the terminal by default, but they are separate file descriptors.** This is why `2>&1` in bash merges them — it calls `dup2(1, 2)` under the hood.

```c
write(STDOUT_FILENO, "hello\n", 6);   // prints normally
write(STDERR_FILENO, "error\n", 6);   // also prints, but via fd 2
```

---

## 2. Fork — Creating a Child Process

**`pid_t fork(void)`** creates a new process by duplicating the calling process. After `fork()` returns:

- The **child** sees return value `0`.
- The **parent** sees the child's PID (> 0).
- Both processes have **separate address spaces** — variables diverge after the fork.
- The child inherits all open file descriptors from the parent.
- Returns `-1` on failure.

```
Before fork: 1 process
After fork:  2 processes (parent + child), both continuing from the same instruction
```

**Orphan vs zombie processes:**
- An **orphan** is a child whose parent died before it; it gets reparented to `init` (PID 1).
- A **zombie** is a child that exited but the parent hasn't called `wait()`; the kernel keeps its exit status around.

See: [01-fork.c](01-fork.c)

---

## 3. Execvp — Replacing the Process Image

**`int execvp(const char *file, char *const argv[])`** replaces the current process image with a new program. It does **not** create a new process — it overwrites the calling process.

Key points:
- If successful, `execvp` **never returns**. Everything after it is unreachable.
- If it fails, it returns `-1` and the caller continues.
- The `argv` array must be `NULL`-terminated (sentinel).
- `execvp` searches the `PATH` environment variable for the executable.
- Open file descriptors survive the `exec` call (unless marked `FD_CLOEXEC`).

**Variants of exec:**

| Function   | Search PATH? | Arguments passed as... |
|------------|--------------|------------------------|
| `execvp`   | Yes          | Array (vector)         |
| `execv`    | No           | Array (vector)         |
| `execlp`   | Yes          | List (variadic)        |
| `execl`    | No           | List (variadic)        |
| `execvpe`  | Yes + env    | Array + environment    |

**Common pattern:** `fork()` then `execvp()` in the child — this is how every shell launches commands.

```c
pid_t pid = fork();
if (pid == 0) {
    // Child: replace self with /bin/ls
    char *args[] = {"ls", "-l", NULL};
    execvp("ls", args);
    perror("execvp failed");   // only reached if execvp fails
    exit(1);
}
// Parent continues here
waitpid(pid, NULL, 0);
```

---

## 4. Open, Read, Write, Close — File I/O

### open — `int open(const char *pathname, int flags, mode_t mode)`

Opens a file and returns a file descriptor (the lowest unused fd number).

| Flag         | Meaning |
|--------------|---------|
| `O_RDONLY`   | Read only |
| `O_WRONLY`   | Write only |
| `O_RDWR`     | Read and write |
| `O_CREAT`    | Create file if it doesn't exist |
| `O_TRUNC`    | Truncate to length 0 on open |
| `O_APPEND`   | Writes always append to end |

### read — `ssize_t read(int fd, void *buf, size_t count)`

Reads up to `count` bytes from fd into `buf`. Returns number of bytes read, `0` on EOF, or `-1` on error.

### write — `ssize_t write(int fd, const void *buf, size_t count)`

Writes up to `count` bytes from `buf` to fd. Returns number written, or `-1` on error.

### close — `int close(int fd)`

Releases the file descriptor. The fd number becomes available for reuse (the next `open`/`dup` will use the lowest free fd).

See: [02-file-io.c](02-file-io.c)

---

## 5. Dup2 — Duplicating File Descriptors

**`int dup2(int oldfd, int newfd)`** makes `newfd` refer to the same open file description as `oldfd`.

- If `newfd` was already open, it is silently closed first.
- After the call, both `oldfd` and `newfd` share the same file offset and status flags.
- Returns `newfd` on success, `-1` on error.

**This is the mechanism behind shell redirection:**

```bash
command > file.txt    # dup2(fd, STDOUT_FILENO)
command < input.txt   # dup2(fd, STDIN_FILENO)
command 2>&1          # dup2(STDOUT_FILENO, STDERR_FILENO)
```

```c
int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
dup2(fd, STDOUT_FILENO);   // fd 1 now points to output.txt
close(fd);                  // the original fd is no longer needed
printf("this goes to the file\n");
```

See: [03-dup2.c](03-dup2.c)

---

## 6. Putting It All Together — A Simple Shell

The program `04-shell.c` implements a minimal shell (`>> ` prompt) that demonstrates all concepts:

1. **fork()** — creates a child to execute each command.
2. **execvp()** — replaces the child process with the requested program.
3. **dup2()** — redirects stdin/stdout when `>` or `<` appears in the command.
4. **waitpid()** — the parent waits for the child to finish.
5. All built on top of **open/close** and the **standard file descriptors**.

Supported features:
- Run any command: `ls -la`
- Output redirection: `ls > output.txt`
- Input redirection: `wc < input.txt`
- Built-in `cd` command
- Built-in `exit` command

See: [04-shell.c](04-shell.c)

---

## 7. Programs Included

| File | Demonstrates |
|------|-------------|
| `01-fork.c` | `fork()`, parent vs child, `waitpid()`, zombie prevention, orphan processes |
| `02-file-io.c` | `open()`, `read()`, `write()`, `close()`, `lseek()`, working with stdin/stdout/stderr |
| `03-dup2.c` | `dup2()` for stdout redirection, stderr redirection, and piping between processes |
| `04-shell.c` | `fork()` + `execvp()` + `dup2()` combined into a working shell with I/O redirection |

---

## 8. How to Build & Run

```bash
# Compile all programs
gcc -o 01-fork 01-fork.c
gcc -o 02-file-io 02-file-io.c
gcc -o 03-dup2 03-dup2.c
gcc -o 04-shell 04-shell.c

# Run individual examples
./01-fork
./02-file-io
./03-dup2

# Start the interactive shell
./04-shell
```

---

## References

- `man 2 fork`
- `man 3 execvp`
- `man 2 open`
- `man 2 read`
- `man 2 write`
- `man 2 close`
- `man 2 dup2`
- Advanced Programming in the UNIX Environment (Stevens)
