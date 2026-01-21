# Void Drifter: A Systems Programming Course for Web Developers

```
    *  .  *       VOID DRIFTER       *  .  *
         *    From JavaScript to C    *
    .  *     A 5-Module Journey    .  *  .
```

## Course Overview

Welcome, JavaScript/TypeScript developer. You're about to leave the comfortable world
of garbage collection, npm packages, and V8's protective embrace. In C, **you** are
the garbage collector. **You** manage the memory. **You** decide when things live and die.

This course builds **Void Drifter**, a networked 2D space shooter, teaching you:
- Manual memory management (malloc/free)
- Pointers and structs (the DNA of C)
- Procedural asset generation (no external files)
- Function pointers (C's version of callbacks)
- Socket programming (raw TCP/UDP)
- Multithreading (pthreads)

## Prerequisites

- Raylib 4.5+ installed (`brew install raylib` on macOS)
- A C11 compiler (clang or gcc)
- Basic understanding of loops, conditionals, functions
- Willingness to see segfaults (they're learning opportunities)

## Module Structure

```
module1_memory/       # The Heap, Structs, Manual Memory
module2_raylib/       # The Raylib Loop & Procedural Art
module3_polymorphism/ # Architecture via Function Pointers
module4_networking/   # Sockets & Binary Protocols
module5_concurrency/  # Threads & Mutexes (Final Game)
```

## The Mental Model Shift

### In JavaScript:
```javascript
let bullets = [];
bullets.push({ x: 10, y: 20 });  // V8 handles memory
bullets = [];                     // Garbage collector cleans up
```

### In C:
```c
Bullet* bullet = malloc(sizeof(Bullet));  // YOU request memory
bullet->x = 10;
bullet->y = 20;
free(bullet);                              // YOU release it (or leak forever)
```

## Building Each Module

Each module has its own Makefile. To build:

```bash
cd module1_memory
make
./void_drifter_m1
```

## A Note on Valgrind

Throughout this course, we'll reference Valgrind - a tool that detects memory leaks.
On macOS with Apple Silicon, use `leaks` instead:

```bash
# Linux
valgrind --leak-check=full ./your_program

# macOS
leaks --atExit -- ./your_program
```

## The Journey Ahead

| Module | Concept | JS Parallel | Deliverable |
|--------|---------|-------------|-------------|
| 1 | malloc/free | new/GC | CLI bullet allocator |
| 2 | Raylib basics | Canvas API | Moving spaceship |
| 3 | Function pointers | Callbacks | Weapon switching |
| 4 | Sockets | WebSockets | Client/Server |
| 5 | Pthreads | Web Workers | Full networked game |

Let's begin. Module 1 awaits.
