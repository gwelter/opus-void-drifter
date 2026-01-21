/**
 * list.c - Doubly Linked List Implementation
 *
 * This file implements all the list operations. Read carefully - linked list
 * manipulation is a FUNDAMENTAL skill in C programming.
 *
 * VISUALIZATION GUIDE:
 * ====================
 * Throughout this file, we use ASCII diagrams to show pointer relationships.
 * When you see arrows, think "points to":
 *
 *     A --> B  means  A->next = B  (A's next pointer points to B)
 *     A <-- B  means  B->prev = A  (B's prev pointer points to A)
 *     A <-> B  means  both of the above (doubly linked)
 */

#include "list.h"
#include <stdio.h>   // For printf
#include <stdlib.h>  // For free

/**
 * list_init - Initialize an empty bullet list
 *
 * CONCEPT: Defensive Programming
 * ==============================
 * Always check for NULL pointers before dereferencing.
 * A NULL dereference causes a segfault - your program crashes.
 *
 * In JavaScript, you might write:
 *     function init(list) { list.head = null; }
 *
 * If list is null/undefined, JS throws a runtime error with a stack trace.
 * In C, you get a cryptic "Segmentation fault" and the program dies.
 */
void list_init(BulletList* list) {
    // Always validate input pointers
    if (list == NULL) {
        fprintf(stderr, "ERROR: list_init called with NULL pointer\n");
        return;
    }

    // Set up empty list state
    list->head = NULL;   // No first element
    list->tail = NULL;   // No last element
    list->count = 0;     // Zero elements
}

/**
 * list_add - Add a bullet to the end (tail) of the list
 *
 * DEEP DIVE: The Two Cases
 * ========================
 *
 * CASE 1: Empty list (head == NULL)
 * ---------------------------------
 * Before: head -> NULL, tail -> NULL
 *
 * After:  head -> [NEW] <- tail
 *         The new bullet is both head AND tail
 *
 * CASE 2: Non-empty list
 * ----------------------
 * Before: head -> [A] <-> [B] <- tail
 *
 * After:  head -> [A] <-> [B] <-> [NEW] <- tail
 *         [B]->next = NEW
 *         [NEW]->prev = B
 *         tail = NEW
 */
void list_add(BulletList* list, BulletNode* bullet) {
    // Validate inputs
    if (list == NULL) {
        fprintf(stderr, "ERROR: list_add called with NULL list\n");
        return;
    }
    if (bullet == NULL) {
        fprintf(stderr, "ERROR: list_add called with NULL bullet\n");
        return;
    }

    // Ensure the bullet's pointers are clean
    // (It might be recycled from somewhere else)
    bullet->next = NULL;
    bullet->prev = NULL;

    if (list->head == NULL) {
        // CASE 1: Empty list
        // The new bullet becomes both head and tail
        //
        //   head ──┐     ┌── tail
        //          ▼     ▼
        //        [BULLET]
        //        prev=NULL
        //        next=NULL

        list->head = bullet;
        list->tail = bullet;
    } else {
        // CASE 2: Non-empty list
        // Append to the tail
        //
        // Before:
        //   head ──▶ [...] ◀──▶ [TAIL] ◀── tail
        //
        // After:
        //   head ──▶ [...] ◀──▶ [OLD_TAIL] ◀──▶ [BULLET] ◀── tail

        // Step 1: Link old tail to new bullet
        list->tail->next = bullet;  // Old tail points forward to new bullet

        // Step 2: Link new bullet back to old tail
        bullet->prev = list->tail;  // New bullet points back to old tail

        // Step 3: Update tail pointer to new bullet
        list->tail = bullet;
    }

    // Increment count
    list->count++;
}

/**
 * list_remove - Remove a specific bullet from the list
 *
 * DEEP DIVE: Pointer Surgery
 * ==========================
 * This is the trickiest operation. We must handle 4 cases:
 *
 * CASE 1: Removing the ONLY element
 *         head -> [X] <- tail
 *         Result: head -> NULL <- tail
 *
 * CASE 2: Removing the HEAD (but not only element)
 *         head -> [X] <-> [B] <-> [C] <- tail
 *         Result: head -> [B] <-> [C] <- tail
 *
 * CASE 3: Removing the TAIL (but not only element)
 *         head -> [A] <-> [B] <-> [X] <- tail
 *         Result: head -> [A] <-> [B] <- tail
 *
 * CASE 4: Removing from the MIDDLE
 *         head -> [A] <-> [X] <-> [C] <- tail
 *         Result: head -> [A] <-> [C] <- tail
 */
void list_remove(BulletList* list, BulletNode* bullet) {
    // Validate inputs
    if (list == NULL || bullet == NULL) {
        return;  // Nothing to do
    }

    // Get references to neighbors (might be NULL)
    BulletNode* prev_node = bullet->prev;
    BulletNode* next_node = bullet->next;

    // --- Update the PREVIOUS node's 'next' pointer ---
    if (prev_node == NULL) {
        // No previous node means we're removing the HEAD
        // Update list's head pointer to skip over the removed bullet
        list->head = next_node;
    } else {
        // There is a previous node - make it point to our next
        // Before: [prev] --> [bullet] --> [next]
        // After:  [prev] --------------> [next]
        prev_node->next = next_node;
    }

    // --- Update the NEXT node's 'prev' pointer ---
    if (next_node == NULL) {
        // No next node means we're removing the TAIL
        // Update list's tail pointer to the previous node
        list->tail = prev_node;
    } else {
        // There is a next node - make it point back to our prev
        // Before: [prev] <-- [bullet] <-- [next]
        // After:  [prev] <-------------- [next]
        next_node->prev = prev_node;
    }

    // Decrement count
    list->count--;

    // Clean up the removed bullet's pointers (defensive)
    bullet->next = NULL;
    bullet->prev = NULL;

    // Free the bullet's memory
    // The bullet is now "orphaned" - no one points to it
    // We MUST free it or we leak memory
    bullet_destroy(bullet);
}

/**
 * list_destroy - Free ALL bullets and reset the list
 *
 * DEEP DIVE: Safe Iteration While Deleting
 * ========================================
 * The classic mistake:
 *
 *     while (current != NULL) {
 *         free(current);           // current's memory is now invalid!
 *         current = current->next; // BUG! Accessing freed memory!
 *     }
 *
 * The solution: Save 'next' BEFORE freeing
 *
 *     while (current != NULL) {
 *         temp = current->next;    // Save reference FIRST
 *         free(current);           // Now safe to free
 *         current = temp;          // Use saved reference
 *     }
 *
 * This pattern appears constantly in C - memorize it!
 */
size_t list_destroy(BulletList* list) {
    if (list == NULL) {
        return 0;
    }

    size_t freed_count = 0;
    BulletNode* current = list->head;

    // Traverse the list, freeing each node
    while (current != NULL) {
        // CRITICAL: Save the next pointer BEFORE freeing current
        // After free(current), the memory at 'current' is invalid
        // Reading current->next after free is UNDEFINED BEHAVIOR
        BulletNode* next = current->next;

        // Now safe to free the current node
        // Note: We use free() directly here, not bullet_destroy()
        // because bullet_destroy might do extra work we don't need
        // when bulk-destroying. Both approaches are valid.
        free(current);
        freed_count++;

        // Move to the saved next pointer
        current = next;
    }

    // Reset the list to empty state
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;

    return freed_count;
}

/**
 * list_print - Display all bullets in the list
 *
 * CONCEPT: Iteration Pattern
 * ==========================
 * This is the standard linked list traversal pattern.
 * Compare to JavaScript:
 *
 * JS:  bullets.forEach((bullet, index) => { ... });
 *
 * C:   int index = 0;
 *      for (BulletNode* cur = list->head; cur != NULL; cur = cur->next) {
 *          // use cur and index
 *          index++;
 *      }
 *
 * Or with a while loop (equivalent):
 *
 *      BulletNode* cur = list->head;
 *      while (cur != NULL) {
 *          // use cur
 *          cur = cur->next;
 *      }
 */
void list_print(const BulletList* list) {
    if (list == NULL) {
        printf("(null list)\n");
        return;
    }

    printf("--- Bullet List (%zu bullets) ---\n", list->count);

    if (list->head == NULL) {
        printf("(empty)\n");
        return;
    }

    // Traverse and print each bullet
    int index = 0;
    BulletNode* current = list->head;

    while (current != NULL) {
        bullet_print(current, index);
        current = current->next;  // Move to next node
        index++;
    }
}

/**
 * list_count - Get number of bullets in the list
 *
 * CONCEPT: Accessor Functions
 * ===========================
 * Why wrap a simple field access in a function?
 *
 * 1. ENCAPSULATION: If we later change how count is stored
 *    (maybe we compute it dynamically), callers don't change.
 *
 * 2. SAFETY: We can handle NULL gracefully.
 *
 * 3. CONSISTENCY: All list operations go through functions.
 *
 * In practice, for performance-critical code, you might
 * access list->count directly. But starting with accessors
 * is good discipline.
 */
size_t list_count(const BulletList* list) {
    if (list == NULL) {
        return 0;
    }
    return list->count;
}
