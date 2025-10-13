#include "splashkit.h"
#include "utilities.h"

// Reads the number of targets from the player.
// The input value is stored in loop and determines how many targets must be hit.
void input_target_count(int &loop)
{
    loop = read_integer("Input your target: ");
}

// Generates a new target position and resets related variables.
// - Randomizes the target's x and y coordinates within the window
// - Resets the radius r to the initial value init_r
// - Resets the current_time counter to 0
// - Sets need_new_target to false until the current target is resolved
void spawn_target(double &x, double &y, double &r, double init_r, int &current_time, bool &need_new_target)
{
    x = rnd(700);
    y = rnd(500);
    r = init_r;
    current_time = 0;
    need_new_target = false;
}

// Draws the game information on the screen.
// Displays the number of remaining targets and the fastest reaction time so far.
void draw_info(int loop, int best_time)
{
    draw_text("Target left: " + std::to_string(loop), COLOR_BLACK, 50, 50);
    draw_text("Fastest reaction time(ms): " + std::to_string(best_time), COLOR_BLACK, 50, 80);
}

// Handles the logic when the player clicks the mouse.
// - If the click is inside the target radius: decrease remaining targets,
//   update best_time if faster, and mark need_new_target as true
// - If the click misses: increase the target radius as a penalty
void handle_mouse_click(double mx, double my, double &x, double &y, double &r,
                        int &loop, int &best_time, int current_time, bool &need_new_target)
{
    double distance = (x - mx) * (x - mx) + (y - my) * (y - my);

    if (distance <= r * r) // Target hit
    {
        loop--;
        if (best_time == -1 || current_time < best_time)
        {
            best_time = current_time;
        }
        need_new_target = true;
    }
    else // Target missed
    {
        r += 5;
    }
}

// Applies a penalty if the player takes too long to click the target.
// If current_time is greater than or equal to 1000 ms, the target radius increases.
void timeout_penalty(int current_time, double &r)
{
    if (current_time >= 1000)
    {
        r += 5;
    }
}

// Displays the game over screen.
// - If best_time is valid, shows the fastest reaction time
// - Otherwise, shows a message that no targets were hit
// The screen remains for 3 seconds before closing
void game_over_screen(int best_time)
{
    clear_screen(COLOR_WHITE);
    if (best_time != -1)
    {
        draw_text("Game Over! Best reaction time: " + std::to_string(best_time) + " ms",
                  COLOR_BLACK, 200, 300);
    }
    else
    {
        draw_text("Game Over! No successful hits.", COLOR_BLACK, 200, 300);
    }
    refresh_screen();
    delay(3000);
}

// Main function of the game.
// - Reads the number of targets
// - Opens the game window
// - Runs the main loop until all targets are hit or quit is requested
// - Handles spawning, drawing, mouse clicks, penalties, and timing
// - Ends with the game over screen
int main()
{
    int loop;                  // Number of targets left
    double x = 0, y = 0;       // Target coordinates
    double r = 20, init_r = 20;// Target radius and initial radius
    double mx, my;             // Mouse click coordinates
    bool need_new_target = true; // Flag to control target spawning
    int best_time = -1;        // Best reaction time (-1 means no hits yet)
    int current_time = 0;      // Time counter for current target

    input_target_count(loop);
    open_window("Reaction Game", 800, 600);

    while (loop > 0 && !quit_requested())
    {
        process_events();
        clear_screen(COLOR_WHITE);

        draw_info(loop, best_time);

        if (need_new_target)
        {
            spawn_target(x, y, r, init_r, current_time, need_new_target);
        }

        fill_circle(COLOR_RED, x, y, r);

        if (mouse_clicked(LEFT_BUTTON))
        {
            mx = mouse_x();
            my = mouse_y();
            handle_mouse_click(mx, my, x, y, r, loop, best_time, current_time, need_new_target);
        }

        timeout_penalty(current_time, r);

        refresh_screen(50);
        current_time += 50;
    }

    game_over_screen(best_time);

    return 0;
}

