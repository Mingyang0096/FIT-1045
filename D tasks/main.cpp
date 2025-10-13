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

// ==================== 读取顶层二维数组 ====================
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

// ==================== 查找唯一出口（边界上的唯一 ROAD） ====================
std::pair<int,int> find_exit_cell(const std::vector<std::vector<int>> &g)
{
    int H = (int)g.size(), W = (int)g[0].size();
    std::vector<std::pair<int,int>> exits;
    // 上下边
    for (int c=0;c<W;++c){
        if (g[0][c]==ROAD) exits.emplace_back(0,c);
        if (g[H-1][c]==ROAD) exits.emplace_back(H-1,c);
    }
    // 左右边
    for (int r=0;r<H;++r){
        if (g[r][0]==ROAD) exits.emplace_back(r,0);
        if (g[r][W-1]==ROAD) exits.emplace_back(r,W-1);
    }
    if (exits.size()!=1) throw std::runtime_error("maze border must contain exactly one exit");
    return exits[0];
}

// ==================== 随机路格 ====================
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

// ==================== 调 Python（同步）返回 JSON ====================
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

// ==================== 让 Python 生成 maze.json（指定尺寸与环密度） ====================
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
    while (fgets(buf, (int)sizeof(buf), pipe)) { /* 可在此打印日志：puts(buf); */ }
#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif
}

// ==================== 平滑移动器（网格 + 像素插值） ====================
struct Mover {
    // 网格坐标（格为单位）
    int r{0}, c{0};
    int tr{0}, tc{0};     // 目标格（移动完成时写入 r,c）

    // 像素坐标（渲染使用）
    float x{0}, y{0};
    float sx{0}, sy{0};   // 起点像素
    float tx{0}, ty{0};   // 终点像素

    // 进度（0~1）
    float t{0};
    bool moving{false};

    // 速度（像素/秒）
    float speed{160.0f};  // 玩家默认 160px/s，TILE=32 即 5格/秒
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

void snap_to_cell(Mover &m) // 立即对齐到当前 r,c 的像素位置
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
    // 每秒 speed 像素；跨一个格需要 TILE 像素 => t 增量 = (speed*dt)/TILE
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

        // ---------- 首次运行自动生成地图（如果不存在或读取失败） ----------
        if (!std::filesystem::exists(maze_file)) {
            // 首次没有文件：按默认 20x20 生成
            run_python_generate_maze(maze_file, 20, 20, 0.08);
        }

        std::vector<std::vector<int>> maze;
        try {
            maze = load_maze(maze_file);
        } catch (...) {
            // 文件存在但内容坏了/格式不对：重新生成再读
            run_python_generate_maze(maze_file, 20, 20, 0.08);
            maze = load_maze(maze_file);
        }

        int H = (int)maze.size();
        int W = (int)maze[0].size();
        auto exit_cell = find_exit_cell(maze);
        int exit_r = exit_cell.first, exit_c = exit_cell.second;

        // 画面
        TILE = 32;
        PADDING = 0;
        const int SCR_W = W * TILE + PADDING*2;
        const int SCR_H = H * TILE + PADDING*2;
        open_window("Maze Chase (smooth + async A*)", SCR_W, SCR_H);

        // 贴图
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

        // 出生
        std::random_device rd; std::mt19937 rng(rd());
        auto pspawn = random_road(maze, rng);
        auto mspawn = random_road(maze, rng);
        while (mspawn == pspawn) mspawn = random_road(maze, rng);

        Mover player;
        player.r = player.tr = pspawn.first;
        player.c = player.tc = pspawn.second;
        player.x = cell_to_px_c(player.c);
        player.y = cell_to_px_r(player.r);
        player.speed = 180.0f; // 玩家更快点：~5.6 格/秒

        Mover monster;
        monster.r = monster.tr = mspawn.first;
        monster.c = monster.tc = mspawn.second;
        monster.x = cell_to_px_c(monster.c);
        monster.y = cell_to_px_r(monster.r);
        monster.speed = 140.0f; // 怪物稍慢

        bool game_over = false;
        bool victory   = false;

        // === AI 异步：每隔一定时间请求一次 A*，不阻塞主循环 ===
        const double AI_INTERVAL = 0.18; // 秒；越小越“跟手”
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
            // 重新生成迷宫 -> 重新读取 -> 重置状态（维持尺寸不变）
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

            // 重置状态
            game_over = false;
            victory = false;
            ai_accum = 0.0;
            ai_running = false;
        };

        // 帧时间
        auto last = std::chrono::steady_clock::now();

        while (!window_close_requested("Maze Chase (smooth + async A*)"))
        {
            process_events();

            // 计算 dt（秒）
            auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - last).count();
            last = now;
            dt = std::min(dt, 0.05f); // 防止极端卡顿导致穿模

            // 胜利/失败界面输入：优先处理
            if (victory) {
                if (key_typed(Y_KEY)) { reset_round(); }
                else if (key_typed(N_KEY) || key_typed(ESCAPE_KEY)) { break; }
            } else if (game_over) {
                if (key_typed(Y_KEY)) { reset_round(); }
                else if (key_typed(N_KEY) || key_typed(ESCAPE_KEY)) { break; }
            } else {
                // --- 玩家输入（平滑格间移动；按住方向键连续走） ---
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

                // --- 怪物移动（使用上一次算好的目标；若无则等待 AI） ---
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

                // --- 累计 AI 调度时间，到点且未在算时再发起新请求 ---
                ai_accum += dt;
                if (!ai_running && ai_accum >= AI_INTERVAL) request_ai();
            }

            // --- 更新平滑移动 ---
            update_mover(player, dt);
            update_mover(monster, dt);

            // --- 终点/失败判定（胜利优先于失败） ---
            if (!victory && player.r == exit_r && player.c == exit_c) {
                victory = true;
                ai_running = false;  // 停止进一步 AI
            }
            if (!victory && !game_over && player.r == monster.r && player.c == monster.c) {
                game_over = true;
                ai_running = false;
            }

        // --- 渲染 ---
        if (victory || game_over)
        {
            // 先清屏再只显示文字提示
            clear_screen(COLOR_BLACK);

            if (victory) {
                draw_text("YOU WIN!", COLOR_YELLOW, "arial", 32, 12, 10);
                draw_text("Press Y: next / N: quit", COLOR_WHITE, "arial", 22, 12, 50);
            } else { // game_over
                draw_text("GAME OVER", COLOR_YELLOW, "arial", 32, 12, 10);
                draw_text("Press Y: retry / N: quit", COLOR_WHITE, "arial", 22, 12, 50);
            }

            refresh_screen(60);
            // 跳过当前帧的地图与角色绘制，保持纯文字背景
            continue; 
        }

        // ---- 正常渲染地图与角色（胜利/失败时不会走到这里） ----
        clear_screen(COLOR_BLACK);

// 地图
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

// 玩家
if (bitmap_valid(player_bmp)) draw_bitmap(player_bmp, player.x, player.y, opt_player);
else fill_circle(COLOR_CYAN, player.x + TILE/2.0f, player.y + TILE/2.0f, TILE*0.35f);

// 怪物
if (bitmap_valid(monster_bmp)) draw_bitmap(monster_bmp, monster.x, monster.y, opt_monster);
else fill_circle(COLOR_RED, monster.x + TILE/2.0f, monster.y + TILE/2.0f, TILE*0.35f);

refresh_screen(60);


            // 地图
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

            // 玩家
            if (bitmap_valid(player_bmp)) draw_bitmap(player_bmp, player.x, player.y, opt_player);
            else fill_circle(COLOR_CYAN, player.x + TILE/2.0f, player.y + TILE/2.0f, TILE*0.35f);

            // 怪物
            if (bitmap_valid(monster_bmp)) draw_bitmap(monster_bmp, monster.x, monster.y, opt_monster);
            else fill_circle(COLOR_RED, monster.x + TILE/2.0f, monster.y + TILE/2.0f, TILE*0.35f);

            // 叠加胜利/失败界面
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
