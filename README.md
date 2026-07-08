# OS-Essentials

A collection of systems-programming projects built from scratch in C/C++ to explore how an operating system actually works under the hood — process execution, ELF loading, demand paging, multithreading, and CPU scheduling.

Each folder is a **standalone project** with its own build system. No external OS-level dependencies beyond a POSIX environment (Linux) and a standard GCC/G++ toolchain.

---

## Table of Contents

- [Overview](#overview)
- [Repository Structure](#repository-structure)
- [Projects](#projects)
  - [1. Simple Shell](#1-simple-shell)
  - [2. Simple Loader](#2-simple-loader)
  - [3. Simple Smart Loader](#3-simple-smart-loader)
  - [4. Simple Multithreader](#4-simple-multithreader)
  - [5. Simple Scheduler](#5-simple-scheduler)
- [Concepts Covered](#concepts-covered)
- [Requirements](#requirements)
- [License](#license)

---

## Overview

Modern operating systems hide an enormous amount of complexity behind a few simple abstractions: `exec()`, `malloc()`, threads, and the scheduler. **OS-Essentials** peels that back one layer at a time by re-implementing core pieces of that machinery in userspace:

| # | Project | What it teaches you |
|---|---------|---------------------|
| 1 | Simple Shell | Process creation, `fork`/`exec`, pipes, command history |
| 2 | Simple Loader | ELF binary format, program headers, manual segment loading |
| 3 | Simple Smart Loader | Demand paging via `SIGSEGV`, lazy segment loading, internal fragmentation |
| 4 | Simple Multithreader | POSIX threads, `parallel_for` abstractions, workload partitioning |
| 5 | Simple Scheduler | Round-robin CPU scheduling, `SIGSTOP`/`SIGCONT`, IPC via pipes |

---

## Repository Structure

```
OS-Essentials/
├── LICENSE
├── README.md
│
├── Simple_Loader/
│   ├── Makefile
│   ├── launcher/
│   │   ├── launch.c
│   │   └── Makefile
│   ├── loader/
│   │   ├── loader.c
│   │   ├── loader.h
│   │   └── Makefile
│   └── test/
│       ├── fib.c
│       └── Makefile
│
├── Simple_Multithreader/
│   ├── Makefile
│   ├── matrix.cpp
│   ├── simple-multithreader.h
│   └── vector.cpp
│
├── Simple_Scheduler/
│   ├── dummy_main.h
│   ├── fib.c
│   ├── header.h
│   ├── Makefile
│   ├── scheduler.c
│   └── shell.c
│
├── Simple_Shell/
│   └── shell.c
│
└── Simple_Smart_Loader/
    ├── Makefile
    ├── launcher/
    │   ├── launcher.c
    │   └── Makefile
    ├── loader/
    │   ├── loader.c
    │   ├── loader.h
    │   └── Makefile
    └── test/
        ├── Makefile
        └── sum.c
```

---

## Projects

### 1. Simple Shell

A minimal Unix-like command-line shell written in C.

**Features**
- Interactive REPL (`SimpleShell:$`) via `fork()` + `execvp()`
- Piped command execution (`cmd1 | cmd2 | ... | cmdN`) using chained `pipe()`/`dup2()`
- Command history with PID, start time, end time, and execution duration
- Built-in `history` and `exit` commands

**Run it**
```bash
cd Simple_Shell
gcc shell.c -o shell
./shell
```

**Try**
```
SimpleShell:$ ls -la | grep .c
SimpleShell:$ history
SimpleShell:$ exit
```

---

### 2. Simple Loader

A user-space **ELF loader** that parses a 32-bit ELF executable and runs it manually — no `exec()` involved.

**How it works**
1. Reads the target binary into memory with `read()`
2. Validates the ELF magic number (`0x7F 'E' 'L' 'F'`)
3. Walks the Program Header Table (`Elf32_Phdr`) to find every `PT_LOAD` segment
4. `mmap()`s memory for each loadable segment and `memcpy()`s its contents in eagerly
5. Locates the segment containing the entry point (`e_entry`)
6. Casts the mapped address to a function pointer and **calls it directly**, capturing the return value

This is *eager* loading — every byte of every segment is mapped and copied up front, whether the program touches it or not (contrast with the Smart Loader below).

**Build & run**
```bash
cd Simple_Loader
make
./launcher/launcher test/fib
```

**Test payload:** `test/fib.c` — a freestanding `_start()` that computes `fib(40)` recursively with no libc, compiled as a static 32-bit non-PIE binary so its layout is simple enough for the loader to parse by hand.

---

### 3. Simple Smart Loader

An evolution of the Simple Loader that implements **demand paging** — segments are only mapped into memory when the program actually touches them, one page at a time, exactly like a real OS.

**How it works**
1. Parses the ELF header and program headers the same way as the Simple Loader
2. Instead of eagerly mapping every segment, it installs a custom `SIGSEGV` handler via `sigaction()`
3. Jumps straight to the entry point address — which isn't mapped in memory yet
4. The first touch of that address **faults**; the handler:
   - Rounds the faulting address down to its containing page (`& ~(PAGE_SIZE - 1)`)
   - Finds which `PT_LOAD` segment owns that address
   - `mmap()`s exactly one 4 KB page at that address (`MAP_FIXED`)
   - Copies only the relevant bytes from the ELF file into that page
   - Returns, letting execution resume as if nothing happened
5. Repeats for every new page the program touches, until it finishes

**Instrumentation** — on exit, the loader reports:
- Total number of page faults handled
- Total number of pages allocated
- **Internal fragmentation**: wasted space (in KB) across all allocated pages, i.e. `(pages allocated × 4096) − (bytes actually used by segments)`

**Build & run**
```bash
cd Simple_Smart_Loader
make
./launcher/launcher test/sum
```

**Test payload:** `test/sum.c` — a freestanding program with a 4 KB-aligned 4096-int array, forcing the loader to fault in multiple pages and giving you something measurable to inspect in the fragmentation stats.

---

### 4. Simple Multithreader

A lightweight `parallel_for` abstraction built directly on top of POSIX threads (`pthread.h`) — no OpenMP, no TBB, just raw threads and a lambda-friendly API.

**API**
```cpp
// 1D parallel loop (e.g. vector operations)
parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads);

// 2D parallel loop (e.g. matrix operations)
parallel_for(int low1, int high1, int low2, int high2,
             std::function<void(int, int)> &&lambda, int numThreads);
```

Internally, the iteration range is split as evenly as possible across `numThreads` threads (with any remainder distributed to the earliest threads), each thread runs its slice of the loop body, and the caller blocks until every thread has joined. Both variants report their own wall-clock execution time.

**Included benchmarks**
- `vector.cpp` — parallel element-wise vector addition
- `matrix.cpp` — parallel matrix initialization + parallel matrix multiplication

**Build & run**
```bash
cd Simple_Multithreader
make
./vector 4          # 4 threads, default size
./matrix 4 512       # 4 threads, 512x512 matrices
```

---

### 5. Simple Scheduler

A cooperative, multi-core **round-robin process scheduler** that sits behind a modified shell, using `SIGSTOP`/`SIGCONT` to time-slice arbitrary child processes across a configurable number of virtual CPUs.

**Architecture**
```
                    ┌──────────────┐   pipe (PIDs)   ┌────────────────┐
   submit <cmd> ──▶ │  Shell       │ ──────────────▶ │  Scheduler     │
                    │ (shell.c)    │                  │ (scheduler.c)  │
                    └──────────────┘                  └────────────────┘
                          │                                    │
                          │ SIGCHLD handler                    │ SIGSTOP / SIGCONT
                          ▼                                    ▼
                  reports terminated PIDs           dispatches jobs to NCPU cores
                  back to scheduler (negative PID)   in round-robin time slices
```

**Shell responsibilities** (`shell.c`)
- Forks the scheduler process at startup, wiring up a pipe as the communication channel
- `submit <command>` forks the target command and immediately reports its PID to the scheduler (does **not** wait for it — that's the scheduler's job)
- A `SIGCHLD` handler (`report_terminated_jobs`) reaps terminated children with `waitpid(WNOHANG)` and reports their termination (as a negative PID) back to the scheduler

**Scheduler responsibilities** (`scheduler.c`)
- Maintains a circular ready queue of PIDs
- Each time slice: `SIGSTOP`s every currently running process and re-enqueues it, then dispatches the next `NCPU` ready processes with `SIGCONT`
- Tracks arrival time, wait time, and completion time per job
- Detects shell shutdown + an empty run/ready queue to know when to exit, then prints a final job summary (completion time and wait time per job, in time slices)

**Sample test payload:** `fib.c` (this project's variant) — computes `fib(40)` but includes `dummy_main.h`, which `#define`s `main` to `dummy_main` and installs a *real* `main()` that immediately `raise(SIGSTOP)`s itself before calling into the program logic. This lets the scheduler take control of the process the instant it's forked, before it does any real work.

**Build & run**
```bash
cd Simple_Scheduler
make
./shell <NCPU> <TimeQuanta>
```
```
Scheduler Created PID: 12345
SimpleShell:$ submit ./fib
SimpleShell:$ history
```

---

## Concepts Covered

- **Process management** — `fork`, `exec`, `wait`/`waitpid`, zombie/orphan handling
- **Inter-process communication** — anonymous pipes, `dup2` redirection, PID messaging
- **Signals** — `SIGSTOP`/`SIGCONT` for preemption, `SIGCHLD` for reaping, `SIGSEGV` for demand paging
- **ELF binary format** — ELF header, program header table, `PT_LOAD` segments, entry points
- **Virtual memory** — `mmap`/`munmap`, page alignment, page faults, internal fragmentation
- **Multithreading** — POSIX threads, workload partitioning, lambda-based parallel loops
- **CPU scheduling** — round-robin dispatch, ready queues, wait-time/turnaround-time accounting

---

## Requirements

- Linux (or WSL on Windows) — signal-based demand paging and `SIGSTOP`/`SIGCONT` scheduling rely on POSIX semantics
- GCC / G++ with 32-bit support (`gcc-multilib` / `g++-multilib` for the loader projects, since they operate on `Elf32_*` structures)
- GNU Make
- `pthread` (bundled with glibc on virtually every distro)

```bash
# Debian/Ubuntu
sudo apt install build-essential gcc-multilib g++-multilib
```

---

## License

MIT License