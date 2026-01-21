/**
 * player.h - Player Entity with Weapon System
 *
 * The player now has an EQUIPPED WEAPON that can be swapped at runtime.
 * This demonstrates how the Strategy Pattern is used in practice.
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "weapon.h"

// Forward declarations to avoid circular includes
typedef struct BulletList BulletList;

/**
 * Player - The player's spaceship
 *
 * New in Module 3: The 'weapon' field
 *
 * The player contains a Weapon struct (not a pointer).
 * This is "composition" - the player HAS-A weapon.
 * When player is created, a weapon is created inside it.
 * When player is destroyed, the weapon is destroyed with it.
 */
typedef struct {
    // Transform
    Vector2 position;
    Vector2 velocity;

    // Physics
    float speed;
    float acceleration;
    float friction;

    // Weapon system - NEW IN MODULE 3
    Weapon weapon;              // Currently equipped weapon
    int is_firing;              // Is fire button held?

    // Visual
    Texture2D* texture;
    Texture2D* glow_texture;

    // State
    int is_thrusting;

} Player;

/**
 * player_init - Initialize a player
 *
 * @param player        Player to initialize
 * @param start_x       Starting X position
 * @param start_y       Starting Y position
 * @param texture       Ship texture (owned by caller)
 * @param glow_texture  Engine glow texture (owned by caller)
 */
void player_init(Player* player, float start_x, float start_y,
                 Texture2D* texture, Texture2D* glow_texture);

/**
 * player_handle_input - Process input for movement AND firing
 *
 * Now handles:
 *     - WASD/Arrows for movement
 *     - SPACE for firing
 *     - 1/2/3 for weapon switching
 *
 * @param player   Player to update
 * @param bullets  Bullet list for firing
 */
void player_handle_input(Player* player, BulletList* bullets);

/**
 * player_update - Update player physics and weapon
 *
 * @param player        Player to update
 * @param dt            Delta time
 * @param screen_width  Screen boundary
 * @param screen_height Screen boundary
 */
void player_update(Player* player, float dt, int screen_width, int screen_height);

/**
 * player_draw - Render the player
 *
 * @param player  Player to draw
 */
void player_draw(const Player* player);

/**
 * player_switch_weapon - Change to a different weapon type
 *
 * CONCEPT: Runtime Strategy Swapping
 * ==================================
 * This is where we see the power of function pointers.
 * By changing the weapon, we change what happens when player fires,
 * WITHOUT changing any code in the firing system.
 *
 * @param player  Player to update
 * @param type    New weapon type
 */
void player_switch_weapon(Player* player, WeaponType type);

/**
 * player_get_weapon_name - Get current weapon's name for UI
 *
 * @param player  Player to query
 * @return        Weapon name string
 */
const char* player_get_weapon_name(const Player* player);

/**
 * player_get_center - Get player's center position
 *
 * @param player  Player to query
 * @return        Center position
 */
Vector2 player_get_center(const Player* player);

#endif // PLAYER_H
