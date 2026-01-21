# Module 2: The Raylib Loop & Procedural Art

```
┌─────────────────────────────────────────────────────────────────┐
│  "In JavaScript, requestAnimationFrame hides the loop.         │
│   In C game dev, YOU write the loop."                          │
└─────────────────────────────────────────────────────────────────┘
```

## Learning Goals

By the end of this module, you will understand:
1. The game loop pattern (Init → Update → Draw → Cleanup)
2. Raylib's coordinate system (top-left origin)
3. Procedural texture generation (no external files!)
4. The distinction between CPU memory (RAM) and GPU memory (VRAM)
5. Struct composition for game state

---

## Concept 1: The Game Loop

### JavaScript's Hidden Loop

In browser JS, you request animation frames:

```javascript
function gameLoop(timestamp) {
    update();
    draw();
    requestAnimationFrame(gameLoop);  // Browser handles timing
}
requestAnimationFrame(gameLoop);
```

### C's Explicit Loop

In C with Raylib, you own the entire loop:

```c
int main(void) {
    // INIT PHASE
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    // GAME LOOP
    while (!WindowShouldClose()) {  // Until user closes window
        // UPDATE PHASE
        float dt = GetFrameTime();
        UpdatePlayer(dt);
        UpdateBullets(dt);

        // DRAW PHASE
        BeginDrawing();
            ClearBackground(BLACK);
            DrawPlayer();
            DrawBullets();
        EndDrawing();
    }

    // CLEANUP PHASE
    CloseWindow();
    return 0;
}
```

### The Lifecycle

```
┌──────────────────────────────────────────────────────────────────┐
│                    GAME LIFECYCLE                                │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│   ┌─────────┐                                                    │
│   │  INIT   │  Load assets, create window, init state           │
│   └────┬────┘                                                    │
│        │                                                         │
│        ▼                                                         │
│   ┌─────────────────────────────────────────┐                   │
│   │              GAME LOOP                  │◄──────┐            │
│   │  ┌─────────┐  ┌─────────┐  ┌─────────┐ │       │            │
│   │  │ INPUT   │─▶│ UPDATE  │─▶│  DRAW   │ │       │            │
│   │  └─────────┘  └─────────┘  └─────────┘ │       │            │
│   └─────────────────────────────────────────┘       │            │
│        │                                            │            │
│        │ (WindowShouldClose() == false)             │            │
│        └────────────────────────────────────────────┘            │
│        │                                                         │
│        │ (WindowShouldClose() == true)                          │
│        ▼                                                         │
│   ┌─────────┐                                                    │
│   │ CLEANUP │  Free memory, close window                        │
│   └─────────┘                                                    │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

---

## Concept 2: The Coordinate System

### Web Canvas vs Raylib

Both use the same coordinate system (luckily!):

```
(0,0) ────────────────────▶ X+
  │
  │     ┌─────────────────────────┐
  │     │                         │
  │     │      GAME WINDOW        │
  │     │                         │
  │     │    (400, 300)           │
  │     │        •                │
  │     │                         │
  ▼     └─────────────────────────┘
  Y+                        (800, 600)

- Origin (0,0) is TOP-LEFT
- X increases going RIGHT
- Y increases going DOWN (!)
- This is opposite of math class where Y goes UP
```

### Drawing Shapes

```c
// Raylib drawing functions (must be between BeginDrawing/EndDrawing)

// Rectangles: position is TOP-LEFT corner
DrawRectangle(100, 100, 50, 30, RED);  // x, y, width, height, color

// Circles: position is CENTER
DrawCircle(200, 200, 25, BLUE);  // centerX, centerY, radius, color

// Lines: start point to end point
DrawLine(0, 0, 400, 300, WHITE);  // x1, y1, x2, y2, color

// Triangles: three vertices
DrawTriangle(
    (Vector2){100, 200},  // vertex 1
    (Vector2){150, 100},  // vertex 2
    (Vector2){200, 200},  // vertex 3
    GREEN
);
```

---

## Concept 3: Procedural Textures (No Asset Files!)

### The Problem

Traditional game development:
```c
Texture2D ship = LoadTexture("assets/ship.png");  // Requires external file
```

Our constraint: **No external assets**. Everything generated in code.

### The Solution: Generate Images in RAM, Upload to VRAM

```
┌─────────────────────────────────────────────────────────────────┐
│                  TEXTURE CREATION PIPELINE                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   CPU (Your Code)              │  GPU (Graphics Card)           │
│   ════════════════             │  ═══════════════════           │
│                                │                                │
│   1. GenImageColor()    ───────┤                                │
│      Creates Image in RAM      │                                │
│                                │                                │
│   2. ImageDrawPixel()          │                                │
│      Manipulate pixels         │                                │
│                                │                                │
│   3. LoadTextureFromImage() ───┼──▶ Upload to VRAM              │
│      Copies to GPU memory      │    (Now a Texture2D)           │
│                                │                                │
│   4. UnloadImage()             │                                │
│      Free the RAM copy         │                                │
│      (GPU has its own copy)    │    5. DrawTexture()            │
│                                │       GPU renders it           │
│                                │                                │
│   On exit:                     │                                │
│   6. UnloadTexture() ──────────┼──▶ Free VRAM                   │
│                                │                                │
└─────────────────────────────────────────────────────────────────┘
```

### Code Example: Creating a Gradient Circle

```c
// Step 1: Create blank image in RAM
Image img = GenImageColor(64, 64, BLANK);  // 64x64, transparent

// Step 2: Draw pixels to create our ship
int centerX = 32, centerY = 32;
for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 64; x++) {
        float dist = sqrtf((x - centerX) * (x - centerX) +
                          (y - centerY) * (y - centerY));
        if (dist < 30) {
            // Inside the circle - color based on distance
            unsigned char alpha = (unsigned char)(255 * (1.0f - dist / 30.0f));
            Color pixel = { 0, 200, 255, alpha };  // Cyan glow
            ImageDrawPixel(&img, x, y, pixel);
        }
    }
}

// Step 3: Upload to GPU
Texture2D shipTexture = LoadTextureFromImage(img);

// Step 4: Free RAM copy (GPU has its own)
UnloadImage(img);

// ... in draw loop ...
DrawTexture(shipTexture, playerX, playerY, WHITE);

// ... on cleanup ...
UnloadTexture(shipTexture);  // Free VRAM
```

### Why This Matters

Understanding RAM vs VRAM is crucial:

| Aspect | Image (RAM) | Texture (VRAM) |
|--------|-------------|----------------|
| Location | CPU memory | GPU memory |
| Access | Fast to modify pixels | Fast to render |
| Use case | Creating/editing | Drawing |
| Size limit | System RAM (~16GB) | GPU VRAM (~8GB) |

If you keep modifying a texture every frame, you're constantly uploading
to VRAM - slow! Instead, pre-generate textures at init time.

---

## Concept 4: Delta Time (Frame-Rate Independence)

### The Bug

```c
// BAD: Movement depends on frame rate
void Update() {
    if (IsKeyDown(KEY_RIGHT)) {
        player.x += 5;  // 5 pixels per frame
    }
}
// At 60 FPS: 300 pixels/second
// At 30 FPS: 150 pixels/second  <-- Player moves slower!
```

### The Fix

```c
// GOOD: Movement independent of frame rate
void Update() {
    float dt = GetFrameTime();  // Seconds since last frame
    if (IsKeyDown(KEY_RIGHT)) {
        player.x += 300.0f * dt;  // 300 pixels per SECOND
    }
}
// At 60 FPS: dt ≈ 0.0167, movement = 5 pixels
// At 30 FPS: dt ≈ 0.0333, movement = 10 pixels
// Result: Same 300 pixels/second regardless of FPS!
```

---

## Concept 5: Struct Composition (GameState)

### The Pattern

Instead of global variables scattered everywhere:

```c
// BAD: Globals everywhere
Player g_player;
BulletList g_bullets;
int g_score;
bool g_paused;
```

Group related data into a GameState struct:

```c
// GOOD: Single source of truth
typedef struct {
    Player player;
    BulletList bullets;
    int score;
    bool paused;
} GameState;

// Pass state explicitly
void update(GameState* state, float dt) {
    update_player(&state->player, dt);
    update_bullets(&state->bullets, dt);
}
```

### Benefits

1. **Explicit Dependencies**: Functions declare what state they need
2. **Testability**: Easy to create test GameStates
3. **Save/Load**: Serialize one struct to save the game
4. **Multiplayer**: Each player can have their own GameState

---

## The Deliverable

A Raylib window displaying:
1. A procedurally generated spaceship (no external files)
2. Movement via WASD/arrow keys
3. Smooth, frame-rate-independent motion
4. Visual feedback when moving (engine glow)

```
┌─────────────────────────────────────────┐
│                                         │
│                                         │
│              ▲                          │
│             /█\   ← Procedural ship     │
│            /███\                        │
│           ╱  █  ╲                       │
│              ║   ← Engine trail (when   │
│              ║      moving)             │
│                                         │
│                                         │
│  WASD/Arrows to move    FPS: 60         │
└─────────────────────────────────────────┘
```

---

## File Structure

```
module2_raylib/
├── main.c           # Entry point, game loop
├── player.h         # Player struct and function declarations
├── player.c         # Player implementation
├── textures.h       # Procedural texture generation declarations
├── textures.c       # Procedural texture implementations
├── game_state.h     # GameState struct definition
└── Makefile         # Build configuration (links Raylib)
```

Now let's look at the code!
