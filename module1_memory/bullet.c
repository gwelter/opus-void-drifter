/**
 * bullet.c - Bullet Entity Implementation
 *
 * CONCEPT: Implementation Files (.c)
 * ==================================
 * This file contains the ACTUAL CODE for functions declared in bullet.h.
 * When compiled, this becomes object code (bullet.o) that gets linked
 * with other object files to create the final executable.
 *
 * Compilation flow:
 *     bullet.c  ──compile──▶  bullet.o  ──┐
 *     list.c    ──compile──▶  list.o    ──┼──link──▶  void_drifter_m1
 *     main.c    ──compile──▶  main.o    ──┘
 */

#include "bullet.h"  // Our header (declarations we're implementing)
#include <stdio.h>   // For printf
#include <stdlib.h>  // For malloc, free

/**
 * bullet_create - Allocate and initialize a new bullet
 *
 * DEEP DIVE: malloc()
 * ===================
 * malloc(size) does the following:
 *     1. Asks the OS for 'size' bytes of heap memory
 *     2. Returns a pointer to the start of that memory
 *     3. Returns NULL if allocation fails (out of memory)
 *
 * The memory is UNINITIALIZED (contains garbage/random values).
 * Always initialize your data after malloc!
 *
 * DEEP DIVE: sizeof()
 * ===================
 * sizeof(BulletNode) returns the size IN BYTES of the struct.
 * This is computed at COMPILE TIME, not runtime.
 * Always use sizeof() rather than hardcoding sizes - struct sizes
 * can vary between systems due to alignment/padding.
 */
BulletNode* bullet_create(float x, float y, int damage) {
    // Request memory from the heap
    // sizeof(BulletNode) tells malloc exactly how many bytes we need
    BulletNode* bullet = malloc(sizeof(BulletNode));

    // CRITICAL: Always check if malloc succeeded!
    // In real games, you might have a memory pool instead of malloc.
    // For learning, we'll keep it simple.
    if (bullet == NULL) {
        // malloc failed - probably out of memory
        // In production, you'd handle this more gracefully
        fprintf(stderr, "ERROR: Failed to allocate bullet!\n");
        return NULL;
    }

    // Initialize all fields
    // Using the arrow operator (->) because bullet is a POINTER
    bullet->x = x;
    bullet->y = y;
    bullet->damage = damage;

    // IMPORTANT: Initialize pointers to NULL
    // Uninitialized pointers contain GARBAGE addresses
    // Dereferencing garbage = crash (segfault)
    bullet->next = NULL;
    bullet->prev = NULL;

    // Return the pointer - this is the "key" to our storage locker
    return bullet;
}

/**
 * bullet_destroy - Free a bullet's memory
 *
 * DEEP DIVE: free()
 * =================
 * free(ptr) does the following:
 *     1. Marks the memory at 'ptr' as available for reuse
 *     2. Does NOT change the pointer itself (still points to same address)
 *     3. Does NOT zero out the memory (data might still be readable - DANGER!)
 *
 * After free():
 *     - The pointer becomes a "dangling pointer"
 *     - Using it is UNDEFINED BEHAVIOR (might crash, might seem to work)
 *     - Best practice: Set pointer to NULL after free
 *
 * COMMON BUGS:
 *     - Double free: Calling free() twice on same pointer = crash
 *     - Use after free: Accessing memory after free() = undefined behavior
 *     - Memory leak: Forgetting to call free() = memory never released
 */
void bullet_destroy(BulletNode* bullet) {
    // Safe to call free(NULL) - it does nothing
    // This makes our destroy function safe to call on already-freed bullets
    if (bullet == NULL) {
        return;
    }

    // Optional: Zero out sensitive data before freeing
    // (Good practice for security-sensitive applications)
    bullet->x = 0;
    bullet->y = 0;
    bullet->damage = 0;
    bullet->next = NULL;
    bullet->prev = NULL;

    // Release the memory back to the heap
    free(bullet);

    // NOTE: We can't set 'bullet = NULL' here because we received
    // a COPY of the pointer (pass by value). The caller's pointer
    // still points to the now-freed memory. Caller should set to NULL.
}

/**
 * bullet_print - Display bullet info for debugging
 *
 * CONCEPT: const Correctness
 * ==========================
 * The 'const' keyword promises we won't modify the bullet.
 * This is similar to TypeScript's 'readonly' but enforced at compile time.
 *
 * Benefits:
 *     1. Self-documenting: Reader knows this function only reads
 *     2. Compiler enforced: Error if you try to modify
 *     3. Optimization: Compiler can make assumptions
 *
 * CONCEPT: %p Format Specifier
 * ============================
 * %p prints a pointer's VALUE (the memory address) in hexadecimal.
 * This lets us see WHERE in memory our bullet lives.
 *
 * Example output: 0x7f8b4c004080
 * - 0x prefix indicates hexadecimal
 * - The number is the actual RAM address
 */
void bullet_print(const BulletNode* bullet, int index) {
    if (bullet == NULL) {
        printf("[%d] (null bullet)\n", index);
        return;
    }

    // %p prints the pointer's address (where in memory this bullet lives)
    // We cast to (void*) for portability - %p expects void*
    printf("[%d] addr=%p  x=%.1f  y=%.1f  damage=%d\n",
           index,
           (void*)bullet,  // Cast to void* for %p
           bullet->x,
           bullet->y,
           bullet->damage);
}
