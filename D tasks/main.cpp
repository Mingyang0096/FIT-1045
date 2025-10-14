// main.cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <cstdio>
#include <stdexcept>
#include <array>
#include <sstream>
#include <filesystem>
#include <iomanip>   // std::quoted
#include <future>
#include <chrono>
#include <algorithm>

#include "splashkit.h"
#include "nlohmann/json.hpp"

static const int WALL = 1;
static const int ROAD = 0;

// Load the top level two dimensional array from a json file
std::vector<std::vector<int>> load_maze(const std::string &path)
{
    std::ifstream in(path);
    if (!in.is_open()) throw std::runtime_error("cannot open: " + path);

    nlohmann::json nj;
    in >> nj;

    if (!nj.is_array() || nj.empty() || !nj[0].is_array())
        throw std::runtime_error("maze.json must be a 2D array");

    std::vector<std::vector<int>> g;
    g.reserve(nj.size());
    for (auto &row : nj)
    {
        std::vector<int> r;
        r.reserve(row.size());
        for (auto &v : row) r.push_back(v.get<int>());
        g.push_back(std::move(r));
    }
    return g;
}

// Find the single exit cell which must be the only ROAD on the border
std::pair<int,int> find_exit_cell(const std::vector<std::vector<int>> &g)
{
    int H = (int)g.size(), W = (int)g[0].size();
    std::vector<std::pair<int,int>> exits;
    // Top and bottom borders
    for (int c=0;c<W;++c){
        if (g[0][c]==ROAD) exits.emplace_back(0,c);
        if (g[H-1][c]==ROAD) exits.emplace_back(H-1,c);
    }
    // Left and right borders
    for (int r=0;r<H;++r){
        if (g[r][0]==ROAD) exits.emplace_back(r,0);
        if (g[r][W-1]==ROAD) exits.emplace_back(r,W-1);
    }
    if (exits.size()!=1) throw std::runtime_error("maze border must contain exactly one exit");
    return exits[0];
}

// Pick a random road cell from all ROAD cells
std::pair<int,int> random_road(const std::vector<std::vector<int>> &g, std::mt19937 &rng)
{
    int H = (int)g.size(), W = (int)g[0].size();
    std::vector<std::pair<int,int>> roads;
    roads.reserve(H*W);
    for (int r=0;r<H;++r)
        for (int c=0;c<W;++c)
            if (g[r][c]==ROAD) roads.emplace_back(r,c);
    if (roads.empty()) throw std::runtime_error("no road cell in maze");
    std::uniform_int_distribution<int> dist(0, (int)roads.size()-1);
    return roads[dist(rng)];
}

// Call the python script in a blocking way and parse returned json
nlohmann::json run_python_next_step(const std::string &maze_path, int from_r, int from_c, int to_r, int to_c)
{
    std::ostringstream cmd;
    cmd << "python generator.py next-step"
        << " --maze " << std::quoted(maze_path)
        << " --from " << from_r << ' ' << from_c
        << " --to "   << to_r   << ' ' << to_c;

#if defined(_WIN32)
    FILE *pipe = _popen(cmd.str().c_str(), "r");
#else
    FILE *pipe = popen(cmd.str().c_str(), "r");
#endif
    if (!pipe) throw std::runtime_error("failed to run python next-step");

    std::string output;
    std::array<char, 256> buf{};
    while (fgets(buf.data(), (int)buf.size(), pipe)) output += buf.data();

#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    if (output.empty()) throw std::runtime_error("python next-step returned empty output");
    return nlohmann::json::parse(output);
}

// Ask python to generate maze.json with given size and loop density
void run_python_generate_maze(const std::string &out_path, int H, int W, double loop_density = 0.08)
{
    std::ostringstream cmd;
    cmd << "python generator.py generate"
        << " --H " << H
        << " --W " << W
        << " --loop " << loop_density
        << " --out " << std::quoted(out_path);
#if defined(_WIN32)
    FILE *pipe = _popen(cmd.str().c_str(), "r");
#else
    FILE *pipe = popen(cmd.str().c_str(), "r");
#endif
    if (!pipe) throw std::runtime_error("failed to run python generate");
    char buf[256];
    while (fgets(buf, (int)sizeof(buf), pipe)) { /* You can print logs here such as puts with buf */ }
#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif
}

// Smooth mover that operates on grid cells with pixel interpolation for rendering
struct Mover {
    // Grid coordinates in cell units
    int r{0}, c{0};
    int tr{0}, tc{0};     // Target cell to commit to r and c after movement completes

    // Pixel coordinates for rendering
    float x{0}, y{0};
    float sx{0}, sy{0};   // Pixel start position
    float tx{0}, ty{0};   // Pixel target position

    // Progress from zero to one
    float t{0};
    bool moving{false};

    // Speed in pixels per second
    float speed{160.0f};  // Player default is one hundred sixty pixels per second. With TILE as thirty two this is five cells per second
};

static int TILE = 32;
static int PADDING = 0;

inline float cell_to_px_c(int c) { return (float)(c * TILE + PADDING); }
inline float cell_to_px_r(int r) { return (float)(r * TILE + PADDING); }

void start_move(Mover &m, int nr, int nc)
{
    m.tr = nr; m.tc = nc;
    m.sx = m.x; m.sy = m.y;
    m.tx = cell_to_px_c(nc);
    m.ty = cell_to_px_r(nr);
    m.t  = 0.0f;
    m.moving = true;
}

void snap_to_cell(Mover &m) // Immediately align pixel position to current r and c
{
    m.tr = m.r; m.tc = m.c;
    m.x = cell_to_px_c(m.c);
    m.y = cell_to_px_r(m.r);
    m.sx = m.x; m.sy = m.y;
    m.tx = m.x; m.ty = m.y;
    m.t = 0.0f;
    m.moving = false;
}

void update_mover(Mover &m, float dt)
{
    if (!m.moving) return;
    // Speed is in pixels per second. One cell takes TILE pixels. Therefore t increases by speed times dt divided by TILE
    m.t += (m.speed * dt) / (float)TILE;
    if (m.t >= 1.0f) {
        m.t = 1.0f;
        m.moving = false;
        m.r = m.tr; m.c = m.tc;
        m.x = m.tx; m.y = m.ty;
    } else {
        m.x = m.sx + (m.tx - m.sx) * m.t;
        m.y = m.sy + (m.ty - m.sy) * m.t;
    }
}

int main()
{
    try {
        std::cout << "cwd: " << std::filesystem::current_path().string() << "\n";
        const std::string maze_file = "maze.json";

        // Auto generate the map on first run if the file is missing or cannot be read
        if (!std::filesystem::exists(maze_file)) {
            // No file on first run. Generate default twenty by twenty
            run_python_generate_maze(maze_file, 20, 20, 0.08);
        }

        std::vector<std::vector<int>> maze;
        try {
            maze = load_maze(maze_file);
        } catch (...) {
            // File exists but is corrupted or has a wrong format. Regenerate and read again
            run_python_generate_maze(maze_file, 20, 20, 0.08);
            maze = load_maze(maze_file);
        }

        int H = (int)maze.size();
        int W = (int)maze[0].size();
        auto exit_cell = find_exit_cell(maze);
        int exit_r = exit_cell.first, exit_c = exit_cell.second;

        // Screen setup
        TILE = 32;
        PADDING = 0;
        const int SCR_W = W * TILE + PADDING*2;
        const int SCR_H = H * TILE + PADDING*2;
        open_window("Maze Chase (smooth + async A*)", SCR_W, SCR_H);

        // Textures and sprites
        bitmap floor_bmp   = load_bitmap("floor",   "Floor.bmp");
        bitmap wall_bmp    = load_bitmap("wall",    "Wall.bmp");
        bitmap player_bmp  = load_bitmap("player",  "Player.bmp");
        bitmap monster_bmp = load_bitmap("monster", "Monster.png");

        auto scale_for = [&](bitmap bmp)->drawing_options {
            if (bitmap_valid(bmp))
            {
                double bw = (double)bitmap_width(bmp);
                double bh = (double)bitmap_height(bmp);
                double sx = TILE / std::max(1.0, bw);
                double sy = TILE / std::max(1.0, bh);
                return option_scale_bmp(sx, sy);
            }
            return option_defaults();
        };
        drawing_options opt_floor   = scale_for(floor_bmp);
        drawing_options opt_wall    = scale_for(wall_bmp);
        drawing_options opt_player  = scale_for(player_bmp);
        drawing_options opt_monster = scale_for(monster_bmp);

        // Spawn points for player and monster on road cells
        std::random_device rd; std::mt19937 rng(rd());
        auto pspawn = random_road(maze, rng);
        auto mspawn = random_road(maze, rng);
        while (mspawn == pspawn) mspawn = random_road(maze, rng);

        Mover player;
        player.r = player.tr = pspawn.first;
        player.c = player.tc = pspawn.second;
        player.x = cell_to_px_c(player.c);
        player.y = cell_to_px_r(player.r);
        player.speed = 180.0f; // Player is a little faster which is about five point six cells per second

        Mover monster;
        monster.r = monster.tr = mspawn.first;
        monster.c = monster.tc = mspawn.second;
        monster.x = cell_to_px_c(monster.c);
        monster.y = cell_to_px_r(monster.r);
        monster.speed = 140.0f; // Monster is slightly slower

        bool game_over = false;
        bool victory   = false;

        // Asynchronous AI. Request a star periodically so the main loop is never blocked
        const double AI_INTERVAL = 0.18; // Seconds. A smaller value tracks more closely
        double ai_accum = 0.0;
        bool ai_running = false;
        std::future<nlohmann::json> ai_future;

        auto request_ai = [&]() {
            ai_running = true;
            ai_future = std::async(std::launch::async, [&]() {
                try {
                    return run_python_next_step(maze_file, monster.r, monster.c, player.r, player.c);
                } catch (...) {
                    return nlohmann::json{};
                }
            });
        };

        auto reset_round = [&](){
            // Regenerate the maze then reload it and reset state while keeping the current size
            run_python_generate_maze(maze_file, H, W, 0.08);
            maze = load_maze(maze_file);
            auto ex = find_exit_cell(maze);
            exit_r = ex.first; exit_c = ex.second;

            pspawn = random_road(maze, rng);
            mspawn = random_road(maze, rng);
            while (mspawn == pspawn) mspawn = random_road(maze, rng);

            player.r = player.tr = pspawn.first;
            player.c = player.tc = pspawn.second;
            snap_to_cell(player);

            monster.r = monster.tr = mspawn.first;
            monster.c = monster.tc = mspawn.second;
            snap_to_cell(monster);

            // Reset round state
            game_over = false;
            victory = false;
            ai_accum = 0.0;
            ai_running = false;
        };

        // Frame timing
        auto last = std::chrono::steady_clock::now();

        while (!window_close_requested("Maze Chase (smooth + async A*)"))
        {
            process_events();

            // Compute dt in seconds
            auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - last).count();
            last = now;
            dt = std::min(dt, 0.05f); // Prevent extreme frame gaps that may cause tunneling

            // Input handling while on win or loss screens takes priority
            if (victory) {
                if (key_typed(Y_KEY)) { reset_round(); }
                else if (key_typed(N_KEY) || key_typed(ESCAPE_KEY)) { break; }
            } else if (game_over) {
                if (key_typed(Y_KEY)) { reset_round(); }
                else if (key_typed(N_KEY) || key_typed(ESCAPE_KEY)) { break; }
            } else {
                // Player input. Grid to grid with smooth interpolation. Holding a direction keeps moving
                if (!player.moving)
                {
                    int dr = 0, dc = 0;
                    if (key_down(W_KEY) || key_down(UP_KEY))         dr = -1;
                    else if (key_down(S_KEY) || key_down(DOWN_KEY))  dr = +1;
                    else if (key_down(A_KEY) || key_down(LEFT_KEY))  dc = -1;
                    else if (key_down(D_KEY) || key_down(RIGHT_KEY)) dc = +1;

                    int nr = player.r + dr, nc = player.c + dc;
                    if (dr != 0 || dc != 0)
                    {
                        if (0 <= nr && nr < H && 0 <= nc && nc < W && maze[nr][nc] == ROAD)
                            start_move(player, nr, nc);
                    }
                }

                // Monster movement uses the previously computed target. If none is available it waits for AI
                if (!monster.moving)
                {
                    if (ai_running &&
                        ai_future.valid() &&
                        ai_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                    {
                        auto j = ai_future.get();
                        ai_running = false;
                        ai_accum = 0.0;

                        if (j.is_object() && j.contains("next"))
                        {
                            int nr2 = j["next"]["r"].get<int>();
                            int nc2 = j["next"]["c"].get<int>();
                            if (0 <= nr2 && nr2 < H && 0 <= nc2 && nc2 < W && maze[nr2][nc2] == ROAD)
                                start_move(monster, nr2, nc2);
                        }
                    }
                }

                // Accumulate AI scheduling time and trigger a new request when the interval elapses and no request is running
                ai_accum += dt;
                if (!ai_running && ai_accum >= AI_INTERVAL) request_ai();
            }

            // Update smooth movement for both entities
            update_mover(player, dt);
            update_mover(monster, dt);

            // Win and loss checks. Win has priority over loss
            if (!victory && player.r == exit_r && player.c == exit_c) {
                victory = true;
                ai_running = false;  // Stop further AI
            }
            if (!victory && !game_over && player.r == monster.r && player.c == monster.c) {
                game_over = true;
                ai_running = false;
            }

        // Rendering starts here
        if (victory || game_over)
        {
            // Clear the screen first and draw only the informational text
            clear_screen(COLOR_BLACK);

            if (victory) {
                draw_text("YOU WIN!", COLOR_YELLOW, "arial", 32, 12, 10);
                draw_text("Press Y: next / N: quit", COLOR_WHITE, "arial", 22, 12, 50);
            } else { // game_over
                draw_text("GAME OVER", COLOR_YELLOW, "arial", 32, 12, 10);
                draw_text("Press Y: retry / N: quit", COLOR_WHITE, "arial", 22, 12, 50);
            }

            refresh_screen(60);
            // Skip map and actor rendering for this frame to keep a clean text overlay
            continue; 
        }

        // Normal rendering of map and actors. This path is not used on win or loss frames
        clear_screen(COLOR_BLACK);

// Map tiles
for (int r=0;r<H;++r)
for (int c=0;c<W;++c)
{
    float x = (float)(c * TILE + PADDING);
    float y = (float)(r * TILE + PADDING);
    if (maze[r][c] == WALL)
    {
        if (bitmap_valid(wall_bmp)) draw_bitmap(wall_bmp, x, y, opt_wall);
        else fill_rectangle(rgba_color(35,35,40,255), x, y, TILE-1, TILE-1);
    }
    else
    {
        if (bitmap_valid(floor_bmp)) draw_bitmap(floor_bmp, x, y, opt_floor);
        else fill_rectangle(rgba_color(220,220,220,255), x, y, TILE-1, TILE-1);
    }
}

// Player sprite or fallback
if (bitmap_valid(player_bmp)) draw_bitmap(player_bmp, player.x, player.y, opt_player);
else fill_circle(COLOR_CYAN, player.x + TILE/2.0f, player.y + TILE/2.0f, TILE*0.35f);

// Monster sprite or fallback
if (bitmap_valid(monster_bmp)) draw_bitmap(monster_bmp, monster.x, monster.y, opt_monster);
else fill_circle(COLOR_RED, monster.x + TILE/2.0f, monster.y + TILE/2.0f, TILE*0.35f);

refresh_screen(60);


            // Map drawing again in the standard path
            for (int r=0;r<H;++r)
            for (int c=0;c<W;++c)
            {
                float x = (float)(c * TILE + PADDING);
                float y = (float)(r * TILE + PADDING);
                if (maze[r][c] == WALL)
                {
                    if (bitmap_valid(wall_bmp)) draw_bitmap(wall_bmp, x, y, opt_wall);
                    else fill_rectangle(rgba_color(35,35,40,255), x, y, TILE-1, TILE-1);
                }
                else
                {
                    if (bitmap_valid(floor_bmp)) draw_bitmap(floor_bmp, x, y, opt_floor);
                    else fill_rectangle(rgba_color(220,220,220,255), x, y, TILE-1, TILE-1);
                }
            }

            // Player drawing
            if (bitmap_valid(player_bmp)) draw_bitmap(player_bmp, player.x, player.y, opt_player);
            else fill_circle(COLOR_CYAN, player.x + TILE/2.0f, player.y + TILE/2.0f, TILE*0.35f);

            // Monster drawing
            if (bitmap_valid(monster_bmp)) draw_bitmap(monster_bmp, monster.x, monster.y, opt_monster);
            else fill_circle(COLOR_RED, monster.x + TILE/2.0f, monster.y + TILE/2.0f, TILE*0.35f);

            // Overlay win or loss text after the scene is drawn
            if (victory) {
                draw_text("YOU WIN!", COLOR_YELLOW, "arial", 32, 12, 10);
                draw_text("Press Y: next / N: quit", COLOR_WHITE, "arial", 22, 12, 50);
            } else if (game_over) {
                draw_text("GAME OVER", COLOR_YELLOW, "arial", 32, 12, 10);
                draw_text("Press Y: retry / N: quit", COLOR_WHITE, "arial", 22, 12, 50);
            }

            refresh_screen(60);
        }

        if (bitmap_valid(floor_bmp))   free_bitmap(floor_bmp);
        if (bitmap_valid(wall_bmp))    free_bitmap(wall_bmp);
        if (bitmap_valid(player_bmp))  free_bitmap(player_bmp);
        if (bitmap_valid(monster_bmp)) free_bitmap(monster_bmp);

    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
