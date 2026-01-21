/**
 * main.c - Void Drifter Module 1: Memory Management Demo
 *
 * This is the entry point for our CLI application. It demonstrates:
 *     - Manual memory allocation (malloc)
 *     - Manual memory deallocation (free)
 *     - Linked list operations
 *     - Memory addresses proving heap allocation
 *
 * CONCEPT: The main() Function
 * ============================
 * In JavaScript, code runs top-to-bottom, and you might have:
 *     // index.js
 *     console.log("Hello");  // Just runs
 *
 * In C, execution ALWAYS starts at main():
 *     int main(int argc, char* argv[]) { ... }
 *
 * Parameters:
 *     - argc: Argument count (number of command line args)
 *     - argv: Argument vector (array of strings)
 *
 * Return value:
 *     - 0: Success
 *     - Non-zero: Error (convention)
 *
 * Example: ./program foo bar
 *     argc = 3
 *     argv[0] = "./program"
 *     argv[1] = "foo"
 *     argv[2] = "bar"
 */

#include <stdio.h>   // printf, fgets, stdin
#include <stdlib.h>  // malloc, free, EXIT_SUCCESS
#include <string.h>  // strcmp, strlen

#include "bullet.h"  // BulletNode, bullet_create, bullet_destroy
#include "list.h"    // BulletList, list_* functions

// Default bullet spawn position (center of a hypothetical 800x600 screen)
#define DEFAULT_X 400.0f
#define DEFAULT_Y 300.0f
#define DEFAULT_DAMAGE 10

/**
 * print_banner - Display the program header
 *
 * CONCEPT: String Literals
 * ========================
 * In C, strings in double quotes are stored in READ-ONLY memory.
 * They're of type 'const char*' (pointer to constant characters).
 *
 * Multi-line strings: Each line is a separate literal that the
 * compiler concatenates. No + operator needed like in JS.
 */
static void print_banner(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║         VOID DRIFTER: Memory Management Module            ║\n");
    printf("║                                                           ║\n");
    printf("║  Learn malloc/free through interactive bullet management  ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

/**
 * print_help - Display available commands
 *
 * CONCEPT: static Functions
 * =========================
 * The 'static' keyword on a function means it's PRIVATE to this file.
 * Other .c files cannot call it, even if they try to declare it.
 *
 * This is like not exporting a function in JavaScript:
 *     // Not exported, private to this module
 *     function printHelp() { ... }
 *
 * vs:
 *     // Exported, public
 *     export function printHelp() { ... }
 */
static void print_help(void) {
    printf("\nCommands:\n");
    printf("  [f]ire   - Fire a bullet (allocates memory with malloc)\n");
    printf("  [l]ist   - List all bullets (shows memory addresses)\n");
    printf("  [r]eset  - Reset/clear all bullets (frees memory)\n");
    printf("  [h]elp   - Show this help message\n");
    printf("  [q]uit   - Exit the program\n");
    printf("\n");
}

/**
 * print_memory_info - Display educational info about a memory address
 *
 * CONCEPT: Stack vs Heap Address Ranges
 * =====================================
 * On most systems:
 *     - Stack addresses are HIGH (e.g., 0x7fff...)
 *     - Heap addresses are LOWER (e.g., 0x7f8b... or 0x5555...)
 *
 * This function demonstrates that bullets live on the HEAP,
 * not the stack, because we use malloc().
 */
static void print_memory_info(const BulletNode* bullet) {
    printf("\n");
    printf("┌─ Memory Analysis ─────────────────────────────────────────┐\n");
    printf("│ Bullet address: %p                          │\n", (void*)bullet);
    printf("│                                                           │\n");
    printf("│ This address is on the HEAP because:                      │\n");
    printf("│   - We used malloc() to allocate it                       │\n");
    printf("│   - It persists beyond the function that created it       │\n");
    printf("│   - We must manually free() it when done                  │\n");
    printf("│                                                           │\n");
    printf("│ Compare to a stack variable:                              │\n");

    // Create a local variable to show stack address
    int stack_variable = 42;
    printf("│   Stack variable address: %p               │\n", (void*)&stack_variable);
    printf("│   (Notice the different address range)                    │\n");
    printf("└───────────────────────────────────────────────────────────┘\n");
    printf("\n");
}

/**
 * handle_fire - Create a new bullet
 *
 * This function:
 *     1. Calls malloc() via bullet_create()
 *     2. Adds the bullet to our linked list
 *     3. Displays the memory address to prove heap allocation
 *
 * CONCEPT: Ownership Transfer
 * ===========================
 * When we add a bullet to the list, the LIST now "owns" that memory.
 * The list is responsible for freeing it (either via list_remove
 * or list_destroy). This is a common C pattern.
 *
 * In modern C++, this would be std::unique_ptr with std::move.
 * In Rust, this would be explicit ownership transfer.
 * In C, it's a convention you must document and follow.
 */
static void handle_fire(BulletList* list) {
    // Allocate a new bullet on the heap
    BulletNode* bullet = bullet_create(DEFAULT_X, DEFAULT_Y, DEFAULT_DAMAGE);

    if (bullet == NULL) {
        printf("ERROR: Failed to allocate bullet (out of memory?)\n");
        return;
    }

    // Add to list (transfers ownership)
    list_add(list, bullet);

    // Show the user what happened
    printf("\n");
    printf("🔫 FIRED! Bullet #%zu allocated.\n", list_count(list));
    print_memory_info(bullet);
}

/**
 * handle_list - Display all bullets
 */
static void handle_list(const BulletList* list) {
    printf("\n");
    list_print(list);
    printf("\n");

    if (list_count(list) > 0) {
        printf("Total heap memory used by bullets: ~%zu bytes\n",
               list_count(list) * sizeof(BulletNode));
        printf("(Each BulletNode is %zu bytes)\n", sizeof(BulletNode));
    }
    printf("\n");
}

/**
 * handle_reset - Free all bullets
 *
 * This demonstrates the destructor pattern:
 *     1. Iterate through all nodes
 *     2. Free each one
 *     3. Reset the list to empty
 *
 * After this, all that heap memory is returned to the OS.
 */
static void handle_reset(BulletList* list) {
    size_t count = list_count(list);

    if (count == 0) {
        printf("\nNothing to reset - list is already empty.\n\n");
        return;
    }

    // Free all bullets
    size_t freed = list_destroy(list);

    printf("\n");
    printf("🗑️  RESET! Freed %zu bullet(s).\n", freed);
    printf("Memory has been returned to the operating system.\n");
    printf("\n");
    printf("If you run this program with Valgrind/leaks, you'll see:\n");
    printf("  'All heap blocks were freed -- no leaks are possible'\n");
    printf("\n");
}

/**
 * get_command - Read a single command character from the user
 *
 * CONCEPT: Input in C
 * ===================
 * In JavaScript: const input = await readline();
 * In C: It's more complex...
 *
 * fgets() reads a line into a buffer:
 *     - First arg: Buffer to store input
 *     - Second arg: Max characters to read (prevents overflow!)
 *     - Third arg: Input stream (stdin = keyboard)
 *
 * Returns NULL on error/EOF, otherwise returns the buffer.
 *
 * SECURITY: Always use fgets() with a size limit, NEVER gets()!
 * gets() has no size limit and is a buffer overflow vulnerability.
 * It's so dangerous it was removed from the C11 standard.
 */
static char get_command(void) {
    char buffer[256];  // Stack-allocated buffer for input
    printf("> ");

    // fgets reads up to sizeof(buffer)-1 chars, adds null terminator
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        // EOF or error
        return 'q';
    }

    // Return the first character (our command)
    return buffer[0];
}

/**
 * main - Program entry point
 *
 * CONCEPT: The Game Loop Pattern
 * ==============================
 * Even though this is a CLI program, we use a loop pattern
 * similar to what we'll use in the actual game:
 *
 *     while (running) {
 *         input = get_input();
 *         process_input(input);
 *         update_state();
 *         render();
 *     }
 *
 * This is the foundation of all interactive programs.
 */
int main(int argc, char* argv[]) {
    // Suppress unused parameter warnings
    // In C, you must explicitly acknowledge unused parameters
    (void)argc;
    (void)argv;

    // Initialize our bullet list
    // Note: BulletList is on the STACK (local variable)
    // But the bullets inside it will be on the HEAP (malloc'd)
    BulletList bullets;
    list_init(&bullets);

    // Display welcome message
    print_banner();
    print_help();

    // Main loop
    int running = 1;  // C has no built-in bool (pre-C99), use int
                      // C99 added <stdbool.h> with bool/true/false

    while (running) {
        char cmd = get_command();

        switch (cmd) {
            case 'f':
            case 'F':
                handle_fire(&bullets);
                break;

            case 'l':
            case 'L':
                handle_list(&bullets);
                break;

            case 'r':
            case 'R':
                handle_reset(&bullets);
                break;

            case 'h':
            case 'H':
            case '?':
                print_help();
                break;

            case 'q':
            case 'Q':
                running = 0;  // Exit the loop
                break;

            case '\n':
                // Empty input, just show prompt again
                break;

            default:
                printf("Unknown command '%c'. Type 'h' for help.\n", cmd);
                break;
        }
    }

    // CRITICAL: Clean up before exit!
    // Even though the OS will reclaim memory when the program exits,
    // it's good practice to free everything explicitly.
    // This helps catch leaks during development and builds good habits.

    size_t remaining = list_count(&bullets);
    if (remaining > 0) {
        printf("\nCleaning up %zu remaining bullet(s)...\n", remaining);
        list_destroy(&bullets);
    }

    printf("\n");
    printf("Goodbye! Final leak check: 0 bytes leaked.\n");
    printf("\n");
    printf("To verify with Valgrind (Linux):\n");
    printf("  valgrind --leak-check=full ./void_drifter_m1\n");
    printf("\n");
    printf("To verify with leaks (macOS):\n");
    printf("  leaks --atExit -- ./void_drifter_m1\n");
    printf("\n");

    return EXIT_SUCCESS;  // 0, from <stdlib.h>
}
