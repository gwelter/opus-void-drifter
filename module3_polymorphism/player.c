/**
 * player.c - Player Implementation with Weapon System
 *
 * This builds on Module 2's player with the addition of weapon handling.
 */

#include "player.h"
#include "bullet.h"
#include <math.h>
#include <stddef.h>

// Movement constants
#define PLAYER_SPEED        300.0f
#define PLAYER_ACCEL        800.0f
#define PLAYER_FRICTION     0.95f

/**
 * player_init - Initialize a player
 */
void player_init(Player* player, float start_x, float start_y,
                 Texture2D* texture, Texture2D* glow_texture) {
    if (player == NULL) return;

    // Position
    player->position = (Vector2){ start_x, start_y };
    player->velocity = (Vector2){ 0, 0 };

    // Physics
    player->speed = PLAYER_SPEED;
    player->acceleration = PLAYER_ACCEL;
    player->friction = PLAYER_FRICTION;

    // Weapon - start with spread shot
    player->weapon = weapon_create(WEAPON_SPREAD);
    player->is_firing = 0;

    // Visual
    player->texture = texture;
    player->glow_texture = glow_texture;

    // State
    player->is_thrusting = 0;
}

/**
 * player_handle_input - Process input for movement and combat
 *
 * CONCEPT: Input Separation
 * =========================
 * We separate input handling from physics/game logic.
 * Input sets INTENTIONS (is_firing, thrust direction).
 * Update applies those intentions to game state.
 *
 * This separation allows:
 *     - Easy rebinding of keys
 *     - AI could set the same flags without key input
 *     - Network play (receive intentions from server)
 */
void player_handle_input(Player* player, BulletList* bullets) {
    if (player == NULL) return;

    // --- MOVEMENT INPUT ---
    player->is_thrusting = 0;

    float accel_x = 0.0f;
    float accel_y = 0.0f;

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
        accel_y = -1.0f;
        player->is_thrusting = 1;
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
        accel_y = 1.0f;
        player->is_thrusting = 1;
    }
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
        accel_x = -1.0f;
        player->is_thrusting = 1;
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
        accel_x = 1.0f;
        player->is_thrusting = 1;
    }

    // Normalize diagonal movement
    if (accel_x != 0.0f && accel_y != 0.0f) {
        float inv_sqrt2 = 0.70710678f;
        accel_x *= inv_sqrt2;
        accel_y *= inv_sqrt2;
    }

    // Apply acceleration
    if (player->is_thrusting) {
        float dt = GetFrameTime();
        player->velocity.x += accel_x * player->acceleration * dt;
        player->velocity.y += accel_y * player->acceleration * dt;
    }

    // --- WEAPON SWITCHING INPUT ---
    // IsKeyPressed returns true only on the frame the key is first pressed
    // (not while held), perfect for switching

    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
        player_switch_weapon(player, WEAPON_SPREAD);
    }
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
        player_switch_weapon(player, WEAPON_RAPID);
    }
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) {
        player_switch_weapon(player, WEAPON_LASER);
    }

    // --- FIRING INPUT ---
    // IsKeyDown for continuous fire while held
    player->is_firing = IsKeyDown(KEY_SPACE);

    // Attempt to fire if button is held
    if (player->is_firing && bullets != NULL) {
        // This is where function pointers shine!
        // weapon_fire internally calls weapon->fire(),
        // which could be spread_fire, rapid_fire, or laser_fire
        // depending on what weapon is equipped.
        weapon_fire(&player->weapon, player->position, bullets);
    }
}

/**
 * player_update - Update physics and weapon cooldown
 */
void player_update(Player* player, float dt, int screen_width, int screen_height) {
    if (player == NULL) return;

    // --- UPDATE WEAPON ---
    weapon_update(&player->weapon, dt);

    // --- APPLY FRICTION ---
    float friction_factor = powf(player->friction, dt * 60.0f);
    player->velocity.x *= friction_factor;
    player->velocity.y *= friction_factor;

    // Clamp velocity
    float speed = sqrtf(player->velocity.x * player->velocity.x +
                        player->velocity.y * player->velocity.y);
    if (speed > player->speed) {
        float scale = player->speed / speed;
        player->velocity.x *= scale;
        player->velocity.y *= scale;
    }

    // Stop if very slow
    if (fabsf(player->velocity.x) < 1.0f) player->velocity.x = 0;
    if (fabsf(player->velocity.y) < 1.0f) player->velocity.y = 0;

    // --- UPDATE POSITION ---
    player->position.x += player->velocity.x * dt;
    player->position.y += player->velocity.y * dt;

    // --- CLAMP TO SCREEN ---
    float half_w = 32.0f, half_h = 32.0f;  // Default half-size
    if (player->texture != NULL) {
        half_w = player->texture->width / 2.0f;
        half_h = player->texture->height / 2.0f;
    }

    if (player->position.x < half_w) {
        player->position.x = half_w;
        player->velocity.x = 0;
    }
    if (player->position.x > screen_width - half_w) {
        player->position.x = screen_width - half_w;
        player->velocity.x = 0;
    }
    if (player->position.y < half_h) {
        player->position.y = half_h;
        player->velocity.y = 0;
    }
    if (player->position.y > screen_height - half_h) {
        player->position.y = screen_height - half_h;
        player->velocity.y = 0;
    }
}

/**
 * player_draw - Render the player
 */
void player_draw(const Player* player) {
    if (player == NULL || player->texture == NULL) return;

    float draw_x = player->position.x - player->texture->width / 2.0f;
    float draw_y = player->position.y - player->texture->height / 2.0f;

    // Draw engine glow if thrusting
    if (player->is_thrusting && player->glow_texture != NULL) {
        float glow_x = player->position.x - player->glow_texture->width / 2.0f;
        float glow_y = player->position.y + player->texture->height / 4.0f;

        float pulse = 0.7f + 0.3f * sinf((float)GetTime() * 10.0f);
        Color glow_tint = { 255, 255, 255, (unsigned char)(255 * pulse) };

        DrawTexture(*player->glow_texture, (int)glow_x, (int)glow_y, glow_tint);
    }

    // Draw ship
    DrawTexture(*player->texture, (int)draw_x, (int)draw_y, WHITE);

    // Draw weapon readiness indicator (small circle near ship)
    if (weapon_can_fire(&player->weapon)) {
        // Green = ready to fire
        DrawCircle((int)player->position.x, (int)(player->position.y + 40), 4, GREEN);
    } else {
        // Red = on cooldown
        DrawCircle((int)player->position.x, (int)(player->position.y + 40), 4, RED);
    }
}

/**
 * player_switch_weapon - Change equipped weapon
 *
 * This is the Strategy Pattern in action!
 * We create a new weapon of the specified type,
 * which has a different fire function pointer.
 */
void player_switch_weapon(Player* player, WeaponType type) {
    if (player == NULL) return;

    // Create new weapon
    // The old weapon is on the stack (inside player struct),
    // so it's automatically "freed" when we overwrite it
    player->weapon = weapon_create(type);
}

/**
 * player_get_weapon_name - Get current weapon name for display
 */
const char* player_get_weapon_name(const Player* player) {
    if (player == NULL) return "None";
    return weapon_get_name(&player->weapon);
}

/**
 * player_get_center - Get player center position
 */
Vector2 player_get_center(const Player* player) {
    if (player == NULL) return (Vector2){ 0, 0 };
    return player->position;
}
