/**
 * textures.h - Procedural Texture Generation
 *
 * CONCEPT: Procedural Content Generation (PCG)
 * =============================================
 * Instead of loading images from files, we generate them mathematically.
 *
 * Benefits:
 *     - No external dependencies
 *     - Infinite variations possible
 *     - Smaller distribution size
 *     - Educational: understand how graphics work
 *
 * Used in many games:
 *     - Minecraft's terrain
 *     - No Man's Sky's planets
 *     - Roguelikes' dungeons
 */

#ifndef TEXTURES_H
#define TEXTURES_H

#include "raylib.h"

/**
 * generate_ship_texture - Create a spaceship sprite procedurally
 *
 * PROCESS:
 * ========
 * 1. Create a blank Image (in CPU RAM)
 * 2. Draw pixels to form a ship shape
 * 3. Convert Image to Texture2D (upload to GPU VRAM)
 * 4. The caller must call UnloadTexture() when done
 *
 * The ship design:
 *     - Triangular body with gradient shading
 *     - Cockpit window (darker center)
 *     - Wing details
 *
 * @param width   Width of the texture in pixels
 * @param height  Height of the texture in pixels
 * @param color   Base color for the ship
 * @return        Texture2D ready to draw (caller must unload)
 */
Texture2D generate_ship_texture(int width, int height, Color color);

/**
 * generate_engine_glow_texture - Create an engine flame/glow effect
 *
 * PROCESS:
 * ========
 * Creates a radial gradient that fades from hot (white/yellow center)
 * to cool (orange/red edge) to transparent.
 *
 * This will be drawn behind the ship when moving.
 *
 * @param width   Width of the texture
 * @param height  Height of the texture
 * @return        Texture2D with the glow effect
 */
Texture2D generate_engine_glow_texture(int width, int height);

/**
 * generate_bullet_texture - Create a projectile sprite
 *
 * PROCESS:
 * ========
 * Creates a small glowing orb with a trail effect.
 *
 * @param width   Width of the texture
 * @param height  Height of the texture
 * @param color   Color of the bullet
 * @return        Texture2D for the bullet
 */
Texture2D generate_bullet_texture(int width, int height, Color color);

/**
 * generate_star_field_texture - Create a background star field
 *
 * PROCESS:
 * ========
 * Randomly places "stars" (white pixels of varying brightness)
 * across the texture. Can be tiled for parallax scrolling.
 *
 * @param width       Width of the texture
 * @param height      Height of the texture
 * @param star_count  Number of stars to generate
 * @return            Texture2D with the star field
 */
Texture2D generate_star_field_texture(int width, int height, int star_count);

/**
 * UTILITY: Color manipulation helpers
 *
 * CONCEPT: Color Math
 * ===================
 * Colors in Raylib are RGBA (Red, Green, Blue, Alpha).
 * Each component is 0-255 (unsigned char).
 *
 * To create effects:
 *     - Fade: Multiply RGB or reduce Alpha
 *     - Brighten: Add to RGB (clamp at 255)
 *     - Blend: Mix two colors based on a ratio
 */

/**
 * color_fade - Reduce alpha of a color
 *
 * @param color  The color to fade
 * @param alpha  New alpha value (0-255)
 * @return       Color with new alpha
 */
Color color_fade(Color color, unsigned char alpha);

/**
 * color_lerp - Linear interpolate between two colors
 *
 * CONCEPT: Linear Interpolation (LERP)
 * =====================================
 * lerp(a, b, t) = a + (b - a) * t
 *
 * When t = 0: result = a
 * When t = 1: result = b
 * When t = 0.5: result = midpoint
 *
 * This is fundamental to all smooth animations and gradients.
 *
 * @param c1  Start color
 * @param c2  End color
 * @param t   Interpolation factor (0.0 to 1.0)
 * @return    Blended color
 */
Color color_lerp(Color c1, Color c2, float t);

#endif // TEXTURES_H
