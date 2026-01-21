# Module 1: The Heap, Structs, and Manual Memory

```
┌─────────────────────────────────────────────────────────────────┐
│  "In JavaScript, memory is someone else's problem.             │
│   In C, memory is YOUR problem."                               │
└─────────────────────────────────────────────────────────────────┘
```

## Learning Goals

By the end of this module, you will understand:
1. The difference between Stack (automatic) and Heap (allocated) memory
2. How to use `malloc()` and `free()` correctly
3. How to implement a Doubly Linked List (replacing JS arrays)
4. How to detect memory leaks

---

## Concept 1: Storage Duration (Where Does Data Live?)

### The JavaScript Mental Model (Wrong for C)

In JS, you never think about WHERE your data lives:

```javascript
function fire() {
    let bullet = { x: 100, y: 200 };  // Where is this? Who knows!
    return bullet;                     // JS figures it out
}
```

### The C Reality: Two Types of Memory

```
┌─────────────────────────────────────────────────────────────────┐
│                        PROGRAM MEMORY                           │
├─────────────────────────────────────────────────────────────────┤
│  STACK (Automatic Storage)          │  HEAP (Allocated Storage) │
│  ─────────────────────────          │  ──────────────────────── │
│  • Fast allocation                  │  • Slower allocation      │
│  • Fixed size (few MB)              │  • Large (GBs available)  │
│  • Auto-cleanup on function exit    │  • YOU must free()        │
│  • Local variables live here        │  • malloc() lives here    │
│                                     │                           │
│  int x = 5;      ← Stack            │  malloc(100) → Heap       │
│  char name[20];  ← Stack            │                           │
└─────────────────────────────────────────────────────────────────┘
```

### The Storage Locker Analogy

Think of `malloc()` as renting a storage locker:

```c
// You go to the storage facility (the Heap)
// and request a locker big enough for a Bullet
Bullet* b = malloc(sizeof(Bullet));

// malloc returns the LOCKER NUMBER (memory address)
// This is your KEY (pointer) to access the locker

// When you're done, you MUST return the key
free(b);  // "I no longer need locker #0x7ff123456"

// If you forget to free(), you've lost the key
// but the locker is still rented in your name FOREVER
// This is a MEMORY LEAK
```

### Why Not Just Use the Stack?

```c
Bullet* create_bullet_WRONG() {
    Bullet b = { .x = 100, .y = 200 };  // Lives on stack
    return &b;  // DANGER: Returning address of stack memory!
}   // b is DESTROYED here. The address now points to garbage.

Bullet* create_bullet_RIGHT() {
    Bullet* b = malloc(sizeof(Bullet));  // Lives on heap
    b->x = 100;
    b->y = 200;
    return b;  // Safe: Heap memory persists until free()
}
```

---

## Concept 2: Structs (C's "Objects")

### JavaScript Object vs C Struct

```javascript
// JavaScript: Dynamic, flexible, runtime-typed
let bullet = {
    x: 100,
    y: 200,
    damage: 10,
    owner: "player1"  // Can add anything!
};
```

```c
// C: Fixed layout, compile-time typed, exact memory size
typedef struct {
    float x;        // 4 bytes at offset 0
    float y;        // 4 bytes at offset 4
    int damage;     // 4 bytes at offset 8
    // No "owner" - struct is EXACTLY this and nothing more
} Bullet;           // Total: 12 bytes (probably, see padding below)
```

### Memory Layout Matters

```
Bullet struct in memory (assuming 4-byte alignment):

Address:    0x1000   0x1004   0x1008
           ┌────────┬────────┬────────┐
           │   x    │   y    │ damage │
           │ (float)│ (float)│ (int)  │
           └────────┴────────┴────────┘
             4 bytes  4 bytes  4 bytes = 12 bytes total
```

### The Arrow Operator: Your New Best Friend

```c
// When you have a POINTER to a struct, use ->
Bullet* b = malloc(sizeof(Bullet));
b->x = 100;      // Arrow: "follow the pointer, then access .x"
b->y = 200;

// This is shorthand for:
(*b).x = 100;    // Dereference, then dot-access (ugly!)

// When you have the struct itself (not a pointer), use .
Bullet local = { .x = 50, .y = 50 };
local.x = 100;   // Dot: direct access
```

---

## Concept 3: Doubly Linked Lists (Replacing JS Arrays)

### Why Not Just Use Arrays?

In JS, arrays grow dynamically:
```javascript
let bullets = [];
bullets.push(b1);  // Array grows automatically
bullets.splice(2, 1);  // Easy removal from middle
```

In C, arrays are FIXED SIZE:
```c
Bullet bullets[100];  // Can NEVER hold more than 100
// Removing from the middle? Move everything after it.
// That's O(n) and painful.
```

### The Linked List Solution

Each node contains:
1. The actual data (our Bullet)
2. A pointer to the NEXT node
3. A pointer to the PREVIOUS node (doubly linked)

```
┌──────────────────────────────────────────────────────────────────┐
│                     DOUBLY LINKED LIST                           │
│                                                                  │
│   HEAD                                                    TAIL   │
│    │                                                       │     │
│    ▼                                                       ▼     │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐       │
│  │ Bullet1 │───▶│ Bullet2 │───▶│ Bullet3 │───▶│ Bullet4 │       │
│  │         │◀───│         │◀───│         │◀───│         │       │
│  │ prev:NULL│    │ prev:B1 │    │ prev:B2 │    │ prev:B3 │       │
│  │ next:B2 │    │ next:B3 │    │ next:B4 │    │ next:NULL│       │
│  └─────────┘    └─────────┘    └─────────┘    └─────────┘       │
│                                                                  │
│  Benefits:                                                       │
│  • O(1) insertion at head/tail                                  │
│  • O(1) removal if you have the node pointer                    │
│  • No size limit (until you run out of RAM)                     │
└──────────────────────────────────────────────────────────────────┘
```

### Recursive Struct Definition

```c
// The struct refers to ITSELF via pointers
// This is how we chain nodes together
typedef struct BulletNode {
    float x;
    float y;
    int damage;

    struct BulletNode* next;  // Points to another BulletNode
    struct BulletNode* prev;  // Points to another BulletNode
} BulletNode;
```

---

## Concept 4: Pass-by-Value vs Pass-by-Reference

### JavaScript's Behavior

```javascript
// Objects are passed by reference (sort of)
function modifyBullet(b) {
    b.x = 999;  // Modifies the original!
}
```

### C's Behavior: Everything is Pass-by-Value

```c
// C copies EVERYTHING by default
void modify_bullet_BROKEN(Bullet b) {
    b.x = 999;  // Modifies a COPY, original unchanged!
}

// To modify the original, pass a POINTER
void modify_bullet_WORKS(Bullet* b) {
    b->x = 999;  // Follows pointer to original, modifies it!
}
```

### Why Linked List Operations Need Pointers-to-Pointers

```c
// If we want to modify WHERE head points...
void add_to_head_BROKEN(BulletNode* head, BulletNode* new_node) {
    head = new_node;  // Only modifies local copy of 'head'!
}

// We need a pointer TO the pointer
void add_to_head_WORKS(BulletNode** head, BulletNode* new_node) {
    *head = new_node;  // Dereference to modify the ACTUAL head pointer
}
```

---

## Concept 5: Memory Leaks and Valgrind

### The Leak

```c
void game_loop() {
    while (running) {
        Bullet* b = malloc(sizeof(Bullet));  // Allocate
        // ... use bullet ...
        // OOPS: Forgot to free(b)!
        // Every loop iteration leaks ~12 bytes
        // After 1 million frames: 12MB leaked
    }
}
```

### Detecting with Valgrind (Linux) or Leaks (macOS)

```bash
# Valgrind output for a leak:
==12345== LEAK SUMMARY:
==12345==    definitely lost: 12,000,000 bytes in 1,000,000 blocks

# macOS leaks tool:
leaks Report:
    1000000 leaks for 12000000 total leaked bytes.
```

### The Destructor Pattern

```c
// Always pair malloc with a "destructor" that frees
BulletNode* bullet_create(float x, float y) {
    BulletNode* b = malloc(sizeof(BulletNode));
    // ... initialize ...
    return b;
}

void bullet_destroy(BulletNode* b) {
    free(b);  // Release the storage locker
}

// For a whole list, iterate and free each node
void list_destroy(BulletList* list) {
    BulletNode* current = list->head;
    while (current != NULL) {
        BulletNode* next = current->next;  // Save next BEFORE freeing!
        free(current);
        current = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}
```

---

## The Deliverable

A CLI program demonstrating:
1. Firing bullets (malloc)
2. Listing bullets (traversing the linked list)
3. Resetting (freeing all memory)
4. Printing memory addresses to prove heap allocation

```
$ ./void_drifter_m1
====== VOID DRIFTER: Memory Module ======
Commands: [f]ire, [l]ist, [r]eset, [q]uit

> f
FIRED! Bullet allocated at 0x7f8b4c004080 (Heap)

> f
FIRED! Bullet allocated at 0x7f8b4c0040a0 (Heap)

> l
--- Bullet List (2 bullets) ---
[0] addr=0x7f8b4c004080  x=400.0  y=300.0  damage=10
[1] addr=0x7f8b4c0040a0  x=400.0  y=300.0  damage=10

> r
RESET! Freed 2 bullets. Memory returned to OS.

> l
--- Bullet List (0 bullets) ---
(empty)

> q
Goodbye. Final leak check: 0 bytes leaked.
```

---

## File Structure

```
module1_memory/
├── main.c          # Entry point, CLI loop
├── bullet.h        # BulletNode struct definition
├── bullet.c        # Bullet creation/destruction
├── list.h          # BulletList struct and operations
├── list.c          # List implementation
└── Makefile        # Build configuration
```

Now let's look at the code!
