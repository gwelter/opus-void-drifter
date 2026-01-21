/**
 * player.h - Player Entity Definition
 *
 * CONCEPT: Entity Structure
 * =========================
 * In game development, an "entity" is anything that exists in the game world:
 *     - Players, enemies, bullets, items, etc.
 *
 * Each entity typically has:
 *     - Position (where it is)
 *     - Velocity (how it's moving)
 *     - Visual representation (sprite/texture)
 *     - Behavior (how it responds to input/time)
 *
 * In JavaScript:
 *     class Player {
 *         constructor() {
 *             this.position = { x: 0, y: 0 };
 *             this.velocity = { x: 0, y: 0 };
 *         }
 *         update(dt) { ... }
 *         draw() { ... }
 *     }
 *
 * In C, we use structs for data and separate functions for behavior.
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"

/**
 * Player - The player's spaceship entity
 *
 * MEMORY LAYOUT:
 * ==============
 * Vector2 is { float x, float y } = 8 bytes
 *
 * ┌──────────────────────────────────────────┐
 * │ position (8) │ velocity (8) │ speed (4) │
 * │ accel (4)    │ friction (4) │ is_thrusting (4) │
 * │ texture_ptr (8) │ glow_ptr (8) │
 * └──────────────────────────────────────────┘
 *
 * Note: We store pointers to textures, not the textures themselves.
 * This is because textures should be owned by GameAssets, not Player.
 */
typedef struct Player {
    // Transform (position in world space)
    Vector2 position;

    // Physics
    Vector2 velocity;      // Current velocity (pixels/second)
    float speed;           // Max speed
    float acceleration;    // How fast we accelerate
    float friction;        // Velocity decay per second (0-1)

    // Visual
    Texture2D* texture;     // Pointer to ship texture (owned by GameAssets)
    Texture2D* glow_texture; // Pointer to engine glow texture

    // State
    int is_thrusting;      // Are engines firing? (for visual effect)
    float thrust_direction; // Direction of thrust (radians, for future rotation)

} Player;

/**
 * player_init - Initialize a player with default values
 *
 * CONCEPT: Initialization Functions
 * =================================
 * In C, structs are NOT automatically initialized.
 *
 * BUG EXAMPLE:
 *     Player p;  // All fields contain GARBAGE!
 *     p.position.x += 5;  // Adding 5 to garbage = garbage
 *
 * CORRECT:
 *     Player p;
 *     player_init(&p, x, y, texture, glow);  // Now properly initialized
 *
 * @param player       Pointer to player to initialize
 * @param start_x      Initial X position
 * @param start_y      Initial Y position
 * @param texture      Pointer to ship texture (from GameAssets)
 * @param glow_texture Pointer to engine glow texture
 */
void player_init(Player* player, float start_x, float start_y,
                 Texture2D* texture, Texture2D* glow_texture);

/**
 * player_handle_input - Process keyboard input for movement
 *
 * CONCEPT: Input Handling
 * =======================
 * Raylib provides several input functions:
 *
 * IsKeyDown(KEY)   - Returns true while key is held
 * IsKeyPressed(KEY) - Returns true only on the frame key was pressed
 * IsKeyReleased(KEY) - Returns true only on the frame key was released
 *
 * For movement, we use IsKeyDown (continuous).
 * For actions like firing, we'd use IsKeyPressed (single trigger).
 *
 * @param player  The player to update
 */
void player_handle_input(Player* player);

/**
 * player_update - Update player physics for one frame
 *
 * CONCEPT: Physics Update
 * =======================
 * Each frame, we:
 *     1. Apply friction (slow down if not thrusting)
 *     2. Update position based on velocity
 *     3. Clamp position to screen bounds
 *
 * @param player  The player to update
 * @param dt      Delta time (seconds since last frame)
 * @param screen_width  Screen boundary
 * @param screen_height Screen boundary
 */
void player_update(Player* player, float dt, int screen_width, int screen_height);

/**
 * player_draw - Render the player to the screen
 *
 * CONCEPT: Draw Order
 * ===================
 * In 2D games, draw order matters:
 *     - Things drawn later appear ON TOP of earlier things
 *     - Background first, then entities, then UI
 *
 * For the player, we draw:
 *     1. Engine glow (behind ship)
 *     2. Ship sprite
 *
 * CONCEPT: Texture Origin
 * =======================
 * By default, textures are drawn with their TOP-LEFT corner at the position.
 * To center a texture on the player:
 *     drawX = player.x - texture.width / 2
 *     drawY = player.y - texture.height / 2
 *
 * @param player  The player to draw
 */
void player_draw(const Player* player);

/**
 * player_get_center - Get the center point of the player
 *
 * Useful for spawning bullets from the player's position.
 *
 * @param player  The player
 * @return        Center position as Vector2
 */
Vector2 player_get_center(const Player* player);

#endif // PLAYER_H
