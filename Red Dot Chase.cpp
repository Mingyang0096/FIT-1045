#include "splashkit.h"

/**
 * Structure to encapsulate game-related data, including window properties,
 * circle attributes, and menu state.
 */
typedef struct game_data {
    int window_width;   /**< Width of the game window in pixels. */
    int window_height;  /**< Height of the game window in pixels. */
    float circle_radius;/**< Radius of the red interactive circle. */
    bool menu_active;   /**< Flag to indicate if the menu is currently active. */
} game_data;

/**
 * Displays a detailed screen with a small orange-red circle at the specified coordinates.
 * Exits this screen and returns to the main flow when the SPACE key is pressed.
 * 
 * @param x The x-coordinate for the center of the small orange-red circle.
 * @param y The y-coordinate for the center of the small orange-red circle.
 * @return Always returns `1` to signal successful completion (used for control flow).
 */
int details(float x, float y) {
    while (true) {
        clear_screen(color_light_blue());
        fill_circle(color_orange_red(), x, y, 2);
        refresh_screen();
        if (key_typed(SPACE_KEY)) {
            return 1;
        } else {
            continue;
        }
    }
}

/**
 * Manages the game's menu screen (displayed with a light green background).
 * - Pressing SPACE exits the menu and returns to the main game (`return 1`).
 * - Pressing 'Q' quits the program entirely (`return 0`).
 * 
 * @return `1` to exit the menu and resume the game; `0` to quit the program.
 */
int menu() {
    while (true) {
        clear_screen(color_light_green());
        refresh_screen();
        if (key_typed(SPACE_KEY)) {
            return 1;
        } else if (key_typed(Q_KEY)) {
            return 0;
        } else {
            continue;
        }
    }
}

/**
 * Main game function: initializes game data, opens a window, and runs the main game loop.
 * The loop handles:
 * - Rendering a random red circle.
 * - Responding to key presses (SPACE to reposition the circle, KEYPAD_1 for the "details" screen, KEYPAD_2 for the menu).
 * 
 * @return `0` to indicate successful program termination.
 */
int main() {
    // Initialize game data with window dimensions, circle radius, and menu state
    game_data game;
    game.window_width = 800;
    game.window_height = 600;
    game.circle_radius = 15;
    game.menu_active = 1; // Start with the menu/logic active

    open_window("game", game.window_width, game.window_height);
    clear_screen(color_white());

    float x, y; // Coordinates for the center of the red interactive circle

    while (game.menu_active) {
        process_events();
        // Generate random coordinates for the red circle within the window
        x = rnd(game.window_width);
        y = rnd(game.window_height);
        fill_circle(color_red(), x, y, game.circle_radius);

        if (key_typed(SPACE_KEY)) {
            clear_screen(color_white());
            x = rnd(game.window_width);
            y = rnd(game.window_height);
            fill_circle(color_red(), x, y, game.circle_radius);
            refresh_screen();
        } else if (key_typed(KEYPAD_1)) {
            int unused_return = details(x, y); // Call "details" screen (return value unused)
        } else if (key_typed(KEYPAD_2)) {
            game.menu_active = menu(); // Update menu state based on menu screen's return
        }
        delay(100); // Small delay to control loop speed
    }

    return 0;
}