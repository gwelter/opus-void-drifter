/**
 * bullet.h - Bullet Entity and Linked List
 *
 * This module combines what we learned in Module 1 (linked lists)
 * with what we learned in Module 2 (Raylib rendering).
 *
 * Bullets are stored in a doubly linked list for efficient:
 *     - Insertion (when firing)
 *     - Removal (when hitting something or going off-screen)
 *     - Iteration (for update/draw each frame)
 */

#ifndef BULLET_H
#define BULLET_H

#include "raylib.h"

/**
 * Bullet - A single projectile in the game
 *
 * CONCEPT: Entity Component
 * =========================
 * Each bullet has:
 *     - Transform (position, velocity)
 *     - Appearance (color, size)
 *     - Gameplay (damage, lifetime)
 *     - List links (next, prev)
 */
typedef struct Bullet {
    // Transform
    Vector2 position;
    Vector2 velocity;
    float angle;          // Direction in degrees

    // Appearance
    Color color;
    float radius;

    // Gameplay
    int damage;
    float lifetime;       // Seconds remaining before auto-destroy
    int active;           // Is this bullet still in play?

    // Linked list pointers
    struct Bullet* next;
    struct Bullet* prev;
} Bullet;

/**
 * BulletList - Container for all active bullets
 *
 * CONCEPT: Object Pool Alternative
 * ================================
 * In a full game, you might pre-allocate a pool of bullets
 * and reuse them to avoid malloc/free overhead.
 *
 * For learning, we use a simple linked list with malloc/free.
 */
typedef struct BulletList {
    Bullet* head;
    Bullet* tail;
    int count;
    int max_bullets;      // Limit to prevent memory explosion
} BulletList;

/**
 * bullet_list_init - Initialize an empty bullet list
 *
 * @param list        List to initialize
 * @param max_bullets Maximum bullets allowed (-1 for unlimited)
 */
void bullet_list_init(BulletList* list, int max_bullets);

/**
 * bullet_list_destroy - Free all bullets in the list
 *
 * @param list  List to destroy
 */
void bullet_list_destroy(BulletList* list);

/**
 * bullet_spawn - Create a new bullet and add it to the list
 *
 * CONCEPT: Factory Function
 * =========================
 * Instead of exposing malloc to callers, we provide a "spawn" function
 * that handles allocation, initialization, and list insertion.
 *
 * @param list       List to add bullet to
 * @param position   Starting position
 * @param velocity   Movement vector (pixels/second)
 * @param color      Bullet color
 * @param damage     Damage on hit
 * @return           Pointer to new bullet, or NULL if at max
 */
Bullet* bullet_spawn(BulletList* list, Vector2 position, Vector2 velocity,
                     Color color, int damage);

/**
 * bullet_remove - Remove a bullet from the list and free it
 *
 * @param list    List to remove from
 * @param bullet  Bullet to remove
 */
void bullet_remove(BulletList* list, Bullet* bullet);

/**
 * bullet_list_update - Update all bullets for one frame
 *
 * This function:
 *     - Moves bullets according to velocity
 *     - Decreases lifetime
 *     - Removes bullets that are off-screen or expired
 *
 * @param list          List to update
 * @param dt            Delta time (seconds)
 * @param screen_width  Screen boundary
 * @param screen_height Screen boundary
 */
void bullet_list_update(BulletList* list, float dt,
                        int screen_width, int screen_height);

/**
 * bullet_list_draw - Render all bullets
 *
 * @param list  List to draw
 */
void bullet_list_draw(const BulletList* list);

/**
 * bullet_list_clear - Remove all bullets without destroying the list
 *
 * @param list  List to clear
 */
void bullet_list_clear(BulletList* list);

#endif // BULLET_H
