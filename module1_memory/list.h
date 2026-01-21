/**
 * list.h - Doubly Linked List for Bullets
 *
 * CONCEPT: Data Structures in C
 * =============================
 * In JavaScript, you use Array with push/pop/splice and never think twice.
 * In C, there's no built-in dynamic array. You build your own data structures.
 *
 * A Doubly Linked List gives us:
 *     - O(1) insertion at head or tail
 *     - O(1) removal if we have the node pointer
 *     - No size limit (limited only by available RAM)
 *     - Easy iteration in both directions
 *
 * Trade-offs vs arrays:
 *     - No random access (can't do list[5], must traverse)
 *     - Extra memory per element (two pointers)
 *     - Cache-unfriendly (nodes scattered in memory)
 *
 * For game bullets, linked lists are often ideal:
 *     - Bullets are created/destroyed frequently
 *     - We iterate through all bullets each frame anyway
 *     - We rarely need random access to a specific bullet
 */

#ifndef LIST_H
#define LIST_H

#include "bullet.h"  // We need BulletNode definition
#include <stddef.h>  // For size_t

/**
 * BulletList - Container for managing a collection of bullets
 *
 * This struct holds metadata about our list:
 *     - head: First bullet (entry point for forward iteration)
 *     - tail: Last bullet (entry point for backward iteration, fast append)
 *     - count: Number of bullets (avoid traversing to count)
 *
 * DESIGN PATTERN: The Container
 * =============================
 * Instead of passing around raw pointers like:
 *     void add_bullet(BulletNode** head, BulletNode** tail, int* count, ...)
 *
 * We group related data into a container struct:
 *     void add_bullet(BulletList* list, ...)
 *
 * This is similar to encapsulation in OOP, but explicit.
 */
typedef struct {
    BulletNode* head;   // First bullet in list (NULL if empty)
    BulletNode* tail;   // Last bullet in list (NULL if empty)
    size_t count;       // Number of bullets (size_t is unsigned, can't be negative)
} BulletList;

/**
 * list_init - Initialize an empty bullet list
 *
 * CONCEPT: Initialization vs Construction
 * =======================================
 * In JS: const list = new BulletList(); // Constructor handles everything
 * In C:  BulletList list; list_init(&list); // Two steps, manual init
 *
 * We could also use: BulletList list = {0}; // Zero-initialize all fields
 * But an init function is more explicit and can do validation.
 *
 * @param list  Pointer to the list to initialize
 */
void list_init(BulletList* list);

/**
 * list_add - Add a bullet to the end of the list
 *
 * CONCEPT: Pass-by-Pointer
 * ========================
 * We pass BulletList* (pointer to list) because:
 *     1. We need to MODIFY the list (update head/tail/count)
 *     2. Passing by value would only modify a copy
 *
 * This is like passing an object in JS - we can modify its properties.
 *
 * TIME COMPLEXITY: O(1) - We have tail pointer, so append is instant
 *
 * @param list    The list to add to
 * @param bullet  The bullet to add (takes ownership - list will free it)
 */
void list_add(BulletList* list, BulletNode* bullet);

/**
 * list_remove - Remove a specific bullet from the list
 *
 * CONCEPT: Pointer Surgery
 * ========================
 * Removing from a linked list requires "rewiring" the pointers:
 *
 * Before: A <--> B <--> C    (removing B)
 *
 * After:  A <--> C
 *         B is orphaned (must free it!)
 *
 * Steps:
 *     1. A->next = C
 *     2. C->prev = A
 *     3. free(B)
 *
 * Edge cases:
 *     - Removing head: Update list->head
 *     - Removing tail: Update list->tail
 *     - Removing only element: Both head and tail become NULL
 *
 * TIME COMPLEXITY: O(1) - Just pointer manipulation
 *
 * @param list    The list to remove from
 * @param bullet  The specific bullet to remove
 */
void list_remove(BulletList* list, BulletNode* bullet);

/**
 * list_destroy - Free ALL bullets and reset the list
 *
 * CONCEPT: The Destructor Pattern
 * ================================
 * This is our "cleanup" function. It:
 *     1. Iterates through every node
 *     2. Frees each node's memory
 *     3. Resets the list to empty state
 *
 * CRITICAL: Must save 'next' pointer BEFORE calling free()!
 *
 * BUG EXAMPLE:
 *     while (current != NULL) {
 *         free(current);           // Memory is now invalid!
 *         current = current->next; // CRASH! Accessing freed memory!
 *     }
 *
 * CORRECT:
 *     while (current != NULL) {
 *         BulletNode* next = current->next;  // Save BEFORE freeing
 *         free(current);
 *         current = next;  // Use saved pointer
 *     }
 *
 * @param list  The list to destroy (list struct itself is NOT freed)
 * @return      Number of bullets that were freed
 */
size_t list_destroy(BulletList* list);

/**
 * list_print - Display all bullets in the list
 *
 * CONCEPT: Iteration
 * ==================
 * In JS: bullets.forEach((b, i) => console.log(b));
 * In C:  Manual traversal with a while loop
 *
 * Pattern:
 *     BulletNode* current = list->head;
 *     while (current != NULL) {
 *         // Do something with current
 *         current = current->next;  // Move to next node
 *     }
 *
 * @param list  The list to print
 */
void list_print(const BulletList* list);

/**
 * list_count - Get number of bullets in the list
 *
 * Why have a function when we could just access list->count directly?
 *     1. Encapsulation: If we change internal representation, API stays same
 *     2. Validation: Could add checks in the function
 *     3. Debugging: Can add logging, breakpoints
 *
 * @param list  The list to query
 * @return      Number of bullets
 */
size_t list_count(const BulletList* list);

#endif // LIST_H
