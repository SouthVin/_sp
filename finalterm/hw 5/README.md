# Homework 5: Threads, Race Conditions, Mutex, and Deadlock

---

## Table of Contents

1. [Threads](#1-threads)
2. [Race Conditions](#2-race-conditions)
3. [Mutex](#3-mutex)
4. [Deadlock](#4-deadlock)
5. [Program 1: Bank Deposit/Withdrawal Simulation](#5-program-1-bank-depositwithdrawal-simulation)
6. [Program 2: Producer-Consumer Problem](#6-program-2-producer-consumer-problem)
7. [Program 3: Dining Philosophers Problem](#7-program-3-dining-philosophers-problem)

---

## 1. Threads

A **thread** is the smallest unit of execution within a process. A single process can contain multiple threads, all sharing the same address space (code, data, heap) but each having its own stack and register state (program counter, stack pointer, etc.).

### Key Concepts

| Concept | Description |
|---------|-------------|
| **Lightweight** | Threads are cheaper to create and switch between than processes, since they share the same address space. |
| **Shared Memory** | All threads in a process share global variables, heap memory, and file descriptors. |
| **Independent Execution** | Each thread has its own program counter, stack, and set of registers. |
| **Concurrency** | Threads can run concurrently on multiple CPU cores (true parallelism) or interleaved on a single core (time-slicing). |

### POSIX Threads (pthreads)

On Linux/Unix systems, the POSIX thread library (`pthread`) provides a standard API:

```c
#include <pthread.h>

pthread_t thread;
pthread_create(&thread, NULL, thread_function, arg);
pthread_join(thread, NULL);  // Wait for thread to finish
```

---

## 2. Race Conditions

A **race condition** occurs when two or more threads access shared data concurrently and at least one thread modifies the data, but the final outcome depends on the exact timing (interleaving) of their execution.

### Example — Without Synchronization

```c
int balance = 1000;

// Thread A                       // Thread B
int tmp = balance;  // 1000       int tmp = balance;  // 1000
tmp = tmp + 500;                  tmp = tmp - 500;
balance = tmp;     // 1500       balance = tmp;      // 500
```

If both threads read `balance` before either writes, one update is **lost**. The final result depends on which thread writes last — a classic race condition.

### Consequences

- Data corruption
- Lost updates
- Non-deterministic behavior
- Difficult-to-reproduce bugs

---

## 3. Mutex

A **mutex** (mutual exclusion) is a synchronization primitive that ensures only one thread can enter a critical section at a time.

### How It Works

A mutex acts like a lock:
1. Thread `locks` the mutex before entering the critical section.
2. If another thread tries to lock an already-locked mutex, it *blocks* (waits) until the mutex is unlocked.
3. Thread `unlocks` the mutex when leaving the critical section.

### POSIX Mutex API

```c
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_lock(&mutex);
// --- critical section ---
pthread_mutex_unlock(&mutex);
```

### Corrected Bank Example

```c
// Thread A                       // Thread B
pthread_mutex_lock(&lock);        pthread_mutex_lock(&lock);  // blocked until A unlocks
int tmp = balance;                int tmp = balance;
tmp = tmp + 500;                  tmp = tmp - 500;
balance = tmp;                    balance = tmp;
pthread_mutex_unlock(&lock);      pthread_mutex_unlock(&lock);
```

Now the operations are **atomic** — the result is always correct regardless of scheduling.

### Condition Variables

Sometimes threads need to wait for a *condition* (e.g., buffer not empty). **Condition variables** allow threads to sleep until another thread signals that the condition may be true.

```c
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Wait for condition
pthread_mutex_lock(&mutex);
while (!condition) {
    pthread_cond_wait(&cond, &mutex);  // atomically unlocks mutex and sleeps
}
// condition is now true, mutex is re-locked
pthread_mutex_unlock(&mutex);

// Signal a waiting thread
pthread_cond_signal(&cond);
```

`pthread_cond_wait` **atomically** releases the mutex and puts the thread to sleep. When awakened, it re-acquires the mutex before returning. The `while` loop (not `if`) protects against spurious wakeups.

---

## 4. Deadlock

A **deadlock** is a situation where two or more threads are each waiting for a resource held by another, creating a cycle of waiting that can never be resolved.

### Four Necessary Conditions (Coffman)

1. **Mutual Exclusion** — Resources cannot be shared.
2. **Hold and Wait** — Threads hold resources while waiting for others.
3. **No Preemption** — Resources cannot be forcibly taken away.
4. **Circular Wait** — A cycle exists: A waits for B, B waits for C, ..., Z waits for A.

All four conditions must hold for a deadlock to occur.

### Classic Example: Dining Philosophers

Five philosophers sit at a round table. Each needs **two forks** (left and right) to eat. If all philosophers pick up their left fork simultaneously, each waits forever for the right fork — deadlock.

### Deadlock Prevention Strategies

| Strategy | Description |
|----------|-------------|
| **Resource Hierarchy** | Assign a global order to resources; always acquire in ascending order. Breaks circular wait. |
| **Limit Holders** | Limit the number of threads allowed to hold resources simultaneously. |
| **Try-Lock** | Use `pthread_mutex_trylock()` and release all held locks if a lock cannot be acquired. |
| **Avoid Hold-and-Wait** | Require threads to request all resources at once before starting. |

---

## 5. Program 1: Bank Deposit/Withdrawal Simulation

**File:** `bank.c`

This program simulates a single bank account balance. One person performs 100,000 deposit operations (+1 each) and 100,000 withdrawal operations (-1 each), for a total of 200,000 operations. The final balance should be **0**.

- Uses two threads: one for deposits, one for withdrawals.
- A **mutex** protects the shared `balance` variable.
- Prints the final balance to verify correctness.

### Compile and Run

```bash
gcc -pthread -o bank bank.c && ./bank
```

**Expected output:** `Final balance: 0`

---

## 6. Program 2: Producer-Consumer Problem

**File:** `producer-consumer.c`

The **producer-consumer problem** (also known as the **bounded buffer problem**) involves two types of threads:
- **Producer** threads generate data items and place them into a fixed-size buffer.
- **Consumer** threads remove and process items from the buffer.

The constraints are:
- A producer must block (wait) if the buffer is **full**.
- A consumer must block if the buffer is **empty**.

This program uses:
- A **mutex** to protect the buffer and counter.
- Two **condition variables**: `not_full` (signaled when a slot becomes available) and `not_empty` (signaled when an item is produced).

### Compile and Run

```bash
gcc -pthread -o producer-consumer producer-consumer.c && ./producer-consumer
```

---

## 7. Program 3: Dining Philosophers Problem

**File:** `dining-philosophers.c`

The **dining philosophers problem** (Dijkstra, 1965) models five philosophers who alternate between **thinking** and **eating**. Each requires two forks (left and right) to eat. The goal is to design a solution that prevents **deadlock** and **starvation**.

### Solution Strategy: Resource Hierarchy

Each fork is numbered 0–4. A philosopher always picks up the **lower-numbered** fork first. This breaks the circular wait condition — at least one philosopher will be able to acquire both forks and eat.

### Compile and Run

```bash
gcc -pthread -o dining-philosophers dining-philosophers.c && ./dining-philosophers
```

Each philosopher eats several times. The program prints the state transitions so you can verify there is no deadlock.

---

### References

- Silberschatz, Galvin, Gagne. *Operating System Concepts*, 10th Edition. Chapters 4–7.
- Stevens, Rago. *Advanced Programming in the UNIX Environment*, 3rd Edition. Chapter 11.
- Linux System Programming — Threads: [gemini.google.com/share/f95749239b50](https://gemini.google.com/share/f95749239b50)
- POSIX Threads Programming (LLNL): [https://hpc-tutorials.llnl.gov/posix/](https://hpc-tutorials.llnl.gov/posix/)
