/**
 * game_state.h - Central Game State Container
 *
 * CONCEPT: Single Source of Truth
 * ================================
 * In JavaScript, you might use:
 *     const gameState = { player: {...}, bullets: [...], score: 0 };
 *
 * In C, we create a struct that holds ALL game data.
 * This makes it easy to:
 *     - Pass state to functions explicitly
 *     - Save/load games (serialize one struct)
 *     - Reset state (re-initialize one struct)
 *     - Test (create mock states)
 */

#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "raylib.h"  // For Vector2, Texture2D, etc.

// Forward declaration - we'll define Player fully in player.h
// This avoids circular includes (game_state.h <-> player.h)
typedef struct Player Player;

/**
 * GameAssets - All textures and loaded resources
 *
 * CONCEPT: Asset Management
 * =========================
 * In JS, you might use a preloader that promises textures.
 * In C, we load textures into a struct at init time.
 *
 * Keeping assets separate from game state means:
 *     - Assets are read-only during gameplay
 *     - Easy to see what resources the game uses
 *     - Clear lifecycle: load once, use many, unload at end
 */
typedef struct {
    Texture2D ship_texture;       // The player's spaceship
    Texture2D glow_texture;       // Engine glow effect
    Texture2D bullet_texture;     // Bullet sprite (for later)
} GameAssets;

/**
 * GameConfig - Tunable game parameters
 *
 * CONCEPT: Configuration vs Magic Numbers
 * =======================================
 * Instead of hardcoding values like:
 *     player.x += 300.0f * dt;  // What is 300? Why 300?
 *
 * We use named configuration:
 *     player.x += config->player_speed * dt;  // Clear intent
 *
 * Benefits:
 *     - Easy to tweak gameplay
 *     - Could load from a config file
 *     - Self-documenting code
 */
typedef struct {
    int screen_width;
    int screen_height;
    int target_fps;
    float player_speed;       // Pixels per second
    float player_friction;    // Velocity decay (0-1)
    Color background_color;
} GameConfig;

/**
 * GameState - The complete game state
 *
 * CONCEPT: Composition over Inheritance
 * =====================================
 * In OOP languages, you might inherit:
 *     class GameState extends BaseState { ... }
 *
 * In C, we COMPOSE by embedding structs:
 *     typedef struct {
 *         Player player;     // Contains a Player
 *         GameConfig config; // Contains a GameConfig
 *     } GameState;
 *
 * This is actually what modern languages recommend too!
 * "Prefer composition over inheritance" - Gang of Four
 */
typedef struct GameState {
    // Assets (loaded once, read-only during gameplay)
    GameAssets assets;

    // Configuration (could be modified via settings menu)
    GameConfig config;

    // Player state (defined in player.h, included here by pointer or value)
    // We'll use a full struct, not a pointer, for simplicity
    // This means GameState "contains" a Player, not "points to" one
    // The actual Player struct is defined in player.h

    // For now, we'll store player data directly since we need player.h
    // See player.h for the full Player struct

    // Frame timing
    float delta_time;    // Seconds since last frame
    double total_time;   // Total elapsed time
    int frame_count;     // Number of frames rendered

    // Game flags
    int is_paused;
    int is_running;

} GameState;

/**
 * game_config_default - Get default configuration values
 *
 * CONCEPT: Factory Functions for Structs
 * ======================================
 * Since C doesn't have constructors, we create "factory functions"
 * that return initialized structs.
 *
 * @return A GameConfig with sensible defaults
 */
GameConfig game_config_default(void);

/**
 * game_state_init - Initialize game state with default values
 *
 * This doesn't load assets - that happens in game_assets_load()
 *
 * @param state  Pointer to state to initialize
 * @param config Configuration to use
 */
void game_state_init(GameState* state, GameConfig config);

/**
 * game_assets_load - Load/generate all game assets
 *
 * IMPORTANT: Call this AFTER InitWindow() - Raylib needs an OpenGL context!
 *
 * @param assets Pointer to assets struct to populate
 * @return 0 on success, -1 on failure
 */
int game_assets_load(GameAssets* assets);

/**
 * game_assets_unload - Free all game assets
 *
 * IMPORTANT: Call this BEFORE CloseWindow()!
 *
 * @param assets Pointer to assets to free
 */
void game_assets_unload(GameAssets* assets);

#endif // GAME_STATE_H
