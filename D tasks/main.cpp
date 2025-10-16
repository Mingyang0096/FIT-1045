// main.cpp — Maze + Coins + Regenerate Map + Frozen End-Stats
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <stdexcept>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <set>
#include <queue>     // for std::priority_queue
#include <cstdlib>   // for std::system
#include <cmath>     // for std::abs

#include "splashkit.h"
#include "nlohmann/json.hpp"

using std::string;
using std::vector;
using std::pair;

static const int WALL = 1;
static const int ROAD = 0;

// ---------- Tunables ----------
static int   TILE         = 32;
static int   PADDING      = 0;
static float PLAYER_SPEED = 160.0f;   // px/s
static float MONSTER_SPEED= 140.0f;   // px/s
static int   NUM_COINS    = 20;        // coins per round
static int   COIN_VALUE   = 100;      // score per coin
static float PICK_RADIUS  = 0.48f;    // in tiles
static int   FPS_LIMIT    = 60;       // refresh FPS
// -----------------------------

// px <-> cell helpers
inline float cell_to_px_c(int c) { return (float)(c * TILE + PADDING); }
inline float cell_to_px_r(int r) { return (float)(r * TILE + PADDING); }

// scale any bitmap to TILE size
inline drawing_options make_tile_scale(bitmap bmp)
{
    if (!bitmap_valid(bmp)) return option_defaults();
    double bw = (double)bitmap_width(bmp);
    double bh = (double)bitmap_height(bmp);
    double sx = TILE / (bw > 0 ? bw : 1.0);
    double sy = TILE / (bh > 0 ? bh : 1.0);
    return option_scale_bmp(sx, sy);
}

// read maze: top-level 2D int array
vector<vector<int>> load_maze(const string &path)
{
    std::ifstream in(path);
    if (!in.is_open()) throw std::runtime_error("cannot open: " + path);

    nlohmann::json nj; in >> nj;
    if (!nj.is_array() || nj.empty() || !nj[0].is_array())
        throw std::runtime_error("maze.json must be a 2D array");

    vector<vector<int>> g; g.reserve(nj.size());
    for (auto &row : nj)
    {
        vector<int> r; r.reserve(row.size());
        for (auto &x : row) r.push_back((int)x);
        g.push_back(std::move(r));
    }
    return g;
}

// find the unique border exit (r,c)
pair<int,int> find_single_exit(const vector<vector<int>>& g)
{
    int H=(int)g.size(), W=(int)g[0].size();
    vector<pair<int,int>> exits;
    for (int c=0;c<W;++c){
        if (g[0][c]==ROAD)   exits.emplace_back(0,c);
        if (g[H-1][c]==ROAD) exits.emplace_back(H-1,c);
    }
    for (int r=0;r<H;++r){
        if (g[r][0]==ROAD)   exits.emplace_back(r,0);
        if (g[r][W-1]==ROAD) exits.emplace_back(r,W-1);
    }
    if (exits.size()!=1) throw std::runtime_error("maze border must contain exactly one exit");
    return exits[0];
}

// pick a random ROAD cell
pair<int,int> random_road(const vector<vector<int>> &g, std::mt19937 &rng)
{
    int H=(int)g.size(), W=(int)g[0].size();
    vector<pair<int,int>> roads; roads.reserve(H*W/2);
    for (int r=0;r<H;++r) for (int c=0;c<W;++c)
        if (g[r][c]==ROAD) roads.emplace_back(r,c);
    if (roads.empty()) throw std::runtime_error("no ROAD cells");
    std::uniform_int_distribution<> dist(0,(int)roads.size()-1);
    return roads[dist(rng)];
}

// 判断文件是否存在（跨平台简易法）
static bool file_exists(const std::string& path)
{
    std::ifstream f(path);
    return (bool)f;
}

// 调用 Python 生成 maze.json（失败返回 false）
// 会尝试 `python`，失败再试 `python3`
static bool generate_maze_via_python(const std::string& out_path, int H, int W, int seed = -1)
{
    std::ostringstream cmd;
    cmd << "python generator.py generate --H " << H
        << " --W " << W
        << " --out " << out_path;
    if (seed >= 0) cmd << " --seed " << seed;

    int ret = std::system(cmd.str().c_str());
    if (ret != 0) {
        std::ostringstream cmd2;
        cmd2 << "python3 generator.py generate --H " << H
             << " --W " << W
             << " --out " << out_path;
        if (seed >= 0) cmd2 << " --seed " << seed;
        ret = std::system(cmd2.str().c_str());
    }
    return file_exists(out_path);
}


// movement actor with tween
struct Mover {
    int r{0}, c{0};       // current cell
    int tr{0}, tc{0};     // target cell
    float x{0}, y{0};     // current px
    float sx{0}, sy{0};   // start px
    float tx{0}, ty{0};   // target px
    float t{0};           // 0..1
    bool  moving{false};
    float speed{160.0f};  // px/s
};

void place_at_cell(Mover &m, int r, int c)
{
    m.r=r; m.c=c; m.tr=r; m.tc=c;
    m.x=cell_to_px_c(c); m.y=cell_to_px_r(r);
    m.sx=m.tx=m.x; m.sy=m.ty=m.y;
    m.t=0.0f; m.moving=false;
}

void start_move(Mover &m, int nr, int nc)
{
    m.tr=nr; m.tc=nc;
    m.sx=m.x; m.sy=m.y;
    m.tx=cell_to_px_c(nc); m.ty=cell_to_px_r(nr);
    m.t=0.0f; m.moving=true;
}

void update_mover(Mover &m, float dt)
{
    if (!m.moving) return;
    m.t += (m.speed * dt) / (float)TILE;
    if (m.t >= 1.0f) {
        m.t = 1.0f; m.moving=false;
        m.r = m.tr; m.c = m.tc;
        m.x = m.tx; m.y = m.ty;
    } else {
        m.x = m.sx + (m.tx - m.sx) * m.t;
        m.y = m.sy + (m.ty - m.sy) * m.t;
    }
}

// A*: return next step only
pair<int,int> astar_next_step(const vector<vector<int>>& g, pair<int,int> s, pair<int,int> t)
{
    if (s==t) return s;
    const int H = (int)g.size(), W = (int)g[0].size();
    auto inb = [&](int r,int c){ return 0<=r && r<H && 0<=c && c<W; };
    auto h   = [&](int r,int c){ return std::abs(r-t.first)+std::abs(c-t.second); };
    static const int DR[4]={-1,1,0,0};
    static const int DC[4]={0,0,-1,1};

    struct Node{int r,c,g,f;};
    struct Cmp{
        bool operator()(const Node&a, const Node&b)const{
            return (a.f>b.f) || (a.f==b.f && a.g>b.g);
        }
    };

    std::vector<std::vector<int>> gscore(H, std::vector<int>(W, 1<<29));
    std::vector<std::vector<pair<int,int>>> came(H, std::vector<pair<int,int>>(W, {-1,-1}));
    std::priority_queue<Node, std::vector<Node>, Cmp> pq;

    gscore[s.first][s.second]=0;
    pq.push({s.first,s.second,0,h(s.first,s.second)});

    while(!pq.empty()){
        auto cur = pq.top(); pq.pop();
        if (cur.r==t.first && cur.c==t.second) break;
        for(int k=0;k<4;++k){
            int rr=cur.r+DR[k], cc=cur.c+DC[k];
            if(!inb(rr,cc) || g[rr][cc]==WALL) continue;
            int ng=cur.g+1;
            if (ng<gscore[rr][cc]){
                gscore[rr][cc]=ng;
                came[rr][cc]={cur.r,cur.c};
                int nf=ng+h(rr,cc);
                pq.push({rr,cc,ng,nf});
            }
        }
    }

    auto target=t;
    if (came[target.first][target.second].first==-1){ // unreachable
        int bestf=1<<30; pair<int,int> best=s;
        for(int r=0;r<H;++r) for(int c=0;c<W;++c){
            if (gscore[r][c]<(1<<29)){
                int f=gscore[r][c]+h(r,c);
                if (f<bestf){bestf=f; best={r,c};}
            }
        }
        target=best; if (target==s) return s;
    }
    // backtrack to s; take next step
    pair<int,int> cur=target;
    vector<pair<int,int>> path;
    while(!(cur==s)){
        path.push_back(cur);
        auto p=came[cur.first][cur.second];
        if (p.first==-1) break;
        cur=p;
    }
    if (path.empty()) return s;
    return path.back();
}

// coin
struct Coin { int r{0}, c{0}; bool collected{false}; };

inline bool walkable(const vector<vector<int>>& g, int r, int c){
    return 0<=r && r<(int)g.size() && 0<=c && c<(int)g[0].size() && g[r][c]==ROAD;
}

int main()
{
    try{
        // 若一开始没有 maze.json，则先自动生成一张
        const std::string maze_path = "maze.json";
        // 你想要的默认尺寸（建议奇数更规整）；可按需改
        const int GEN_H = 25;
        const int GEN_W = 25;

        if (!file_exists(maze_path)) {
            // 可选：给个随机 seed，保证每次首次运行都不同
            std::random_device rd; std::mt19937 rng(rd());
            int seed = std::uniform_int_distribution<int>(0, 1'000'000'000)(rng);

            bool ok = generate_maze_via_python(maze_path, GEN_H, GEN_W, seed);
            if (!ok) {
                std::cerr << "ERROR: maze.json not found, and auto-generation failed. "
                            "Make sure generator.py is next to the executable.\n";
                return 1;
            }
        }

        // 现在一定有 maze.json 了，再加载
        auto maze = load_maze(maze_path);

        // initial maze load (you can create one with: python generator.py generate)
        const int INIT_H=(int)maze.size(), INIT_W=(int)maze[0].size();

        // window built for initial size; we keep H/W constant when regenerating
        const int SCR_W = INIT_W*TILE + PADDING*2;
        const int SCR_H = INIT_H*TILE + PADDING*2;
        open_window("Maze + Coins", SCR_W, SCR_H);

        // textures
        bitmap floor_bmp   = load_bitmap("floor",   "Floor.bmp");
        bitmap wall_bmp    = load_bitmap("wall",    "Wall.bmp");
        bitmap player_bmp  = load_bitmap("player",  "Player.bmp");
        bitmap monster_bmp = load_bitmap("monster", "Monster.png");
        bitmap gold_bmp    = load_bitmap("gold",    "Gold.png");

        // scales
        drawing_options opt_floor   = make_tile_scale(floor_bmp);
        drawing_options opt_wall    = make_tile_scale(wall_bmp);
        drawing_options opt_player  = make_tile_scale(player_bmp);
        drawing_options opt_monster = make_tile_scale(monster_bmp);
        drawing_options opt_gold    = make_tile_scale(gold_bmp);

        std::random_device rd; std::mt19937 rng(rd());

        auto exit_cell = find_single_exit(maze);
        auto pspawn = random_road(maze, rng);
        auto mspawn = random_road(maze, rng);
        while (mspawn==pspawn || mspawn==exit_cell) mspawn = random_road(maze, rng);

        Mover player, monster;
        place_at_cell(player,  pspawn.first, pspawn.second);
        place_at_cell(monster, mspawn.first, mspawn.second);
        player.speed  = PLAYER_SPEED;
        monster.speed = MONSTER_SPEED;

        vector<Coin> coins;
        int coins_collected = 0;
        int score = 0;

        // 🔒 冻结的结算数据（避免被重置后显示为空）
        int  last_coins_collected = 0;
        int  last_score = 0;
        bool end_stats_ready = false;

        auto respawn_coins = [&](const pair<int,int>& p, const pair<int,int>& m){
            std::set<pair<int,int>> used{p, m, exit_cell};
            coins.clear();
            for (int i=0;i<NUM_COINS;++i){
                auto cs = random_road(maze, rng);
                while (used.count(cs)) cs = random_road(maze, rng);
                used.insert(cs);
                coins.push_back(Coin{cs.first, cs.second, false});
            }
            coins_collected = 0;
            score = 0;
        };
        respawn_coins(pspawn, mspawn);

        bool victory=false, game_over=false;

        // regenerate maze of same size, then reset everything
        auto reset_round = [&](){
            int H = INIT_H, W = INIT_W;
            int seed = std::uniform_int_distribution<int>(0, 1'000'000'000)(rng);

            std::ostringstream cmd;
            cmd << "python generator.py generate --H " << H
                << " --W " << W
                << " --seed " << seed
                << " --out maze.json";
            int ret = std::system(cmd.str().c_str());
            if (ret != 0) { // python3 fallback
                std::ostringstream cmd2;
                cmd2 << "python3 generator.py generate --H " << H
                     << " --W " << W
                     << " --seed " << seed
                     << " --out maze.json";
                ret = std::system(cmd2.str().c_str());
            }
            // reload
            maze = load_maze("maze.json");
            exit_cell = find_single_exit(maze);

            // respawn characters
            auto np = random_road(maze, rng);
            auto nm = random_road(maze, rng);
            while (nm==np || nm==exit_cell) nm = random_road(maze, rng);

            place_at_cell(player,  np.first, np.second);
            place_at_cell(monster, nm.first, nm.second);

            respawn_coins(np, nm);

            // 清理结算态
            victory=false; game_over=false;
            end_stats_ready = false;
        };

        auto t0 = std::chrono::high_resolution_clock::now();

        while (!window_close_requested("Maze + Coins"))
        {
            process_events();

            // dt
            auto t1 = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(t1 - t0).count();
            t0 = t1;

            // input -> grid move
            if (!player.moving && !victory && !game_over){
                int dr=0, dc=0;
                if (key_down(W_KEY) || key_down(UP_KEY))         dr=-1;
                else if (key_down(S_KEY) || key_down(DOWN_KEY))  dr=+1;
                else if (key_down(A_KEY) || key_down(LEFT_KEY))  dc=-1;
                else if (key_down(D_KEY) || key_down(RIGHT_KEY)) dc=+1;
                int nr=player.r+dr, nc=player.c+dc;
                if ((dr||dc) && walkable(maze,nr,nc)){
                    start_move(player, nr,nc);
                }
            }

            // monster AI
            if (!monster.moving && !victory && !game_over){
                auto step = astar_next_step(maze, {monster.r,monster.c}, {player.r,player.c});
                if (step != pair<int,int>{monster.r,monster.c}) {
                    start_move(monster, step.first, step.second);
                }
            }

            // tween updates
            if (!victory && !game_over){
                update_mover(player,  dt);
                update_mover(monster, dt);
            }

            // coin pickup by player
            if (!victory && !game_over){
                float px = player.x + TILE*0.5f;
                float py = player.y + TILE*0.5f;
                float rad = PICK_RADIUS * TILE;
                float rad2 = rad*rad;
                for (auto &coin : coins){
                    if (coin.collected) continue;
                    float cx = cell_to_px_c(coin.c) + TILE*0.5f;
                    float cy = cell_to_px_r(coin.r) + TILE*0.5f;
                    float dx = px-cx, dy=py-cy;
                    if (dx*dx+dy*dy <= rad2){
                        coin.collected=true;
                        ++coins_collected;
                        score += COIN_VALUE;
                    }
                }
            }

            // win/lose checks
            if (!victory && !game_over){
                if (!player.moving && player.r==exit_cell.first && player.c==exit_cell.second){
                    victory = true;
                }
                if (!player.moving && !monster.moving && player.r==monster.r && player.c==monster.c){
                    game_over = true;
                }
            }

            // 一旦进入结算，冻结本局统计，后续不再变
            if ((victory || game_over) && !end_stats_ready){
                last_coins_collected = coins_collected;
                last_score = score;
                end_stats_ready = true;
            }

            // render
            clear_screen(COLOR_BLACK);

            // map
            int H=(int)maze.size(), W=(int)maze[0].size();
            for (int r=0;r<H;++r) for(int c=0;c<W;++c){
                float x=cell_to_px_c(c), y=cell_to_px_r(r);
                if (bitmap_valid(floor_bmp)) draw_bitmap(floor_bmp, x,y, opt_floor);
                else fill_rectangle(COLOR_GRAY, x,y, TILE,TILE);
                if (maze[r][c]==WALL){
                    if (bitmap_valid(wall_bmp)) draw_bitmap(wall_bmp, x,y, opt_wall);
                    else fill_rectangle(COLOR_DARK_GREEN, x,y, TILE,TILE);
                }
            }

            // coins (only in play)
            if (!victory && !game_over){
                for (const auto &coin: coins){
                    if (coin.collected) continue;
                    float x=cell_to_px_c(coin.c), y=cell_to_px_r(coin.r);
                    if (bitmap_valid(gold_bmp)) draw_bitmap(gold_bmp, x,y, opt_gold);
                    else fill_circle(COLOR_YELLOW, x+TILE*0.5f, y+TILE*0.5f, TILE*0.30f);
                }
            }

            // actors
            if (bitmap_valid(player_bmp)) draw_bitmap(player_bmp, player.x, player.y, opt_player);
            else fill_rectangle(COLOR_BLUE, player.x, player.y, TILE,TILE);
            if (bitmap_valid(monster_bmp)) draw_bitmap(monster_bmp, monster.x, monster.y, opt_monster);
            else fill_rectangle(COLOR_RED, monster.x, monster.y, TILE,TILE);

            // HUD during play
            if (!victory && !game_over){
                std::ostringstream hud; hud<<"Coins "<<coins_collected<<"/"<<NUM_COINS<<"   Score "<<score;
                draw_text(hud.str(), COLOR_WHITE, "arial", 18, 8, 8);
            }

            // end screens (use FROZEN stats)
            if (victory || game_over){
                clear_screen(color_black());
                if (victory){
                    draw_text("YOU WIN!", COLOR_RED, "arial", 32, 12, 10);
                    draw_text("Press Y: next / N: quit", COLOR_WHITE, "arial", 22, 12, 50);
                } else {
                    draw_text("GAME OVER", COLOR_RED, "arial", 32, 12, 10);
                    draw_text("Press Y: retry / N: quit", COLOR_WHITE, "arial", 22, 12, 50);
                }
                std::ostringstream ss;
                ss << "Coins: " << last_coins_collected << "   Score: " << last_score;
                draw_text(ss.str(), COLOR_RED, "arial", 22, 12, 86);

                if (key_typed(Y_KEY))      reset_round();
                else if (key_typed(N_KEY) || key_typed(ESCAPE_KEY)) break;
            }

            refresh_screen(FPS_LIMIT);
        }

        // free bitmaps
        if (bitmap_valid(floor_bmp))   free_bitmap(floor_bmp);
        if (bitmap_valid(wall_bmp))    free_bitmap(wall_bmp);
        if (bitmap_valid(player_bmp))  free_bitmap(player_bmp);
        if (bitmap_valid(monster_bmp)) free_bitmap(monster_bmp);
        if (bitmap_valid(gold_bmp))    free_bitmap(gold_bmp);

    }catch(const std::exception& e){
        std::cerr<<"ERROR: "<<e.what()<<"\n";
        return 1;
    }
    return 0;
}
