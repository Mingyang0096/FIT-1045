#include "splashkit.h"
#include <cstdio>

#define MAX_ROCKS 100
#define PLAYER_SIZE 20
#define PLAYER_SPEED 3

// Structure to store data for a single rock
struct rock_data
{
    double x, y;     // Position of the rock
    double size;     // Radius of the rock
    double speed;    // Falling speed
};

// Structure to store all game state data
struct game_data
{
    rock_data rocks[MAX_ROCKS]; // Array of rocks
    int rock_count;             // Current number of rocks
    double player_x, player_y;  // Player position
    int score;                  // Player score
    int lives;                  // Remaining lives
    unsigned int next_rock_time;// Time for next rock spawn
};

/**
 * Initializes the game state before starting the main loop.
 * Sets player position, score, lives, and schedules the first rock spawn.
 * Resets rock count to zero so no rocks are present at the start.
 */
void init_game(game_data &game)
{
    game.rock_count = 0;
    game.score = 0;
    game.lives = 3;
    game.player_x = screen_width() / 2;
    game.player_y = screen_height() - 50;
    game.next_rock_time = current_ticks() + rnd(1000, 6000);
}

/**
 * Creates and adds a new rock to the game if capacity allows.
 * Randomizes size, position, and falling speed, then increments score.
 * Updates the next rock spawn time to a random interval in the future.
 */
void add_rock(game_data &game)
{
    if (game.rock_count >= MAX_ROCKS) return;

    rock_data r;
    r.size = rnd(20, 200);
    r.x = rnd(0, screen_width());
    r.y = -r.size;
    r.speed = rnd(1, 5);

    game.rocks[game.rock_count] = r;
    game.rock_count++;
    game.score += 1;
    game.next_rock_time = current_ticks() + rnd(1000, 6000);
}

/**
 * Updates the vertical position of all rocks each frame.
 * Removes rocks that move off-screen and awards points based on their size.
 * Uses O(1) removal by replacing removed rock with the last rock in the array.
 */
void update_rocks(game_data &game)
{
    for (int i = 0; i < game.rock_count; )
    {
        game.rocks[i].y += game.rocks[i].speed;

        if (game.rocks[i].y - game.rocks[i].size > screen_height())
        {
            game.score += (int)game.rocks[i].size;
            game.rocks[i] = game.rocks[game.rock_count - 1];
            game.rock_count--;
        }
        else
        {
            i++;
        }
    }
}

/**
 * Checks for collisions between the player and any rock.
 * If a collision occurs, removes the rock and decreases player lives by one.
 * Uses circle-based collision detection for simplicity and efficiency.
 */
void check_collisions(game_data &game)
{
    circle player_circle = circle_at(point_2d{game.player_x, game.player_y}, PLAYER_SIZE);

    for (int i = 0; i < game.rock_count; )
    {
        circle rock_circle = circle_at(point_2d{game.rocks[i].x, game.rocks[i].y}, game.rocks[i].size);
        if (circles_intersect(player_circle, rock_circle))
        {
            game.lives--;
            game.rocks[i] = game.rocks[game.rock_count - 1];
            game.rock_count--;
        }
        else
        {
            i++;
        }
    }
}

/**
 * Reads player input and moves the player left or right.
 * Ensures the player remains within the horizontal bounds of the screen.
 * Movement speed is fixed at PLAYER_SPEED pixels per frame.
 */
void handle_input(game_data &game)
{
    if (key_down(LEFT_KEY))  game.player_x -= PLAYER_SPEED;
    if (key_down(RIGHT_KEY)) game.player_x += PLAYER_SPEED;

    if (game.player_x < PLAYER_SIZE) game.player_x = PLAYER_SIZE;
    if (game.player_x > screen_width() - PLAYER_SIZE) game.player_x = screen_width() - PLAYER_SIZE;
}

/**
 * Draws all game elements to the screen each frame.
 * Displays score and lives, then renders the player and all rocks.
 * Clears the screen before drawing and refreshes it after rendering.
 */
void draw_game(const game_data &game)
{
    clear_screen(COLOR_BLACK);

    draw_text("Score: " + std::to_string(game.score), COLOR_WHITE, 10, 10);
    draw_text("Lives: " + std::to_string(game.lives), COLOR_WHITE, 10, 30);

    fill_circle(COLOR_BLUE, game.player_x, game.player_y, PLAYER_SIZE);

    for (int i = 0; i < game.rock_count; i++)
    {
        fill_circle(COLOR_GRAY, game.rocks[i].x, game.rocks[i].y, game.rocks[i].size);
    }

    refresh_screen();
}

int main()
{
    open_window("Rock Dodge Game", 800, 600);
    game_data game;
    init_game(game);

    while (!quit_requested() && game.lives > 0)
    {
        process_events();
        handle_input(game);

        if (current_ticks() >= game.next_rock_time)
        {
            add_rock(game);
        }

        update_rocks(game);
        check_collisions(game);
        draw_game(game);

        delay(1000 / 60); // Maintain 60 FPS
    }

    close_window("Rock Dodge Game");
    return 0;
}
