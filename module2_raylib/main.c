/**
 * main.c - Void Drifter Module 2: Raylib Game Loop & Procedural Art
 *
 * This module demonstrates:
 *     1. The basic game loop (Init → Update → Draw → Cleanup)
 *     2. Procedural texture generation (no external files)
 *     3. Keyboard input handling
 *     4. Frame-rate independent movement
 *
 * CONCEPT: The Entry Point
 * ========================
 * Raylib doesn't hide the main function like some game engines do.
 * You have full control over the initialization and game loop.
 *
 * This is similar to writing a Node.js server from scratch vs using Express:
 *     - More control
 *     - More responsibility
 *     - Better understanding of what's happening
 */

#include "raylib.h"
#include "player.h"
#include "textures.h"
#include "game_state.h"
#include <stdio.h>
#include <stdlib.h>  // For rand()

// Screen dimensions
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TARGET_FPS 60

/**
 * game_config_default - Create default configuration
 */
GameConfig game_config_default(void) {
    return (GameConfig){
        .screen_width = SCREEN_WIDTH,
        .screen_height = SCREEN_HEIGHT,
        .target_fps = TARGET_FPS,
        .player_speed = 300.0f,
        .player_friction = 0.95f,
        .background_color = (Color){ 10, 10, 20, 255 }  // Dark blue-black
    };
}

/**
 * game_state_init - Initialize game state
 */
void game_state_init(GameState* state, GameConfig config) {
    if (state == NULL) return;

    state->config = config;
    state->delta_time = 0.0f;
    state->total_time = 0.0;
    state->frame_count = 0;
    state->is_paused = 0;
    state->is_running = 1;
}

/**
 * game_assets_load - Generate all procedural textures
 *
 * IMPORTANT: This must be called AFTER InitWindow()!
 * Raylib needs an OpenGL context to create textures.
 */
int game_assets_load(GameAssets* assets) {
    if (assets == NULL) return -1;

    printf("Generating procedural textures...\n");

    // Generate ship texture (64x64 cyan ship)
    assets->ship_texture = generate_ship_texture(64, 64, (Color){ 0, 180, 255, 255 });
    printf("  - Ship texture: %dx%d\n",
           assets->ship_texture.width, assets->ship_texture.height);

    // Generate engine glow (32x64 for elongated flame)
    assets->glow_texture = generate_engine_glow_texture(32, 64);
    printf("  - Glow texture: %dx%d\n",
           assets->glow_texture.width, assets->glow_texture.height);

    // Generate bullet texture (16x24)
    assets->bullet_texture = generate_bullet_texture(16, 24, (Color){ 255, 200, 50, 255 });
    printf("  - Bullet texture: %dx%d\n",
           assets->bullet_texture.width, assets->bullet_texture.height);

    printf("All textures generated successfully!\n\n");
    return 0;
}

/**
 * game_assets_unload - Free all GPU resources
 *
 * IMPORTANT: Call BEFORE CloseWindow()!
 */
void game_assets_unload(GameAssets* assets) {
    if (assets == NULL) return;

    UnloadTexture(assets->ship_texture);
    UnloadTexture(assets->glow_texture);
    UnloadTexture(assets->bullet_texture);
}

/**
 * draw_ui - Draw the user interface overlay
 *
 * CONCEPT: UI Layer
 * =================
 * UI is drawn LAST so it appears on top of everything else.
 * This includes:
 *     - FPS counter
 *     - Instructions
 *     - Score (in a full game)
 */
static void draw_ui(const GameState* state, const Player* player) {
    // Draw FPS in top-left
    DrawFPS(10, 10);

    // Draw instructions
    DrawText("WASD / Arrow Keys to move", 10, SCREEN_HEIGHT - 60, 16, GRAY);
    DrawText("ESC to quit", 10, SCREEN_HEIGHT - 40, 16, GRAY);

    // Draw player position (debug info)
    char pos_text[64];
    snprintf(pos_text, sizeof(pos_text), "Pos: %.0f, %.0f",
             player->position.x, player->position.y);
    DrawText(pos_text, 10, 35, 16, DARKGRAY);

    // Draw velocity (debug info)
    char vel_text[64];
    snprintf(vel_text, sizeof(vel_text), "Vel: %.1f, %.1f",
             player->velocity.x, player->velocity.y);
    DrawText(vel_text, 10, 55, 16, DARKGRAY);

    // Draw frame time
    char frame_text[64];
    snprintf(frame_text, sizeof(frame_text), "Frame: %d  dt: %.3fms",
             state->frame_count, state->delta_time * 1000.0f);
    DrawText(frame_text, 10, 75, 16, DARKGRAY);
}

/**
 * draw_background - Draw the star field background
 *
 * For now, we draw simple dots. In a full game, you might use a
 * pre-generated star field texture with parallax scrolling.
 */
static void draw_background(void) {
    // Simple star field using random positions
    // (In a real game, you'd precompute these or use a texture)
    static int stars_initialized = 0;
    static Vector2 stars[100];

    if (!stars_initialized) {
        for (int i = 0; i < 100; i++) {
            stars[i].x = (float)(rand() % SCREEN_WIDTH);
            stars[i].y = (float)(rand() % SCREEN_HEIGHT);
        }
        stars_initialized = 1;
    }

    // Draw stars with varying brightness
    for (int i = 0; i < 100; i++) {
        unsigned char brightness = (unsigned char)(50 + (i * 2) % 150);
        DrawPixel((int)stars[i].x, (int)stars[i].y,
                  (Color){ brightness, brightness, brightness, 255 });
    }
}

/**
 * main - Program entry point and game loop
 *
 * STRUCTURE:
 * ==========
 *
 * ┌─────────────────────────────────────────────────┐
 * │  INITIALIZATION                                 │
 * │    - Create window                              │
 * │    - Load/generate assets                       │
 * │    - Initialize game state                      │
 * └─────────────────────┬───────────────────────────┘
 *                       │
 *                       ▼
 * ┌─────────────────────────────────────────────────┐
 * │  GAME LOOP (while window is open)               │
 * │    ┌───────────────────────────────────────┐    │
 * │    │  INPUT: Read keyboard/mouse           │    │
 * │    └───────────────────────────────────────┘    │
 * │                     │                           │
 * │                     ▼                           │
 * │    ┌───────────────────────────────────────┐    │
 * │    │  UPDATE: Physics, game logic          │    │
 * │    └───────────────────────────────────────┘    │
 * │                     │                           │
 * │                     ▼                           │
 * │    ┌───────────────────────────────────────┐    │
 * │    │  DRAW: Render everything              │    │
 * │    └───────────────────────────────────────┘    │
 * └─────────────────────┬───────────────────────────┘
 *                       │
 *                       ▼
 * ┌─────────────────────────────────────────────────┐
 * │  CLEANUP                                        │
 * │    - Unload assets                              │
 * │    - Close window                               │
 * └─────────────────────────────────────────────────┘
 */
int main(void) {
    // ========================================
    // INITIALIZATION
    // ========================================

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║     VOID DRIFTER - Module 2: Raylib & Procedural Art   ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    // Initialize window
    // This creates an OpenGL context, required before creating textures
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Void Drifter - Module 2");

    // Set target FPS (Raylib will sleep to maintain this rate)
    SetTargetFPS(TARGET_FPS);

    // Initialize game state
    GameState state;
    GameConfig config = game_config_default();
    game_state_init(&state, config);

    // Load/generate all assets
    if (game_assets_load(&state.assets) != 0) {
        printf("ERROR: Failed to load assets!\n");
        CloseWindow();
        return 1;
    }

    // Initialize player
    // Start in center of screen
    Player player;
    player_init(&player,
                SCREEN_WIDTH / 2.0f,
                SCREEN_HEIGHT / 2.0f,
                &state.assets.ship_texture,
                &state.assets.glow_texture);

    printf("Initialization complete. Starting game loop...\n\n");
    printf("Controls:\n");
    printf("  WASD / Arrow Keys - Move\n");
    printf("  ESC - Quit\n");
    printf("\n");

    // ========================================
    // GAME LOOP
    // ========================================

    while (!WindowShouldClose()) {  // WindowShouldClose returns true when ESC or X button
        // --- FRAME TIMING ---
        state.delta_time = GetFrameTime();  // Seconds since last frame
        state.total_time += state.delta_time;
        state.frame_count++;

        // --- INPUT ---
        player_handle_input(&player);

        // --- UPDATE ---
        player_update(&player, state.delta_time, SCREEN_WIDTH, SCREEN_HEIGHT);

        // --- DRAW ---
        BeginDrawing();
        {
            // Clear screen with background color
            ClearBackground(state.config.background_color);

            // Draw background (stars)
            draw_background();

            // Draw game entities
            player_draw(&player);

            // Draw UI on top
            draw_ui(&state, &player);
        }
        EndDrawing();
    }

    // ========================================
    // CLEANUP
    // ========================================

    printf("\nShutting down...\n");

    // Unload all GPU resources
    game_assets_unload(&state.assets);
    printf("  - Assets unloaded\n");

    // Close the window and OpenGL context
    CloseWindow();
    printf("  - Window closed\n");

    printf("\nGoodbye! Total frames: %d\n", state.frame_count);

    return 0;
}
