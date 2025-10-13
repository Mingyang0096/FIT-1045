#include "splashkit.h"

/**
 * Prompts the user to enter a target score and validates the input.
 * Ensures the input is a positive integer; continues prompting if input is invalid.
 * 
 * @return A valid target score (positive integer)
 */
int get_target_score()
{
    int target;
    while (true)
    {
        write_line("What is your target score:");
        string tar = read_line();
        if (!is_integer(tar))
        {
            write_line("Please enter a whole number");
        }
        else
        {
            target = convert_to_integer(tar);
            if (target > 0) break;
            else write_line("Please enter a positive number");
        }
    }
    return target;
}

/**
 * Draws a health bar to visualize the ratio of remaining targets to total targets.
 * Consists of a gray background and a green foreground whose width adjusts dynamically 
 * based on the proportion of remaining targets.
 * 
 * @param remaining The number of remaining targets
 * @param target    The total number of targets
 */
void draw_health_bar(int remaining, int target)
{
    fill_rectangle(COLOR_GRAY, 50, 20, 200, 20);
    float bar_width = 200 * (remaining / (float)target);
    fill_rectangle(COLOR_GREEN, 50, 20, bar_width, 20);
}

/**
 * Draws a red circular target at the specified position.
 * 
 * @param x      X-coordinate of the target's center
 * @param y      Y-coordinate of the target's center
 * @param radius Radius of the target circle
 */
void draw_target(float x, float y, float radius)
{
    fill_circle(COLOR_RED, x, y, radius);
}

/**
 * Checks if a mouse click position hits the target circle.
 * Determines hit status by comparing the distance between mouse coordinates and 
 * target center with the target radius.
 * 
 * @param mx     X-coordinate of the mouse click
 * @param my     Y-coordinate of the mouse click
 * @param tx     X-coordinate of the target's center
 * @param ty     Y-coordinate of the target's center
 * @param radius Radius of the target circle
 * @return       true if the target is hit, false otherwise
 */
bool is_target_hit(float mx, float my, float tx, float ty, float radius)
{
    float dx = mx - tx;
    float dy = my - ty;
    return (dx * dx + dy * dy <= radius * radius);
}

/**
 * Generates a new random position for the target, ensuring it stays fully within the window.
 * The target position avoids window edges to prevent clipping.
 * 
 * @param x          Reference to store the new target's X-coordinate
 * @param y          Reference to store the new target's Y-coordinate
 * @param radius     Radius of the target circle
 * @param win_width  Width of the game window
 * @param win_height Height of the game window
 */
void generate_new_target(float &x, float &y, float radius, int win_width, int win_height)
{
    x = rnd(radius, win_width - radius);
    y = rnd(radius + 50, win_height - radius);
}

/**
 * Main function of the program, responsible for initializing the game, handling the game loop,
 * and processing user input. Implements logic for reducing remaining targets on click,
 * checking win conditions, and handling game termination.
 * 
 * @return Program exit status code (0 for normal exit)
 */
int main()
{
    // Initialize game window dimensions and target properties
    int win_width = 800, win_height = 600;  // Window size in pixels
    float radius = 40;                       // Radius of the target circle

    // Get valid target score from user and initialize remaining targets counter
    int target = get_target_score();         // Total targets needed to win
    int remaining = target;                  // Tracks remaining targets to click

    // Create and open the game window with specified title and dimensions
    open_window("Click Game", win_width, win_height);

    // Generate initial random position for the target
    float target_x, target_y;                // Coordinates for target's center
    generate_new_target(target_x, target_y, radius, win_width, win_height);

    // Main game loop - continues until user requests to quit
    while (!quit_requested())
    {
        process_events();                    // Handle user input (mouse/keyboard events)
        clear_screen(COLOR_WHITE);           // Clear screen with white background

        // Draw game UI elements
        draw_health_bar(remaining, target);  // Render progress bar
        draw_text("Targets left: " + std::to_string(remaining), COLOR_BLACK, 300, 20);  // Display remaining count
        draw_target(target_x, target_y, radius);  // Render current target

        // Check for left mouse button clicks
        if (mouse_clicked(LEFT_BUTTON))
        {
            // Verify if click position intersects with target
            if (is_target_hit(mouse_x(), mouse_y(), target_x, target_y, radius))
            {
                remaining--;  // Decrement remaining targets counter

                // Check win condition (no remaining targets)
                if (remaining <= 0)
                {
                    clear_screen(COLOR_WHITE);  // Clear screen for victory message
                    draw_text("You win this game!!!", COLOR_RED, 300, 200);  // Display victory text
                    refresh_screen();           // Update display to show message
                    delay(3000);                // Pause for 3 seconds to let user see message
                    break;                      // Exit game loop (end game)
                }

                // Generate new target position after successful hit
                generate_new_target(target_x, target_y, radius, win_width, win_height);
            }
        }

        refresh_screen();  // Update display with all drawn elements
    }

    return 0;  // Indicate successful program termination
}
