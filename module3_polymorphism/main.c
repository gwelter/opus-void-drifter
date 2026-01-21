/**
 * main.c - Void Drifter Module 3: Function Pointers & Weapon System
 *
 * This module demonstrates the Strategy Pattern using function pointers.
 *
 * KEY CONCEPT:
 * ============
 * The player can switch weapons, and each weapon fires differently.
 * Instead of a giant switch statement in the firing code:
 *
 *     switch(weapon_type) {
 *         case SPREAD: ...
 *         case RAPID: ...
 *     }
 *
 * We use function pointers:
 *
 *     weapon->fire(position, bullets);  // Calls the right function automatically!
 *
 * This is:
 *     - Cleaner (no switch sprawl)
 *     - Extensible (add weapons without modifying fire code)
 *     - Like JavaScript callbacks, but in C
 */

#include "raylib.h"
#include "player.h"
#include "weapon.h"
#include "bullet.h"
#include "textures.h"
#include <stdio.h>
#include <stdlib.h>

// Screen configuration
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TARGET_FPS 60

// Maximum bullets on screen
#define MAX_BULLETS 200

/**
 * GameAssets - All loaded textures
 */
typedef struct {
    Texture2D ship_texture;
    Texture2D glow_texture;
} GameAssets;

/**
 * load_assets - Generate procedural textures
 */
static int load_assets(GameAssets* assets) {
    printf("Generating textures...\n");

    assets->ship_texture = generate_ship_texture(64, 64, (Color){ 0, 180, 255, 255 });
    assets->glow_texture = generate_engine_glow_texture(32, 64);

    printf("  Ship: %dx%d\n", assets->ship_texture.width, assets->ship_texture.height);
    printf("  Glow: %dx%d\n", assets->glow_texture.width, assets->glow_texture.height);

    return 0;
}

/**
 * unload_assets - Free GPU resources
 */
static void unload_assets(GameAssets* assets) {
    UnloadTexture(assets->ship_texture);
    UnloadTexture(assets->glow_texture);
}

/**
 * draw_ui - Draw the user interface
 */
static void draw_ui(const Player* player, const BulletList* bullets) {
    // FPS
    DrawFPS(10, 10);

    // Current weapon display
    const char* weapon_name = player_get_weapon_name(player);
    DrawText("Weapon:", 10, SCREEN_HEIGHT - 120, 20, GRAY);
    DrawText(weapon_name, 90, SCREEN_HEIGHT - 120, 20, WHITE);

    // Weapon selection hints
    DrawText("[1] Spread  [2] Rapid  [3] Laser", 10, SCREEN_HEIGHT - 90, 16, DARKGRAY);

    // Bullet count
    char bullet_text[32];
    snprintf(bullet_text, sizeof(bullet_text), "Bullets: %d", bullets->count);
    DrawText(bullet_text, 10, SCREEN_HEIGHT - 60, 16, GRAY);

    // Controls
    DrawText("WASD/Arrows: Move   SPACE: Fire   1-3: Switch Weapon", 10, SCREEN_HEIGHT - 30, 14, DARKGRAY);

    // Weapon-specific indicator color
    Color indicator_color = WHITE;
    switch (player->weapon.type) {
        case WEAPON_SPREAD: indicator_color = (Color){ 255, 200, 50, 255 }; break;   // Yellow
        case WEAPON_RAPID:  indicator_color = (Color){ 50, 200, 255, 255 }; break;   // Cyan
        case WEAPON_LASER:  indicator_color = (Color){ 255, 50, 100, 255 }; break;   // Magenta
        default: break;
    }
    DrawRectangle(10, SCREEN_HEIGHT - 125, 5, 25, indicator_color);
}

/**
 * draw_background - Simple star field
 */
static void draw_background(void) {
    static int initialized = 0;
    static Vector2 stars[100];

    if (!initialized) {
        for (int i = 0; i < 100; i++) {
            stars[i].x = (float)(rand() % SCREEN_WIDTH);
            stars[i].y = (float)(rand() % SCREEN_HEIGHT);
        }
        initialized = 1;
    }

    for (int i = 0; i < 100; i++) {
        unsigned char b = (unsigned char)(50 + (i * 2) % 150);
        DrawPixel((int)stars[i].x, (int)stars[i].y, (Color){ b, b, b, 255 });
    }
}

/**
 * main - Entry point and game loop
 *
 * This module's structure:
 *     1. Initialize window and assets
 *     2. Create player with default weapon
 *     3. Game loop:
 *         a. Input (including weapon switching)
 *         b. Update player and bullets
 *         c. Draw everything
 *     4. Cleanup
 */
int main(void) {
    // --- INIT ---
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  VOID DRIFTER - Module 3: Function Pointers & Weapons     ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Void Drifter - Module 3: Weapons");
    SetTargetFPS(TARGET_FPS);

    // Load assets
    GameAssets assets;
    if (load_assets(&assets) != 0) {
        CloseWindow();
        return 1;
    }

    // Initialize player (center of screen, spread weapon by default)
    Player player;
    player_init(&player,
                SCREEN_WIDTH / 2.0f,
                SCREEN_HEIGHT * 0.75f,  // Lower on screen
                &assets.ship_texture,
                &assets.glow_texture);

    // Initialize bullet list
    BulletList bullets;
    bullet_list_init(&bullets, MAX_BULLETS);

    printf("Controls:\n");
    printf("  WASD/Arrows - Move\n");
    printf("  SPACE       - Fire\n");
    printf("  1/2/3       - Switch weapon\n");
    printf("\n");
    printf("Weapons:\n");
    printf("  [1] Spread Shot - 3 bullets in a fan pattern\n");
    printf("  [2] Rapid Fire  - Fast single shots\n");
    printf("  [3] Laser       - Powerful focused beam\n");
    printf("\n");
    printf("Watch how pressing 1/2/3 changes the firing behavior!\n");
    printf("This is the Strategy Pattern with function pointers.\n\n");

    // --- GAME LOOP ---
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // --- INPUT & UPDATE ---
        player_handle_input(&player, &bullets);
        player_update(&player, dt, SCREEN_WIDTH, SCREEN_HEIGHT);
        bullet_list_update(&bullets, dt, SCREEN_WIDTH, SCREEN_HEIGHT);

        // --- DRAW ---
        BeginDrawing();
        {
            ClearBackground((Color){ 10, 10, 25, 255 });

            draw_background();
            bullet_list_draw(&bullets);
            player_draw(&player);
            draw_ui(&player, &bullets);
        }
        EndDrawing();
    }

    // --- CLEANUP ---
    printf("\nShutting down...\n");
    bullet_list_destroy(&bullets);
    printf("  Bullets freed\n");
    unload_assets(&assets);
    printf("  Assets unloaded\n");
    CloseWindow();
    printf("  Window closed\n\n");

    return 0;
}
