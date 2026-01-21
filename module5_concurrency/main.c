/**
 * main.c - Void Drifter: The Complete Game
 *
 * Module 5 brings together everything we've learned:
 *     - Memory management (Module 1)
 *     - Raylib rendering & procedural textures (Module 2)
 *     - Function pointers for weapons (Module 3)
 *     - Network protocol (Module 4)
 *     - Threading for non-blocking networking (Module 5)
 *
 * ARCHITECTURE:
 * =============
 *
 *   MAIN THREAD                    NETWORK THREAD
 *   ══════════════                 ════════════════
 *   • Input handling               • Connects to server
 *   • Game logic                   • Sends input
 *   • Weapon system                • Receives state
 *   • Rendering                    • Updates SharedState
 *          │                              │
 *          └──────────┬───────────────────┘
 *                     │
 *              SHARED STATE
 *            (Mutex Protected)
 *
 * The game can run in two modes:
 *     1. Offline mode: Single player, no server required
 *     2. Online mode: Connects to server for multiplayer
 */

#include "raylib.h"
#include "textures.h"
#include "weapon.h"
#include "bullet.h"
#include "shared_state.h"
#include "network_client.h"
#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Screen configuration
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TARGET_FPS 60

// Game configuration
#define MAX_BULLETS 200

// Default server
#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 8080

/**
 * LocalPlayer - Our player's state (client-side)
 */
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float speed;
    float acceleration;
    float friction;

    Weapon weapon;
    int is_thrusting;
    int health;

    Texture2D* texture;
    Texture2D* glow_texture;
} LocalPlayer;

/**
 * GameAssets - All loaded textures
 */
typedef struct {
    Texture2D ship_texture;
    Texture2D glow_texture;
    Texture2D other_ship_texture;  // Different color for other players
} GameAssets;

/**
 * GameState - Complete game state
 */
typedef struct {
    // Local player
    LocalPlayer player;

    // Bullets
    BulletList bullets;

    // Assets
    GameAssets assets;

    // Networking (optional)
    NetworkClient* net_client;
    SharedState shared;
    int online_mode;

    // Remote players (copied from shared state each frame)
    RemotePlayer remote_players[MAX_PLAYERS];
    int remote_player_count;

    // Remote bullets (from other players, copied from shared state)
    RemoteBullet remote_bullets[MAX_REMOTE_BULLETS];
    int remote_bullet_count;

    // Frame stats
    int frame_count;
    float delta_time;

} GameState;

/**
 * load_assets - Generate all procedural textures
 */
static int load_assets(GameAssets* assets) {
    printf("Generating procedural textures...\n");

    // Player ship (cyan)
    assets->ship_texture = generate_ship_texture(64, 64, (Color){ 0, 180, 255, 255 });
    printf("  Player ship: %dx%d\n", assets->ship_texture.width, assets->ship_texture.height);

    // Engine glow
    assets->glow_texture = generate_engine_glow_texture(32, 64);
    printf("  Engine glow: %dx%d\n", assets->glow_texture.width, assets->glow_texture.height);

    // Other player ship (green)
    assets->other_ship_texture = generate_ship_texture(64, 64, (Color){ 50, 255, 100, 255 });
    printf("  Other ship: %dx%d\n", assets->other_ship_texture.width, assets->other_ship_texture.height);

    return 0;
}

/**
 * unload_assets - Free GPU resources
 */
static void unload_assets(GameAssets* assets) {
    UnloadTexture(assets->ship_texture);
    UnloadTexture(assets->glow_texture);
    UnloadTexture(assets->other_ship_texture);
}

/**
 * init_local_player - Initialize the local player
 *
 * Uses shared physics constants from protocol.h for consistency with server.
 */
static void init_local_player(LocalPlayer* player, GameAssets* assets) {
    player->position = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT * 0.75f };
    player->velocity = (Vector2){ 0, 0 };
    player->speed = PLAYER_SPEED;
    player->acceleration = PLAYER_ACCELERATION;
    player->friction = PLAYER_FRICTION;

    player->weapon = weapon_create(WEAPON_SPREAD);
    player->is_thrusting = 0;
    player->health = 100;

    player->texture = &assets->ship_texture;
    player->glow_texture = &assets->glow_texture;
}

/**
 * handle_input - Process keyboard input
 */
static uint8_t handle_input(LocalPlayer* player, BulletList* bullets) {
    uint8_t input_flags = 0;
    player->is_thrusting = 0;

    float accel_x = 0, accel_y = 0;

    // Movement
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
        accel_y = -1.0f;
        player->is_thrusting = 1;
        input_flags |= INPUT_UP;
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
        accel_y = 1.0f;
        player->is_thrusting = 1;
        input_flags |= INPUT_DOWN;
    }
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
        accel_x = -1.0f;
        player->is_thrusting = 1;
        input_flags |= INPUT_LEFT;
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
        accel_x = 1.0f;
        player->is_thrusting = 1;
        input_flags |= INPUT_RIGHT;
    }

    // Normalize diagonal
    if (accel_x != 0 && accel_y != 0) {
        float inv_sqrt2 = 0.7071f;
        accel_x *= inv_sqrt2;
        accel_y *= inv_sqrt2;
    }

    // Apply acceleration
    float dt = GetFrameTime();
    player->velocity.x += accel_x * player->acceleration * dt;
    player->velocity.y += accel_y * player->acceleration * dt;

    // Weapon switching
    if (IsKeyPressed(KEY_ONE)) player->weapon = weapon_create(WEAPON_SPREAD);
    if (IsKeyPressed(KEY_TWO)) player->weapon = weapon_create(WEAPON_RAPID);
    if (IsKeyPressed(KEY_THREE)) player->weapon = weapon_create(WEAPON_LASER);

    // Firing
    if (IsKeyDown(KEY_SPACE)) {
        input_flags |= INPUT_FIRE;
        weapon_fire(&player->weapon, player->position, bullets);
    }

    return input_flags;
}

/**
 * update_local_player - Update player physics
 */
static void update_local_player(LocalPlayer* player, float dt) {
    // Friction
    float friction = powf(player->friction, dt * 60.0f);
    player->velocity.x *= friction;
    player->velocity.y *= friction;

    // Clamp velocity
    float speed = sqrtf(player->velocity.x * player->velocity.x +
                        player->velocity.y * player->velocity.y);
    if (speed > player->speed) {
        float scale = player->speed / speed;
        player->velocity.x *= scale;
        player->velocity.y *= scale;
    }

    // Stop if slow
    if (fabsf(player->velocity.x) < 1.0f) player->velocity.x = 0;
    if (fabsf(player->velocity.y) < 1.0f) player->velocity.y = 0;

    // Update position
    player->position.x += player->velocity.x * dt;
    player->position.y += player->velocity.y * dt;

    // Clamp to screen
    float hw = 32, hh = 32;
    if (player->position.x < hw) { player->position.x = hw; player->velocity.x = 0; }
    if (player->position.x > SCREEN_WIDTH - hw) { player->position.x = SCREEN_WIDTH - hw; player->velocity.x = 0; }
    if (player->position.y < hh) { player->position.y = hh; player->velocity.y = 0; }
    if (player->position.y > SCREEN_HEIGHT - hh) { player->position.y = SCREEN_HEIGHT - hh; player->velocity.y = 0; }

    // Update weapon cooldown
    weapon_update(&player->weapon, dt);
}

/**
 * draw_local_player - Render the local player
 */
static void draw_local_player(const LocalPlayer* player) {
    if (player->texture == NULL) return;

    float x = player->position.x - player->texture->width / 2.0f;
    float y = player->position.y - player->texture->height / 2.0f;

    // Engine glow
    if (player->is_thrusting && player->glow_texture != NULL) {
        float gx = player->position.x - player->glow_texture->width / 2.0f;
        float gy = player->position.y + player->texture->height / 4.0f;
        float pulse = 0.7f + 0.3f * sinf((float)GetTime() * 10.0f);
        Color tint = { 255, 255, 255, (unsigned char)(255 * pulse) };
        DrawTexture(*player->glow_texture, (int)gx, (int)gy, tint);
    }

    // Ship
    DrawTexture(*player->texture, (int)x, (int)y, WHITE);
}

/**
 * draw_remote_players - Render other players from server
 */
static void draw_remote_players(const GameState* game) {
    for (int i = 0; i < game->remote_player_count; i++) {
        const RemotePlayer* rp = &game->remote_players[i];
        if (!rp->active) continue;

        // Skip ourselves
        if (rp->id == game->shared.my_id) continue;

        float x = rp->x - game->assets.other_ship_texture.width / 2.0f;
        float y = rp->y - game->assets.other_ship_texture.height / 2.0f;

        DrawTexture(game->assets.other_ship_texture, (int)x, (int)y, WHITE);

        // Draw player ID above
        char id_text[8];
        snprintf(id_text, sizeof(id_text), "P%d", rp->id);
        DrawText(id_text, (int)rp->x - 10, (int)rp->y - 50, 16, GREEN);
    }
}

/**
 * draw_remote_bullets - Render bullets from other players
 *
 * Colors match weapon.c definitions:
 *   Spread: Yellow-orange
 *   Rapid:  Cyan
 *   Laser:  Magenta/pink (larger bullet)
 */
static void draw_remote_bullets(const GameState* game) {
    for (int i = 0; i < game->remote_bullet_count; i++) {
        const RemoteBullet* rb = &game->remote_bullets[i];
        if (!rb->active) continue;

        // Skip our own bullets (we render those locally for better responsiveness)
        if (rb->owner_id == game->shared.my_id) continue;

        // Draw bullet with weapon-specific color and size
        Color bullet_color;
        int width, height;

        switch (rb->weapon_type) {
            case WEAPON_TYPE_SPREAD:
                bullet_color = (Color){ 255, 200, 50, 255 };   // Yellow-orange
                width = 6; height = 10;
                break;
            case WEAPON_TYPE_RAPID:
                bullet_color = (Color){ 50, 200, 255, 255 };   // Cyan
                width = 4; height = 12;
                break;
            case WEAPON_TYPE_LASER:
                bullet_color = (Color){ 255, 50, 100, 255 };   // Magenta/pink
                width = 8; height = 16;  // Larger for laser
                break;
            default:
                bullet_color = WHITE;
                width = 6; height = 10;
                break;
        }

        DrawRectangle((int)(rb->x - width/2), (int)(rb->y - height/2), width, height, bullet_color);
    }
}

/**
 * draw_background - Simple star field
 */
static void draw_background(void) {
    static int init = 0;
    static Vector2 stars[150];

    if (!init) {
        for (int i = 0; i < 150; i++) {
            stars[i].x = (float)(rand() % SCREEN_WIDTH);
            stars[i].y = (float)(rand() % SCREEN_HEIGHT);
        }
        init = 1;
    }

    for (int i = 0; i < 150; i++) {
        unsigned char b = (unsigned char)(40 + (i * 137) % 180);
        DrawPixel((int)stars[i].x, (int)stars[i].y, (Color){ b, b, b, 255 });
    }
}

/**
 * draw_ui - Draw the user interface
 */
static void draw_ui(const GameState* game) {
    // FPS
    DrawFPS(10, 10);

    // Weapon
    DrawText("Weapon:", 10, SCREEN_HEIGHT - 90, 18, GRAY);
    DrawText(weapon_get_name(&game->player.weapon), 90, SCREEN_HEIGHT - 90, 18, WHITE);

    // Weapon keys
    DrawText("[1] Spread  [2] Rapid  [3] Laser", 10, SCREEN_HEIGHT - 65, 14, DARKGRAY);

    // Bullet count
    char buf[32];
    snprintf(buf, sizeof(buf), "Bullets: %d", game->bullets.count);
    DrawText(buf, 10, SCREEN_HEIGHT - 40, 14, GRAY);

    // Network status
    const char* status_str;
    Color status_color;
    switch (shared_state_get_status((SharedState*)&game->shared)) {
        case NET_CONNECTED:
            status_str = "Online";
            status_color = GREEN;
            break;
        case NET_CONNECTING:
            status_str = "Connecting...";
            status_color = YELLOW;
            break;
        case NET_ERROR:
            status_str = "Error";
            status_color = RED;
            break;
        default:
            status_str = "Offline";
            status_color = GRAY;
    }

    DrawText("Network:", SCREEN_WIDTH - 150, 10, 16, GRAY);
    DrawText(status_str, SCREEN_WIDTH - 70, 10, 16, status_color);

    // Player count
    if (game->online_mode) {
        snprintf(buf, sizeof(buf), "Players: %d", game->remote_player_count);
        DrawText(buf, SCREEN_WIDTH - 150, 30, 14, GRAY);
    }

    // Controls
    DrawText("WASD: Move   SPACE: Fire   ESC: Quit", 200, SCREEN_HEIGHT - 20, 12, DARKGRAY);
}

/**
 * main - Entry point
 */
int main(int argc, char* argv[]) {
    // Parse arguments
    const char* host = DEFAULT_HOST;
    uint16_t port = DEFAULT_PORT;
    int online = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--online") == 0 || strcmp(argv[i], "-o") == 0) {
            online = 1;
        } else if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            host = argv[++i];
            online = 1;
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = (uint16_t)atoi(argv[++i]);
            online = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Void Drifter - Module 5: Complete Game\n\n");
            printf("Usage: %s [options]\n\n", argv[0]);
            printf("Options:\n");
            printf("  --online, -o     Connect to server\n");
            printf("  --host HOST      Server address (default: %s)\n", DEFAULT_HOST);
            printf("  --port PORT      Server port (default: %d)\n", DEFAULT_PORT);
            printf("  --help, -h       Show this help\n");
            return 0;
        }
    }

    // Print banner
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║            VOID DRIFTER - The Complete Game                ║\n");
    printf("║                   Module 5: Concurrency                    ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    if (online) {
        printf("Mode: ONLINE (connecting to %s:%d)\n\n", host, port);
    } else {
        printf("Mode: OFFLINE (single player)\n");
        printf("Use --online to connect to a server.\n\n");
    }

    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Void Drifter - Module 5");
    SetTargetFPS(TARGET_FPS);

    // Initialize game state
    GameState game = {0};
    game.online_mode = online;

    // Load assets
    if (load_assets(&game.assets) != 0) {
        CloseWindow();
        return 1;
    }

    // Initialize player
    init_local_player(&game.player, &game.assets);

    // Initialize bullets
    bullet_list_init(&game.bullets, MAX_BULLETS);

    // Initialize shared state
    if (shared_state_init(&game.shared) != 0) {
        unload_assets(&game.assets);
        CloseWindow();
        return 1;
    }

    // Start networking if online
    if (online) {
        game.net_client = net_client_create();
        if (game.net_client != NULL) {
            net_client_connect(game.net_client, &game.shared, host, port);
        }
    }

    printf("Controls:\n");
    printf("  WASD / Arrows - Move\n");
    printf("  SPACE - Fire\n");
    printf("  1/2/3 - Switch weapon\n");
    printf("  ESC - Quit\n\n");

    // Game loop
    while (!WindowShouldClose()) {
        game.delta_time = GetFrameTime();
        game.frame_count++;

        // --- INPUT ---
        uint8_t input = handle_input(&game.player, &game.bullets);

        // Send input to network (if online)
        if (online) {
            shared_state_set_input(&game.shared, input, (uint8_t)game.player.weapon.type);
        }

        // --- UPDATE ---
        // In online mode, server is authoritative - use server position directly
        // In offline mode, run local physics
        if (!online) {
            update_local_player(&game.player, game.delta_time);
        } else {
            // Still need to update weapon cooldown in online mode for local bullet visuals
            weapon_update(&game.player.weapon, game.delta_time);
        }
        bullet_list_update(&game.bullets, game.delta_time, SCREEN_WIDTH, SCREEN_HEIGHT);

        // Copy remote player and bullet data from shared state
        if (online) {
            game.remote_player_count = shared_state_copy_players(&game.shared, game.remote_players);
            game.remote_bullet_count = shared_state_copy_bullets(&game.shared, game.remote_bullets);

            // Use server-authoritative position directly (no prediction)
            float server_x, server_y, server_vx, server_vy;
            if (shared_state_get_my_position(&game.shared, &server_x, &server_y,
                                              &server_vx, &server_vy)) {
                game.player.position.x = server_x;
                game.player.position.y = server_y;
                game.player.velocity.x = server_vx;
                game.player.velocity.y = server_vy;
            }
        }

        // --- DRAW ---
        BeginDrawing();
        {
            ClearBackground((Color){ 8, 8, 20, 255 });

            draw_background();
            bullet_list_draw(&game.bullets);  // Local bullets
            if (online) {
                draw_remote_bullets(&game);   // Bullets from other players
            }
            draw_remote_players(&game);
            draw_local_player(&game.player);
            draw_ui(&game);
        }
        EndDrawing();
    }

    // Cleanup
    printf("\nShutting down...\n");

    if (game.net_client != NULL) {
        net_client_disconnect(game.net_client);
        net_client_destroy(game.net_client);
    }

    shared_state_destroy(&game.shared);
    bullet_list_destroy(&game.bullets);
    unload_assets(&game.assets);
    CloseWindow();

    printf("Goodbye!\n");
    return 0;
}
