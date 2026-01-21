/**
 * weapon.c - Weapon System Implementation
 *
 * This is where the Strategy Pattern comes to life!
 * Each weapon type has its own fire function (the "strategy"),
 * but they all share the same interface (FireFunc).
 */

#include "weapon.h"
#include "bullet.h"  // NOW we include full definition - needed for bullet_spawn
#include <math.h>    // For cosf, sinf
#include <stddef.h>  // For NULL

// Math constant
#define PI 3.14159265358979323846f
#define DEG_TO_RAD(deg) ((deg) * PI / 180.0f)

/**
 * =======================================================================
 * CONCRETE STRATEGIES: Fire Function Implementations
 * =======================================================================
 *
 * Each of these functions:
 *     1. Has the SAME signature (matches FireFunc typedef)
 *     2. Does something DIFFERENT when called
 *     3. Can be swapped into any Weapon at runtime
 *
 * This is polymorphism in C!
 */

/**
 * spread_fire - The Spread Shot strategy
 *
 * Spawns 3 bullets in a fan pattern:
 *     - One going straight up
 *     - One angled 15 degrees left
 *     - One angled 15 degrees right
 *
 * MATH: Converting angle to velocity
 * ==================================
 * To move at angle θ with speed s:
 *     velocity.x = s * sin(θ)   (horizontal component)
 *     velocity.y = -s * cos(θ)  (vertical - negative because up is -Y)
 *
 * Angle 0° = straight up
 * Angle -15° = slightly left
 * Angle +15° = slightly right
 */
void spread_fire(Vector2 position, BulletList* bullets) {
    if (bullets == NULL) return;

    // Bullet properties for spread shot
    float speed = 400.0f;
    Color color = { 255, 200, 50, 255 };  // Yellow-orange
    int damage = 5;

    // Spawn three bullets at different angles
    float angles[] = { -15.0f, 0.0f, 15.0f };  // Degrees from vertical

    for (int i = 0; i < 3; i++) {
        float angle_rad = DEG_TO_RAD(angles[i]);

        // Convert angle to velocity vector
        // Note: In screen coords, -Y is up, so we use -cos for Y
        Vector2 velocity = {
            .x = speed * sinf(angle_rad),
            .y = -speed * cosf(angle_rad)  // Negative = upward
        };

        // Offset spawn position slightly based on angle
        Vector2 spawn_pos = {
            .x = position.x + 10.0f * sinf(angle_rad),
            .y = position.y - 20.0f  // Start above player center
        };

        bullet_spawn(bullets, spawn_pos, velocity, color, damage);
    }
}

/**
 * rapid_fire - The Rapid Fire strategy
 *
 * Spawns a single bullet straight up.
 * The "rapid" part comes from the high fire_rate in the Weapon struct,
 * not from spawning multiple bullets.
 *
 * This demonstrates separation of concerns:
 *     - Fire function: WHAT happens when you fire
 *     - Fire rate: HOW OFTEN you can fire
 */
void rapid_fire(Vector2 position, BulletList* bullets) {
    if (bullets == NULL) return;

    // Bullet properties for rapid fire
    float speed = 600.0f;  // Faster than spread
    Color color = { 50, 200, 255, 255 };  // Cyan
    int damage = 3;  // Less damage per shot (but more shots!)

    Vector2 velocity = { 0.0f, -speed };  // Straight up
    Vector2 spawn_pos = { position.x, position.y - 25.0f };

    bullet_spawn(bullets, spawn_pos, velocity, color, damage);
}

/**
 * laser_fire - The Laser strategy
 *
 * Spawns a single powerful, fast "laser" bullet.
 * In a full game, this might be a continuous beam.
 * For simplicity, we use a large, fast bullet.
 */
void laser_fire(Vector2 position, BulletList* bullets) {
    if (bullets == NULL) return;

    // Bullet properties for laser
    float speed = 800.0f;  // Very fast
    Color color = { 255, 50, 100, 255 };  // Magenta/pink
    int damage = 15;  // High damage

    Vector2 velocity = { 0.0f, -speed };
    Vector2 spawn_pos = { position.x, position.y - 30.0f };

    Bullet* laser = bullet_spawn(bullets, spawn_pos, velocity, color, damage);
    if (laser != NULL) {
        // Make laser bullet larger
        laser->radius = 8.0f;
    }
}

/**
 * =======================================================================
 * WEAPON FACTORY: Creating Weapons
 * =======================================================================
 */

/**
 * weapon_create - Factory function to create a weapon
 *
 * PATTERN: Factory with Switch
 * ============================
 * Here we DO use a switch, but only for initialization.
 * Once created, the weapon's fire function pointer is used directly.
 *
 * This is different from having a switch in the fire code itself:
 *     - Here: switch runs ONCE at creation
 *     - Bad: switch runs EVERY TIME you fire
 */
Weapon weapon_create(WeaponType type) {
    Weapon weapon = {0};  // Zero-initialize all fields

    switch (type) {
        case WEAPON_SPREAD:
            weapon.name = "Spread Shot";
            weapon.type = WEAPON_SPREAD;
            weapon.fire = spread_fire;  // Assign function pointer!
            weapon.fire_rate = 3.0f;    // 3 shots per second
            weapon.bullet_speed = 400.0f;
            weapon.bullet_damage = 5;
            weapon.bullet_color = (Color){ 255, 200, 50, 255 };
            break;

        case WEAPON_RAPID:
            weapon.name = "Rapid Fire";
            weapon.type = WEAPON_RAPID;
            weapon.fire = rapid_fire;   // Different function pointer!
            weapon.fire_rate = 10.0f;   // 10 shots per second (fast!)
            weapon.bullet_speed = 600.0f;
            weapon.bullet_damage = 3;
            weapon.bullet_color = (Color){ 50, 200, 255, 255 };
            break;

        case WEAPON_LASER:
            weapon.name = "Laser";
            weapon.type = WEAPON_LASER;
            weapon.fire = laser_fire;   // Yet another function pointer!
            weapon.fire_rate = 1.5f;    // 1.5 shots per second (slow but powerful)
            weapon.bullet_speed = 800.0f;
            weapon.bullet_damage = 15;
            weapon.bullet_color = (Color){ 255, 50, 100, 255 };
            break;

        default:
            // Unknown type - create a default spread weapon
            weapon.name = "Unknown";
            weapon.type = WEAPON_SPREAD;
            weapon.fire = spread_fire;
            weapon.fire_rate = 2.0f;
            break;
    }

    weapon.cooldown = 0.0f;  // Ready to fire immediately
    return weapon;
}

/**
 * weapon_fire - Attempt to fire the weapon
 *
 * CONCEPT: Calling a Function Pointer
 * ===================================
 * Once we have a function pointer, we call it just like a regular function:
 *
 *     weapon->fire(position, bullets);
 *
 * The compiler:
 *     1. Looks up the address stored in weapon->fire
 *     2. Jumps to that address
 *     3. Executes the function found there
 *
 * This is how polymorphism works at the machine level!
 */
int weapon_fire(Weapon* weapon, Vector2 position, BulletList* bullets) {
    if (weapon == NULL || bullets == NULL) return 0;

    // Check if still on cooldown
    if (weapon->cooldown > 0.0f) {
        return 0;  // Can't fire yet
    }

    // Check if we have a fire function
    if (weapon->fire == NULL) {
        return 0;  // No fire function assigned
    }

    // FIRE! Call the function pointer
    // This is where the magic happens.
    // Depending on weapon type, this calls spread_fire, rapid_fire, or laser_fire
    weapon->fire(position, bullets);

    // Reset cooldown
    // Cooldown = 1 / fire_rate
    // e.g., 10 shots/sec means 0.1 sec cooldown
    weapon->cooldown = 1.0f / weapon->fire_rate;

    return 1;  // Successfully fired
}

/**
 * weapon_update - Update weapon cooldown
 *
 * Call every frame to count down the cooldown timer.
 */
void weapon_update(Weapon* weapon, float dt) {
    if (weapon == NULL) return;

    // Decrease cooldown
    if (weapon->cooldown > 0.0f) {
        weapon->cooldown -= dt;
        if (weapon->cooldown < 0.0f) {
            weapon->cooldown = 0.0f;
        }
    }
}

/**
 * weapon_can_fire - Check if weapon is ready
 */
int weapon_can_fire(const Weapon* weapon) {
    if (weapon == NULL) return 0;
    return weapon->cooldown <= 0.0f;
}

/**
 * weapon_get_name - Get weapon display name
 */
const char* weapon_get_name(const Weapon* weapon) {
    if (weapon == NULL) return "None";
    return weapon->name;
}
