/**
 * player.c - Player Entity Implementation
 *
 * This file implements the player's movement, physics, and rendering.
 */

#include "player.h"
#include <math.h>    // For fabsf, sqrtf, powf, sinf
#include <stddef.h>  // For NULL

// Movement constants
// These could be moved to a config struct for easier tweaking
#define PLAYER_DEFAULT_SPEED       300.0f   // Pixels per second
#define PLAYER_DEFAULT_ACCEL       800.0f   // Acceleration rate
#define PLAYER_DEFAULT_FRICTION    0.95f    // Velocity retention per frame

/**
 * player_init - Initialize a player with default values
 *
 * PATTERN: Defensive Initialization
 * =================================
 * We set EVERY field explicitly, even if it would be zero.
 * This makes the code clear and prevents issues if struct changes.
 */
void player_init(Player* player, float start_x, float start_y,
                 Texture2D* texture, Texture2D* glow_texture) {
    if (player == NULL) return;

    // Position - where the player is in the world
    player->position.x = start_x;
    player->position.y = start_y;

    // Velocity - currently stationary
    player->velocity.x = 0.0f;
    player->velocity.y = 0.0f;

    // Movement parameters
    player->speed = PLAYER_DEFAULT_SPEED;
    player->acceleration = PLAYER_DEFAULT_ACCEL;
    player->friction = PLAYER_DEFAULT_FRICTION;

    // Visual references (we don't own these, just point to them)
    player->texture = texture;
    player->glow_texture = glow_texture;

    // State
    player->is_thrusting = 0;
    player->thrust_direction = 0.0f;
}

/**
 * player_handle_input - Process keyboard input for movement
 *
 * DEEP DIVE: Input Mapping
 * ========================
 * We support both WASD and Arrow keys for accessibility.
 *
 * Instead of directly modifying position, we modify VELOCITY.
 * This creates smoother, more physical movement.
 *
 * The velocity is then used in player_update() to move the player.
 */
void player_handle_input(Player* player) {
    if (player == NULL) return;

    // Reset thrust state each frame
    player->is_thrusting = 0;

    // Acceleration to apply this frame (will be normalized later if diagonal)
    float accel_x = 0.0f;
    float accel_y = 0.0f;

    // Check movement keys
    // IsKeyDown returns true every frame the key is held
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
        accel_y = -1.0f;  // Up (negative Y in screen coordinates)
        player->is_thrusting = 1;
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
        accel_y = 1.0f;   // Down
        player->is_thrusting = 1;
    }
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
        accel_x = -1.0f;  // Left
        player->is_thrusting = 1;
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
        accel_x = 1.0f;   // Right
        player->is_thrusting = 1;
    }

    // Normalize diagonal movement
    // Without this, moving diagonally would be ~1.41x faster!
    // (Because sqrt(1^2 + 1^2) = sqrt(2) ≈ 1.41)
    if (accel_x != 0.0f && accel_y != 0.0f) {
        // Divide by sqrt(2) to normalize
        float inv_sqrt2 = 0.70710678f;  // 1 / sqrt(2)
        accel_x *= inv_sqrt2;
        accel_y *= inv_sqrt2;
    }

    // Apply acceleration to velocity
    // We DON'T multiply by dt here - that happens in update()
    // This just sets the direction/magnitude of acceleration
    if (player->is_thrusting) {
        // Store acceleration direction for the update function to use
        // We'll apply the actual acceleration * dt in update()
        player->velocity.x += accel_x * player->acceleration * GetFrameTime();
        player->velocity.y += accel_y * player->acceleration * GetFrameTime();
    }
}

/**
 * player_update - Update player physics for one frame
 *
 * PHYSICS MODEL:
 * ==============
 *     velocity = velocity * friction                  (slow down)
 *     position = position + velocity * dt            (move)
 *     position = clamp(position, 0, screen_size)     (stay in bounds)
 *
 * The friction model creates a "space drift" feel:
 *     - When you stop thrusting, you gradually slow down
 *     - Higher friction = quicker stop
 *     - Lower friction = more "icy" sliding
 */
void player_update(Player* player, float dt, int screen_width, int screen_height) {
    if (player == NULL) return;

    // Apply friction (velocity decay)
    // This creates the "drift" effect when not thrusting
    // friction^(dt*60) normalizes for frame rate
    // At 60 FPS with friction 0.95: each frame velocity *= 0.95
    // At 30 FPS with friction 0.95: each frame velocity *= 0.95^2 = 0.9025
    float friction_this_frame = powf(player->friction, dt * 60.0f);
    player->velocity.x *= friction_this_frame;
    player->velocity.y *= friction_this_frame;

    // Clamp velocity to max speed
    float current_speed = sqrtf(player->velocity.x * player->velocity.x +
                                player->velocity.y * player->velocity.y);
    if (current_speed > player->speed) {
        float scale = player->speed / current_speed;
        player->velocity.x *= scale;
        player->velocity.y *= scale;
    }

    // Stop completely if very slow (prevents endless micro-movement)
    if (fabsf(player->velocity.x) < 1.0f) player->velocity.x = 0.0f;
    if (fabsf(player->velocity.y) < 1.0f) player->velocity.y = 0.0f;

    // Update position based on velocity
    // This is the core physics equation: position += velocity * time
    player->position.x += player->velocity.x * dt;
    player->position.y += player->velocity.y * dt;

    // Get texture dimensions for boundary checking
    // (Player position is center, so we use half-dimensions)
    float half_width = 0.0f;
    float half_height = 0.0f;
    if (player->texture != NULL) {
        half_width = player->texture->width / 2.0f;
        half_height = player->texture->height / 2.0f;
    }

    // Clamp to screen bounds
    // This keeps the player visible on screen
    if (player->position.x < half_width) {
        player->position.x = half_width;
        player->velocity.x = 0.0f;  // Stop horizontal movement at edge
    }
    if (player->position.x > screen_width - half_width) {
        player->position.x = screen_width - half_width;
        player->velocity.x = 0.0f;
    }
    if (player->position.y < half_height) {
        player->position.y = half_height;
        player->velocity.y = 0.0f;
    }
    if (player->position.y > screen_height - half_height) {
        player->position.y = screen_height - half_height;
        player->velocity.y = 0.0f;
    }
}

/**
 * player_draw - Render the player to the screen
 *
 * DRAW ORDER:
 * ===========
 * 1. Engine glow (if thrusting) - drawn BEHIND the ship
 * 2. Ship sprite - drawn on top
 *
 * CENTERING:
 * ==========
 * Textures draw from top-left corner by default.
 * To center on player position:
 *     draw_x = position.x - texture.width / 2
 *     draw_y = position.y - texture.height / 2
 *
 * TINT:
 * =====
 * The 'tint' parameter modulates the texture color.
 * WHITE means no change. Other colors multiply with texture colors.
 */
void player_draw(const Player* player) {
    if (player == NULL) return;
    if (player->texture == NULL) return;

    // Calculate draw position (center texture on player position)
    float draw_x = player->position.x - player->texture->width / 2.0f;
    float draw_y = player->position.y - player->texture->height / 2.0f;

    // Draw engine glow if thrusting
    if (player->is_thrusting && player->glow_texture != NULL) {
        // Glow is positioned below the ship
        float glow_x = player->position.x - player->glow_texture->width / 2.0f;
        float glow_y = player->position.y + player->texture->height / 4.0f;

        // Pulsing effect using time
        float pulse = 0.7f + 0.3f * sinf((float)GetTime() * 10.0f);
        Color glow_tint = { 255, 255, 255, (unsigned char)(255 * pulse) };

        DrawTexture(*player->glow_texture, (int)glow_x, (int)glow_y, glow_tint);
    }

    // Draw the ship
    DrawTexture(*player->texture, (int)draw_x, (int)draw_y, WHITE);

    // Debug: Draw velocity vector (uncomment to visualize)
    // DrawLine(
    //     (int)player->position.x,
    //     (int)player->position.y,
    //     (int)(player->position.x + player->velocity.x * 0.5f),
    //     (int)(player->position.y + player->velocity.y * 0.5f),
    //     GREEN
    // );
}

/**
 * player_get_center - Get the center point of the player
 */
Vector2 player_get_center(const Player* player) {
    if (player == NULL) {
        return (Vector2){ 0, 0 };
    }
    return player->position;
}
