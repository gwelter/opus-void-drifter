/**
 * bullet.c - Bullet Entity Implementation
 *
 * This implements the bullet linked list from Module 1,
 * now integrated with Raylib for rendering.
 */

#include "bullet.h"
#include <stdlib.h>  // For malloc, free
#include <stddef.h>  // For NULL
#include <math.h>    // For cosf, sinf

// Default bullet properties
#define DEFAULT_BULLET_RADIUS 4.0f
#define DEFAULT_BULLET_LIFETIME 3.0f  // Seconds before auto-destroy

/**
 * bullet_list_init - Initialize an empty bullet list
 */
void bullet_list_init(BulletList* list, int max_bullets) {
    if (list == NULL) return;

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    list->max_bullets = max_bullets;
}

/**
 * bullet_list_destroy - Free all bullets in the list
 *
 * REMEMBER: Save next pointer BEFORE freeing!
 */
void bullet_list_destroy(BulletList* list) {
    if (list == NULL) return;

    Bullet* current = list->head;
    while (current != NULL) {
        Bullet* next = current->next;  // Save before free
        free(current);
        current = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

/**
 * bullet_spawn - Create a new bullet and add it to the list
 *
 * PATTERN: Encapsulated Allocation
 * ================================
 * Callers don't need to know about malloc.
 * They just call spawn() and get back a bullet (or NULL).
 */
Bullet* bullet_spawn(BulletList* list, Vector2 position, Vector2 velocity,
                     Color color, int damage) {
    if (list == NULL) return NULL;

    // Check if at max capacity
    if (list->max_bullets > 0 && list->count >= list->max_bullets) {
        // At capacity - could remove oldest bullet here
        // For now, just refuse to spawn
        return NULL;
    }

    // Allocate new bullet
    Bullet* bullet = malloc(sizeof(Bullet));
    if (bullet == NULL) return NULL;

    // Initialize bullet properties
    bullet->position = position;
    bullet->velocity = velocity;
    bullet->angle = atan2f(velocity.y, velocity.x) * (180.0f / 3.14159f);
    bullet->color = color;
    bullet->radius = DEFAULT_BULLET_RADIUS;
    bullet->damage = damage;
    bullet->lifetime = DEFAULT_BULLET_LIFETIME;
    bullet->active = 1;

    // Initialize list pointers
    bullet->next = NULL;
    bullet->prev = NULL;

    // Add to list (at tail for chronological order)
    if (list->head == NULL) {
        // Empty list
        list->head = bullet;
        list->tail = bullet;
    } else {
        // Add to tail
        list->tail->next = bullet;
        bullet->prev = list->tail;
        list->tail = bullet;
    }

    list->count++;
    return bullet;
}

/**
 * bullet_remove - Remove a bullet from the list and free it
 *
 * CRITICAL: Handle all 4 cases:
 * 1. Only element (head == tail == bullet)
 * 2. Removing head (bullet->prev == NULL)
 * 3. Removing tail (bullet->next == NULL)
 * 4. Removing middle (has both prev and next)
 */
void bullet_remove(BulletList* list, Bullet* bullet) {
    if (list == NULL || bullet == NULL) return;

    // Update previous node's next pointer
    if (bullet->prev != NULL) {
        bullet->prev->next = bullet->next;
    } else {
        // Removing head
        list->head = bullet->next;
    }

    // Update next node's prev pointer
    if (bullet->next != NULL) {
        bullet->next->prev = bullet->prev;
    } else {
        // Removing tail
        list->tail = bullet->prev;
    }

    list->count--;
    free(bullet);
}

/**
 * bullet_list_update - Update all bullets for one frame
 *
 * PATTERN: Safe Iteration While Removing
 * ======================================
 * When iterating a list and possibly removing elements,
 * ALWAYS save the next pointer before potentially removing.
 */
void bullet_list_update(BulletList* list, float dt,
                        int screen_width, int screen_height) {
    if (list == NULL) return;

    Bullet* current = list->head;
    while (current != NULL) {
        // IMPORTANT: Save next before we might remove current
        Bullet* next = current->next;

        // Update position
        current->position.x += current->velocity.x * dt;
        current->position.y += current->velocity.y * dt;

        // Update lifetime
        current->lifetime -= dt;

        // Check if bullet should be removed
        int should_remove = 0;

        // Check lifetime
        if (current->lifetime <= 0) {
            should_remove = 1;
        }

        // Check screen bounds (with margin for bullet size)
        float margin = current->radius * 2;
        if (current->position.x < -margin ||
            current->position.x > screen_width + margin ||
            current->position.y < -margin ||
            current->position.y > screen_height + margin) {
            should_remove = 1;
        }

        // Remove if needed
        if (should_remove) {
            bullet_remove(list, current);
        }

        current = next;  // Use saved pointer
    }
}

/**
 * bullet_list_draw - Render all bullets
 *
 * We draw bullets as glowing circles with a brighter center.
 */
void bullet_list_draw(const BulletList* list) {
    if (list == NULL) return;

    Bullet* current = list->head;
    while (current != NULL) {
        // Outer glow (larger, more transparent)
        Color glow_color = current->color;
        glow_color.a = 100;
        DrawCircle((int)current->position.x, (int)current->position.y,
                   current->radius * 2, glow_color);

        // Core (smaller, brighter)
        DrawCircle((int)current->position.x, (int)current->position.y,
                   current->radius, current->color);

        // Bright center
        Color bright = WHITE;
        bright.a = 200;
        DrawCircle((int)current->position.x, (int)current->position.y,
                   current->radius * 0.5f, bright);

        current = current->next;
    }
}

/**
 * bullet_list_clear - Remove all bullets without destroying the list
 */
void bullet_list_clear(BulletList* list) {
    if (list == NULL) return;

    Bullet* current = list->head;
    while (current != NULL) {
        Bullet* next = current->next;
        free(current);
        current = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}
