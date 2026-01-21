# Module 3: Architecture via Function Pointers

```
┌─────────────────────────────────────────────────────────────────┐
│  "In JavaScript, callbacks are first-class citizens.           │
│   In C, function pointers are callbacks on hard mode."         │
└─────────────────────────────────────────────────────────────────┘
```

## Learning Goals

By the end of this module, you will understand:
1. Function pointers - storing "addresses of code"
2. The Strategy Pattern - swapping algorithms at runtime
3. Forward declarations - solving circular dependencies
4. How to build an extensible weapon system

---

## Concept 1: Function Pointers

### The JavaScript Mental Model

In JS, functions are values you can store and pass around:

```javascript
const shootSpread = (position) => { /* spawn 3 bullets */ };
const shootRapid = (position) => { /* spawn 1 bullet fast */ };

let currentFireFunc = shootSpread;  // Store a function

// Later, call it
currentFireFunc({ x: 100, y: 200 });

// Or swap it
currentFireFunc = shootRapid;
```

### The C Equivalent: Function Pointers

In C, functions live in memory at specific addresses. We can store those addresses!

```c
// Function declarations
void shoot_spread(Vector2 position);
void shoot_rapid(Vector2 position);

// Declare a VARIABLE that can hold a FUNCTION ADDRESS
// The type is: "pointer to function taking Vector2 and returning void"
void (*fire_func)(Vector2 position);

// Store the address of shoot_spread
fire_func = shoot_spread;  // Note: no () - we want the address, not to call it

// Call it
fire_func((Vector2){100, 200});

// Swap it
fire_func = shoot_rapid;
fire_func((Vector2){100, 200});  // Now calls shoot_rapid
```

### Breaking Down the Syntax

```c
void (*fire_func)(Vector2 position);
│     │    │       └── Parameter list
│     │    └── Variable name
│     └── * means "pointer to"
└── Return type

Read it as: "fire_func is a pointer to a function that takes Vector2 and returns void"
```

### Why Not Just Use `switch`?

```c
// The UGLY way (without function pointers)
void fire_weapon(int weapon_type, Vector2 pos) {
    switch (weapon_type) {
        case WEAPON_SPREAD: shoot_spread(pos); break;
        case WEAPON_RAPID:  shoot_rapid(pos);  break;
        case WEAPON_LASER:  shoot_laser(pos);  break;
        // Adding a new weapon? Edit this function!
        // 50 weapons? 50 case statements!
    }
}

// The CLEAN way (with function pointers)
typedef struct {
    void (*fire)(Vector2 position);
} Weapon;

void fire_weapon(Weapon* weapon, Vector2 pos) {
    weapon->fire(pos);  // Just call the pointer!
    // Adding a new weapon? Just create it with its own fire function!
}
```

---

## Concept 2: typedef for Function Pointers

The syntax `void (*name)(params)` is ugly and error-prone. Use `typedef`:

```c
// Without typedef (ugly)
void (*fire_func)(Vector2 position);
void (*fire_func_array[10])(Vector2 position);  // Even uglier!

// With typedef (clean)
typedef void (*FireFunc)(Vector2 position);

FireFunc fire_func;         // Much cleaner!
FireFunc fire_funcs[10];    // Array of function pointers
```

### Common Patterns

```c
// Update functions (called every frame)
typedef void (*UpdateFunc)(void* entity, float dt);

// Draw functions (render an entity)
typedef void (*DrawFunc)(const void* entity);

// Event handlers (respond to events)
typedef void (*EventHandler)(int event_type, void* data);

// Comparators (for sorting)
typedef int (*Comparator)(const void* a, const void* b);
```

---

## Concept 3: The Strategy Pattern

### The Pattern

```
┌─────────────────────────────────────────────────────────────────┐
│                     STRATEGY PATTERN                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   CONTEXT (Player)               STRATEGY INTERFACE (FireFunc) │
│   ┌─────────────────┐            ┌──────────────────┐          │
│   │ Weapon weapon;  │───────────▶│ void (*fire)()   │          │
│   └─────────────────┘            └──────────────────┘          │
│                                           ▲                     │
│                                           │ implements          │
│                               ┌───────────┴───────────┐        │
│                               │                       │        │
│                    ┌──────────────────┐    ┌──────────────────┐│
│                    │ shoot_spread()   │    │ shoot_rapid()    ││
│                    │ CONCRETE STRATEGY│    │ CONCRETE STRATEGY││
│                    └──────────────────┘    └──────────────────┘│
│                                                                 │
│   At runtime, we can swap which strategy the player uses!      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### In Our Game: The Weapon System

```c
// The Strategy "interface" - defines what a weapon can do
typedef void (*FireFunc)(Vector2 position, BulletList* bullets);
typedef void (*SpecialFunc)(Vector2 position, BulletList* bullets);

// The Weapon struct embeds the function pointers
typedef struct {
    const char* name;           // "Spread Shot", "Rapid Fire", etc.
    FireFunc fire;              // Primary fire function
    SpecialFunc special;        // Secondary/special ability
    float fire_rate;            // Shots per second
    float cooldown;             // Current cooldown timer
    int ammo;                   // -1 for infinite
} Weapon;

// Concrete strategies (the actual implementations)
void spread_fire(Vector2 pos, BulletList* bullets) {
    // Spawn 3 bullets in a spread pattern
    spawn_bullet(bullets, pos, -15.0f);  // Left
    spawn_bullet(bullets, pos,   0.0f);  // Center
    spawn_bullet(bullets, pos,  15.0f);  // Right
}

void rapid_fire(Vector2 pos, BulletList* bullets) {
    // Spawn 1 bullet straight up
    spawn_bullet(bullets, pos, 0.0f);
}

// Create weapon instances
Weapon weapon_spread = {
    .name = "Spread Shot",
    .fire = spread_fire,
    .fire_rate = 2.0f,
    // ...
};

Weapon weapon_rapid = {
    .name = "Rapid Fire",
    .fire = rapid_fire,
    .fire_rate = 10.0f,
    // ...
};
```

---

## Concept 4: Forward Declarations

### The Problem: Circular Dependencies

```c
// weapon.h
#include "bullet.h"  // Weapon needs BulletList

// bullet.h
#include "weapon.h"  // Bullet needs Weapon (maybe for damage type?)

// COMPILE ERROR: Each file includes the other!
```

### The Solution: Forward Declarations

```c
// weapon.h
#ifndef WEAPON_H
#define WEAPON_H

// Forward declare BulletList - we don't need its full definition here
// We only use BulletList* (pointer), not BulletList directly
typedef struct BulletList BulletList;

typedef void (*FireFunc)(Vector2 position, BulletList* bullets);

// Now we can use BulletList* without including bullet.h
typedef struct {
    FireFunc fire;
    // ...
} Weapon;

#endif

// weapon.c
#include "weapon.h"
#include "bullet.h"  // NOW we include it, for the actual implementation
```

### Why This Works

```
┌─────────────────────────────────────────────────────────────────┐
│  Forward declaration tells the compiler:                        │
│  "BulletList is a struct that EXISTS. I'll show you later."    │
│                                                                 │
│  This is enough for the compiler to understand:                 │
│      BulletList* ptr;  // "Pointer to some struct called        │
│                        //  BulletList. Size of pointer is       │
│                        //  8 bytes. That's all I need!"         │
│                                                                 │
│  But NOT enough for:                                            │
│      BulletList list;  // "I need to know the SIZE of           │
│                        //  BulletList to allocate stack space.  │
│                        //  ERROR: incomplete type!"             │
└─────────────────────────────────────────────────────────────────┘
```

---

## The Deliverable

A graphical demo where:
1. Player can switch weapons with number keys (1, 2, 3)
2. Each weapon has different firing behavior
3. Bullets are spawned via function pointers
4. Visual feedback shows current weapon

```
┌─────────────────────────────────────────────────────────────────┐
│  Weapon: [SPREAD SHOT]    Ammo: ∞                              │
│                                                                 │
│                    •   •   •  ← Spread bullets                  │
│                     \  |  /                                     │
│                      \ | /                                      │
│                       \|/                                       │
│                        ▲   ← Player ship                        │
│                                                                 │
│  [1] Spread  [2] Rapid  [3] Laser     SPACE to fire            │
└─────────────────────────────────────────────────────────────────┘
```

---

## File Structure

```
module3_polymorphism/
├── main.c           # Entry point, game loop
├── player.h/c       # Player with equipped weapon
├── weapon.h/c       # Weapon struct and fire functions
├── bullet.h/c       # Bullet struct and list
├── textures.h/c     # Procedural texture generation
└── Makefile
```

Now let's look at the code!
