/**
 * textures.c - Procedural Texture Generation Implementation
 *
 * DEEP DIVE: How Images Work in Memory
 * =====================================
 * An image is just a 2D array of pixels.
 * Each pixel is 4 bytes: R, G, B, A (Red, Green, Blue, Alpha)
 *
 * For a 64x64 image:
 *     Total pixels: 64 * 64 = 4096
 *     Total bytes:  4096 * 4 = 16,384 bytes (16 KB)
 *
 * Pixels are stored row-by-row (row-major order):
 *     [Row 0: pixel 0,0 | 0,1 | 0,2 | ... | 0,63]
 *     [Row 1: pixel 1,0 | 1,1 | 1,2 | ... | 1,63]
 *     ...
 *
 * To access pixel at (x, y):
 *     index = y * width + x
 */

#include "textures.h"
#include <math.h>    // For sqrtf, sinf, cosf
#include <stdlib.h>  // For rand

// Helper: Clamp a value between min and max
static float clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Helper: Clamp an int between 0 and 255
static unsigned char clamp_byte(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (unsigned char)value;
}

/**
 * color_fade - Reduce alpha of a color
 */
Color color_fade(Color color, unsigned char alpha) {
    return (Color){ color.r, color.g, color.b, alpha };
}

/**
 * color_lerp - Linear interpolate between two colors
 *
 * EXPLANATION:
 * ============
 * For each component (R, G, B, A):
 *     result = start + (end - start) * t
 *
 * Example: lerp(RED, BLUE, 0.5)
 *     R: 255 + (0 - 255) * 0.5 = 127
 *     G: 0 + (0 - 0) * 0.5 = 0
 *     B: 0 + (255 - 0) * 0.5 = 127
 *     Result: Purple!
 */
Color color_lerp(Color c1, Color c2, float t) {
    // Clamp t to valid range
    t = clampf(t, 0.0f, 1.0f);

    return (Color){
        .r = clamp_byte((int)(c1.r + (c2.r - c1.r) * t)),
        .g = clamp_byte((int)(c1.g + (c2.g - c1.g) * t)),
        .b = clamp_byte((int)(c1.b + (c2.b - c1.b) * t)),
        .a = clamp_byte((int)(c1.a + (c2.a - c1.a) * t))
    };
}

/**
 * generate_ship_texture - Create a spaceship sprite
 *
 * DESIGN:
 * =======
 * The ship is a classic "arrow" shape pointing UP:
 *
 *         ▲  (tip)
 *        ╱█╲
 *       ╱███╲
 *      ╱█████╲
 *     ╱███████╲
 *    ╱ ███████ ╲
 *   ╱  ███████  ╲
 *  ───┬───────┬───  (base with notch)
 *     │  ███  │
 *     └───────┘  (engine area)
 *
 * Algorithm:
 * 1. For each pixel, determine if it's inside the ship shape
 * 2. If inside, calculate color based on position
 * 3. Apply shading to give 3D appearance
 */
Texture2D generate_ship_texture(int width, int height, Color color) {
    // Step 1: Create a blank image in CPU memory
    // GenImageColor creates a solid-color image
    // We use BLANK (transparent) as base
    Image img = GenImageColor(width, height, BLANK);

    // Ship geometry parameters
    float centerX = width / 2.0f;
    float tipY = height * 0.1f;         // Tip at 10% from top
    float baseY = height * 0.85f;       // Base at 85% from top
    float wingWidth = width * 0.45f;    // Half-width at base
    float notchDepth = height * 0.15f;  // How deep the engine notch is

    // Step 2: Draw each pixel
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float fx = (float)x;
            float fy = (float)y;

            // Calculate distance from center (for shading)
            float dx = fx - centerX;

            // Progress from tip (0) to base (1)
            float progress = (fy - tipY) / (baseY - tipY);

            if (progress < 0 || progress > 1) {
                // Above tip or below base - check for engine notch
                if (fy > baseY) {
                    // Engine notch area
                    float notchProgress = (fy - baseY) / notchDepth;
                    float notchWidth = wingWidth * 0.4f * (1.0f - notchProgress);
                    if (fabsf(dx) < notchWidth && notchProgress < 1.0f) {
                        // Inside engine notch - darker color
                        Color engineColor = color_lerp(color, BLACK, 0.5f);
                        engineColor.a = (unsigned char)(255 * (1.0f - notchProgress));
                        ImageDrawPixel(&img, x, y, engineColor);
                    }
                }
                continue;
            }

            // Ship width at this Y position (triangle shape)
            float widthAtY = wingWidth * progress;

            // Check if inside the ship body
            if (fabsf(dx) <= widthAtY) {
                // Inside the ship!

                // Calculate shading based on horizontal position
                // Creates a 3D rounded look
                float shadeAmount = fabsf(dx) / widthAtY;  // 0 at center, 1 at edge
                shadeAmount = shadeAmount * shadeAmount;   // Quadratic falloff

                // Apply shading (darker at edges)
                Color pixelColor = color_lerp(color, BLACK, shadeAmount * 0.4f);

                // Cockpit window (darker oval in upper portion)
                float cockpitY = tipY + (baseY - tipY) * 0.3f;
                float cockpitHeight = (baseY - tipY) * 0.2f;
                float cockpitWidth = wingWidth * 0.15f;

                if (fy > cockpitY && fy < cockpitY + cockpitHeight) {
                    float cockpitProgress = (fy - cockpitY) / cockpitHeight;
                    float cockpitWidthAtY = cockpitWidth * sinf(cockpitProgress * 3.14159f);
                    if (fabsf(dx) < cockpitWidthAtY) {
                        // Inside cockpit - dark blue
                        pixelColor = (Color){ 20, 40, 80, 255 };
                    }
                }

                // Edge highlight (lighter line at the very edge)
                if (fabsf(fabsf(dx) - widthAtY) < 1.5f) {
                    pixelColor = color_lerp(pixelColor, WHITE, 0.3f);
                }

                ImageDrawPixel(&img, x, y, pixelColor);
            }
        }
    }

    // Step 3: Upload to GPU (converts Image to Texture2D)
    Texture2D texture = LoadTextureFromImage(img);

    // Step 4: Free the CPU memory (GPU has its own copy now)
    UnloadImage(img);

    return texture;
}

/**
 * generate_engine_glow_texture - Create engine flame effect
 *
 * DESIGN:
 * =======
 * A radial gradient that looks like engine exhaust:
 *     - White/yellow at center (hottest)
 *     - Orange in middle
 *     - Red at edge
 *     - Transparent beyond
 *
 * We use distance from center to determine color.
 */
Texture2D generate_engine_glow_texture(int width, int height) {
    Image img = GenImageColor(width, height, BLANK);

    float centerX = width / 2.0f;
    float centerY = height * 0.2f;  // Glow originates near top
    float maxRadius = height * 0.8f;

    // Color stops for the gradient
    Color hot = { 255, 255, 255, 255 };      // White (center)
    Color warm = { 255, 200, 50, 255 };      // Yellow
    Color cool = { 255, 100, 20, 200 };      // Orange
    Color cold = { 255, 50, 10, 100 };       // Red
    Color edge = { 255, 20, 5, 0 };          // Transparent red

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float dx = x - centerX;
            float dy = y - centerY;

            // Elongate vertically (oval shape)
            float dist = sqrtf(dx * dx + (dy * 0.5f) * (dy * 0.5f));
            float t = dist / maxRadius;

            if (t > 1.0f) continue;  // Outside glow radius

            // Multi-stop gradient
            Color pixelColor;
            if (t < 0.1f) {
                pixelColor = color_lerp(hot, warm, t / 0.1f);
            } else if (t < 0.3f) {
                pixelColor = color_lerp(warm, cool, (t - 0.1f) / 0.2f);
            } else if (t < 0.6f) {
                pixelColor = color_lerp(cool, cold, (t - 0.3f) / 0.3f);
            } else {
                pixelColor = color_lerp(cold, edge, (t - 0.6f) / 0.4f);
            }

            // Add some noise for flame effect
            int noise = (rand() % 30) - 15;
            pixelColor.r = clamp_byte(pixelColor.r + noise);
            pixelColor.g = clamp_byte(pixelColor.g + noise / 2);

            ImageDrawPixel(&img, x, y, pixelColor);
        }
    }

    Texture2D texture = LoadTextureFromImage(img);
    UnloadImage(img);
    return texture;
}

/**
 * generate_bullet_texture - Create a projectile sprite
 *
 * DESIGN:
 * =======
 * A glowing orb with a slight trail:
 *     ○ ← Bright center
 *    ◐  ← Glow around it
 *   ─── ← Fading trail behind
 */
Texture2D generate_bullet_texture(int width, int height, Color color) {
    Image img = GenImageColor(width, height, BLANK);

    float centerX = width / 2.0f;
    float centerY = height * 0.3f;  // Bullet center toward front
    float radius = width * 0.25f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float dx = x - centerX;
            float dy = y - centerY;
            float dist = sqrtf(dx * dx + dy * dy);

            Color pixelColor = BLANK;

            if (dist < radius) {
                // Core of bullet - bright
                float t = dist / radius;
                pixelColor = color_lerp(WHITE, color, t);
                pixelColor.a = 255;
            } else if (dist < radius * 2) {
                // Outer glow
                float t = (dist - radius) / radius;
                pixelColor = color;
                pixelColor.a = (unsigned char)(200 * (1.0f - t));
            }

            // Trail effect (below the bullet)
            if (y > centerY + radius && x > centerX - 3 && x < centerX + 3) {
                float trailProgress = (y - centerY - radius) / (height - centerY - radius);
                if (trailProgress < 1.0f) {
                    Color trailColor = color;
                    trailColor.a = (unsigned char)(150 * (1.0f - trailProgress));
                    // Only draw trail if not already drawn bullet
                    if (pixelColor.a < trailColor.a) {
                        pixelColor = trailColor;
                    }
                }
            }

            if (pixelColor.a > 0) {
                ImageDrawPixel(&img, x, y, pixelColor);
            }
        }
    }

    Texture2D texture = LoadTextureFromImage(img);
    UnloadImage(img);
    return texture;
}

/**
 * generate_star_field_texture - Create background stars
 *
 * DESIGN:
 * =======
 * Random white pixels of varying brightness scattered across the image.
 * Some stars are larger (2x2 pixels) for depth.
 */
Texture2D generate_star_field_texture(int width, int height, int star_count) {
    // Dark background
    Image img = GenImageColor(width, height, (Color){ 5, 5, 15, 255 });

    for (int i = 0; i < star_count; i++) {
        int x = rand() % width;
        int y = rand() % height;

        // Random brightness
        unsigned char brightness = (unsigned char)(100 + rand() % 155);
        Color star_color = { brightness, brightness, brightness, 255 };

        // Slight color variation (some stars are slightly blue/yellow)
        int tint = rand() % 3;
        if (tint == 0) {
            star_color.b = clamp_byte(star_color.b + 30);  // Blue tint
        } else if (tint == 1) {
            star_color.r = clamp_byte(star_color.r + 20);  // Yellow tint
            star_color.g = clamp_byte(star_color.g + 10);
        }

        ImageDrawPixel(&img, x, y, star_color);

        // Some stars are larger (brighter = bigger)
        if (brightness > 200 && x < width - 1 && y < height - 1) {
            Color dimmer = star_color;
            dimmer.a = brightness / 2;
            ImageDrawPixel(&img, x + 1, y, dimmer);
            ImageDrawPixel(&img, x, y + 1, dimmer);
        }
    }

    Texture2D texture = LoadTextureFromImage(img);
    UnloadImage(img);
    return texture;
}
