/**
 * weapon.h - Weapon System with Function Pointers
 *
 * CONCEPT: The Strategy Pattern in C
 * ==================================
 * This is the heart of Module 3. We use function pointers to implement
 * swappable weapon behaviors - the Strategy Pattern.
 *
 * In JavaScript terms:
 * ```javascript
 * class Weapon {
 *     constructor(fireFunc) {
 *         this.fire = fireFunc;  // Store a callback
 *     }
 * }
 *
 * const spreadWeapon = new Weapon((pos, bullets) => {
 *     // Spawn spread bullets
 * });
 * ```
 *
 * In C, we use function pointers to achieve the same thing:
 * ```c
 * typedef void (*FireFunc)(Vector2 pos, BulletList* bullets);
 *
 * typedef struct {
 *     FireFunc fire;  // Store a function pointer
 * } Weapon;
 * ```
 */

#ifndef WEAPON_H
#define WEAPON_H

#include "raylib.h"

// Forward declaration - avoids circular dependency with bullet.h
// We only need to know BulletList EXISTS, not its full definition
// (because we only use BulletList* pointers, not BulletList values)
typedef struct BulletList BulletList;

/**
 * CONCEPT: typedef for Function Pointers
 * ======================================
 * Instead of writing the ugly syntax everywhere:
 *     void (*fire)(Vector2, BulletList*)
 *
 * We create a typedef:
 *     typedef void (*FireFunc)(Vector2, BulletList*);
 *
 * Now we can use FireFunc like any other type.
 */

/**
 * FireFunc - Function pointer type for weapon firing
 *
 * @param position  Where to spawn bullets from
 * @param bullets   The bullet list to add new bullets to
 *
 * This is the "strategy interface" - all fire functions must match this signature.
 */
typedef void (*FireFunc)(Vector2 position, BulletList* bullets);

/**
 * WeaponType - Enum for weapon identification
 *
 * CONCEPT: Enums in C
 * ===================
 * Enums are just named integer constants.
 *     WEAPON_SPREAD = 0
 *     WEAPON_RAPID = 1
 *     WEAPON_LASER = 2
 *     WEAPON_COUNT = 3 (useful for array sizing)
 */
typedef enum {
    WEAPON_SPREAD,
    WEAPON_RAPID,
    WEAPON_LASER,
    WEAPON_COUNT  // Always last - gives us the count of weapons
} WeaponType;

/**
 * Weapon - A weapon with swappable fire behavior
 *
 * MEMORY LAYOUT:
 * ==============
 * ┌────────────────────────────────────────────────────────┐
 * │ name (8 bytes - pointer to string literal)             │
 * │ type (4 bytes - enum/int)                              │
 * │ fire (8 bytes - function pointer)                      │
 * │ fire_rate (4 bytes - float)                            │
 * │ cooldown (4 bytes - float)                             │
 * │ bullet_speed (4 bytes - float)                         │
 * │ bullet_damage (4 bytes - int)                          │
 * │ bullet_color (4 bytes - Color struct)                  │
 * └────────────────────────────────────────────────────────┘
 *
 * The fire function pointer is what makes this powerful.
 * Different weapons have different fire functions, but they
 * all match the FireFunc signature.
 */
typedef struct {
    // Identity
    const char* name;   // "Spread Shot", "Rapid Fire", etc.
    WeaponType type;    // Enum value for identification

    // THE KEY: Function pointer for firing behavior
    FireFunc fire;      // Points to spread_fire, rapid_fire, etc.

    // Firing mechanics
    float fire_rate;    // Shots per second
    float cooldown;     // Current cooldown timer (decrements each frame)

    // Bullet properties (passed to fire function via weapon_fire)
    float bullet_speed;   // How fast bullets travel
    int bullet_damage;    // Damage per bullet
    Color bullet_color;   // Bullet appearance

} Weapon;

/**
 * =======================================================================
 * CONCRETE STRATEGIES: The actual fire function implementations
 * =======================================================================
 * These functions implement different firing behaviors.
 * They all have the same signature (FireFunc) so they're interchangeable.
 */

/**
 * spread_fire - Fire 3 bullets in a spread pattern
 *
 *        •   •   •
 *         \  |  /
 *          \ | /
 *           \|/
 *            ▲
 *
 * @param position  Player center position
 * @param bullets   Bullet list to add to
 */
void spread_fire(Vector2 position, BulletList* bullets);

/**
 * rapid_fire - Fire a single bullet straight up
 * (The weapon's high fire_rate makes it "rapid")
 *
 *            •
 *            |
 *            |
 *            ▲
 *
 * @param position  Player center position
 * @param bullets   Bullet list to add to
 */
void rapid_fire(Vector2 position, BulletList* bullets);

/**
 * laser_fire - Fire a powerful focused beam
 * (Actually spawns a fast, large bullet)
 *
 *            █
 *            █
 *            █
 *            ▲
 *
 * @param position  Player center position
 * @param bullets   Bullet list to add to
 */
void laser_fire(Vector2 position, BulletList* bullets);

/**
 * =======================================================================
 * WEAPON API: Functions to create and use weapons
 * =======================================================================
 */

/**
 * weapon_create - Create a weapon with specific properties
 *
 * CONCEPT: Factory Function
 * =========================
 * Instead of having callers manually fill in the struct,
 * we provide a factory that sets up everything correctly.
 *
 * @param type  Which weapon type to create
 * @return      Fully initialized Weapon struct
 */
Weapon weapon_create(WeaponType type);

/**
 * weapon_fire - Attempt to fire the weapon
 *
 * CONCEPT: Cooldown System
 * ========================
 * Weapons can't fire every frame - they have a cooldown based on fire_rate.
 * This function checks the cooldown and calls the fire function pointer
 * if enough time has passed.
 *
 * @param weapon    The weapon to fire
 * @param position  Where to spawn bullets
 * @param bullets   Bullet list to add to
 * @return          1 if fired, 0 if still on cooldown
 */
int weapon_fire(Weapon* weapon, Vector2 position, BulletList* bullets);

/**
 * weapon_update - Update weapon cooldown
 *
 * Call this every frame to decrease the cooldown timer.
 *
 * @param weapon  Weapon to update
 * @param dt      Delta time (seconds)
 */
void weapon_update(Weapon* weapon, float dt);

/**
 * weapon_can_fire - Check if weapon is ready to fire
 *
 * @param weapon  Weapon to check
 * @return        1 if ready, 0 if on cooldown
 */
int weapon_can_fire(const Weapon* weapon);

/**
 * weapon_get_name - Get the weapon's display name
 *
 * @param weapon  Weapon to query
 * @return        Name string (do not free!)
 */
const char* weapon_get_name(const Weapon* weapon);

#endif // WEAPON_H
