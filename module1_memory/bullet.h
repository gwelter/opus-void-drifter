/**
 * bullet.h - Bullet Entity Definition
 *
 * CONCEPT: Header Files (.h) in C
 * ================================
 * In JavaScript, you might write:
 *     export class Bullet { ... }
 *
 * In C, we separate DECLARATION (what exists) from DEFINITION (how it works):
 *     - .h files: Declarations (like TypeScript .d.ts files)
 *     - .c files: Definitions (the actual implementation)
 *
 * This separation allows:
 *     1. Multiple .c files to share the same definitions
 *     2. The compiler to check types without seeing implementation
 *     3. Faster compilation (only recompile changed .c files)
 */

#ifndef BULLET_H  // Include guard: prevents double-inclusion
#define BULLET_H  // If BULLET_H not defined, define it and include this file

#include <stddef.h>  // For NULL

/**
 * CONCEPT: typedef struct
 * =======================
 * In C, 'struct BulletNode' is the full type name.
 * 'typedef' creates an alias so we can just write 'BulletNode'.
 *
 * Without typedef:
 *     struct BulletNode* bullet = malloc(...);
 *
 * With typedef:
 *     BulletNode* bullet = malloc(...);
 *
 * Much cleaner, similar to how you'd use types in TypeScript.
 */

/**
 * BulletNode - A single bullet in our doubly linked list
 *
 * MEMORY LAYOUT (assuming 4-byte alignment on most systems):
 * ┌─────────┬─────────┬─────────┬─────────┬─────────┐
 * │    x    │    y    │  damage │  *next  │  *prev  │
 * │ 4 bytes │ 4 bytes │ 4 bytes │ 8 bytes │ 8 bytes │
 * └─────────┴─────────┴─────────┴─────────┴─────────┘
 * Total: 28 bytes (but likely padded to 32 for alignment)
 *
 * NOTE: Pointer sizes are 8 bytes on 64-bit systems, 4 bytes on 32-bit.
 */
typedef struct BulletNode {
    // Bullet data
    float x;        // X position in game world
    float y;        // Y position in game world
    int damage;     // Damage dealt on hit

    // Linked list pointers - THIS IS THE KEY CONCEPT
    // Each node knows its neighbors, forming a chain
    struct BulletNode* next;  // Pointer to next bullet (or NULL if last)
    struct BulletNode* prev;  // Pointer to previous bullet (or NULL if first)

    // WHY 'struct BulletNode*' instead of 'BulletNode*'?
    // At this point in parsing, the typedef isn't complete yet.
    // We must use the full 'struct BulletNode' name for self-reference.
} BulletNode;

/**
 * bullet_create - Allocate and initialize a new bullet
 *
 * CONCEPT: Factory Functions
 * ==========================
 * In JS, you might use: new Bullet(x, y, damage)
 * In C, we create "factory functions" that call malloc internally.
 *
 * @param x       Initial X position
 * @param y       Initial Y position
 * @param damage  Damage value
 * @return        Pointer to newly allocated BulletNode, or NULL if malloc fails
 *
 * CALLER'S RESPONSIBILITY: Must call bullet_destroy() when done!
 */
BulletNode* bullet_create(float x, float y, int damage);

/**
 * bullet_destroy - Free a bullet's memory
 *
 * CONCEPT: Destructor Functions
 * =============================
 * Since C has no garbage collector, every malloc needs a matching free.
 * We wrap free() in a "destructor" function for:
 *     1. Consistency (pair create/destroy)
 *     2. Future-proofing (might need cleanup logic later)
 *     3. Safety (can add NULL checks, debug logging, etc.)
 *
 * @param bullet  The bullet to free (safe to pass NULL)
 */
void bullet_destroy(BulletNode* bullet);

/**
 * bullet_print - Display bullet info for debugging
 *
 * @param bullet  The bullet to display
 * @param index   Display index (for list enumeration)
 */
void bullet_print(const BulletNode* bullet, int index);

#endif // BULLET_H
