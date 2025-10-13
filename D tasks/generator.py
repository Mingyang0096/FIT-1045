# generator.py
# -*- coding: utf-8 -*-
import json
import random
import argparse
from heapq import heappush, heappop
from typing import List, Tuple, Optional

WALL, ROAD = 1, 0

def generate_single_exit_maze(
    H: int = 20,
    W: int = 20,
    seed: Optional[int] = None,
    exit_side: Optional[str] = None,
    loop_density: float = 0.08
) -> Tuple[List[List[int]], Tuple[int,int]]:
    assert H >= 5 and W >= 5
    if seed is not None:
        random.seed(seed)

    g = [[WALL]*W for _ in range(H)]

    for r in range(1, H, 2):
        for c in range(1, W, 2):
            if r < H-1 and c < W-1:
                g[r][c] = ROAD

    def inb_cell(r, c): return 1 <= r < H-1 and 1 <= c < W-1 and r%2==1 and c%2==1
    DIRS2 = [(-2,0),(2,0),(0,-2),(0,2)]

    start = (1,1)
    stack = [start]
    seen = {start}

    while stack:
        r,c = stack[-1]
        nbrs = []
        for dr,dc in DIRS2:
            rr,cc = r+dr, c+dc
            if inb_cell(rr,cc) and (rr,cc) not in seen:
                nbrs.append((rr,cc))
        if not nbrs:
            stack.pop(); continue
        rr,cc = random.choice(nbrs)
        wr,wc = (r+rr)//2, (c+cc)//2
        g[wr][wc] = ROAD
        seen.add((rr,cc))
        stack.append((rr,cc))

    cand = []
    for r in range(1, H-1):
        for c in range(1, W-1):
            if g[r][c] != WALL: continue
            if (g[r][c-1]==ROAD and g[r][c+1]==ROAD) or (g[r-1][c]==ROAD and g[r+1][c]==ROAD):
                cand.append((r,c))
    random.shuffle(cand)
    k = int(len(cand)*loop_density)
    for i in range(k):
        r,c = cand[i]
        g[r][c] = ROAD

    for c in range(W):
        g[0][c] = WALL
        g[H-1][c] = WALL
    for r in range(H):
        g[r][0] = WALL
        g[r][W-1] = WALL

    side = exit_side if exit_side in ("bottom","right") else random.choice(["bottom","right"])

    def open_bottom():
        cols = [c for c in range(1, W-1) if g[H-2][c]==ROAD]
        if not cols:
            c = random.randrange(1, W-1)
            r = H-2
            while r>=1 and g[r][c]!=ROAD:
                g[r][c]=ROAD; r-=1
            cols = [c]
        c = random.choice(cols)
        g[H-1][c] = ROAD
        return (H-1,c)

    def open_right():
        rows = [r for r in range(1, H-1) if g[r][W-2]==ROAD]
        if not rows:
            r = random.randrange(1, H-1)
            c = W-2
            while c>=1 and g[r][c]!=ROAD:
                g[r][c]=ROAD; c-=1
            rows = [r]
        r = random.choice(rows)
        g[r][W-1] = ROAD
        return (r,W-1)

    exit_rc = open_bottom() if side=="bottom" else open_right()
    assert _check_single_exit(g, exit_rc)
    return g, exit_rc

def _check_single_exit(g: List[List[int]], exit_rc: Tuple[int,int]) -> bool:
    H, W = len(g), len(g[0])
    er, ec = exit_rc
    if not ((er==H-1 and 0<=ec<W) or (ec==W-1 and 0<=er<H)): return False
    roads = 0
    for c in range(W): roads += (g[0][c]==ROAD) + (g[H-1][c]==ROAD)
    for r in range(H): roads += (g[r][0]==ROAD) + (g[r][W-1]==ROAD)
    return roads == 1

def astar_next_step(g: List[List[int]], start: Tuple[int,int], goal: Tuple[int,int]) -> Tuple[int,int]:
    if start == goal: return start
    H, W = len(g), len(g[0])

    def inb(r,c): return 0 <= r < H and 0 <= c < W
    def h(a,b): return abs(a-goal[0]) + abs(b-goal[1])

    openh = []
    gscore = {start: 0}
    fscore = {start: h(*start)}
    came = {}
    heappush(openh, (fscore[start], 0, start))
    DIRS = [(-1,0),(1,0),(0,-1),(0,1)]

    while openh:
        _, _, cur = heappop(openh)
        if cur == goal: break
        r,c = cur
        for dr,dc in DIRS:
            rr,cc = r+dr, c+dc
            if not inb(rr,cc) or g[rr][cc]==WALL: continue
            ng = gscore[cur]+1
            if ng < gscore.get((rr,cc), 1<<30):
                came[(rr,cc)] = cur
                gscore[(rr,cc)] = ng
                fscore[(rr,cc)] = ng + h(rr,cc)
                heappush(openh, (fscore[(rr,cc)], ng, (rr,cc)))

    target = goal
    if target not in came:
        best, bestf = None, 1<<30
        for p, fs in fscore.items():
            if fs < bestf: bestf = fs; best = p
        if best is None: return start
        target = best
        if target == start: return start

    path = []
    cur = target
    while cur != start:
        path.append(cur)
        cur = came.get(cur, start)
        if cur == start: break
    if not path: return start
    return path[-1]

def main():
    parser = argparse.ArgumentParser(description="Maze generator & A* next-step")
    sub = parser.add_subparsers(dest="cmd")

    pgen = sub.add_parser("generate", help="generate maze.json")
    pgen.add_argument("--H", type=int, default=20)
    pgen.add_argument("--W", type=int, default=20)
    pgen.add_argument("--seed", type=int, default=None)
    pgen.add_argument("--exit", choices=["bottom","right"], default=None)
    pgen.add_argument("--loop", type=float, default=0.08)
    pgen.add_argument("--out", type=str, default="maze.json")

    pns = sub.add_parser("next-step", help="compute next step by A*")
    pns.add_argument("--maze", type=str, default="maze.json")
    pns.add_argument("--from", dest="FROM", nargs=2, type=int, required=True)
    pns.add_argument("--to",   dest="TO",   nargs=2, type=int, required=True)

    args = parser.parse_args()

    if args.cmd is None:
        g, _ = generate_single_exit_maze()
        with open("maze.json", "w", encoding="utf-8") as f:
            json.dump(g, f, ensure_ascii=False)
        print("generated default maze.json (20x20, single entrance)")
        return

    if args.cmd == "generate":
        g, exit_rc = generate_single_exit_maze(
            H=args.H, W=args.W, seed=args.seed, exit_side=args.exit, loop_density=args.loop
        )
        with open(args.out, "w", encoding="utf-8") as f:
            json.dump(g, f, ensure_ascii=False)
        print(json.dumps({"ok": True, "file": args.out, "exit": {"r": exit_rc[0], "c": exit_rc[1]}}, ensure_ascii=False))
        return

    if args.cmd == "next-step":
        with open(args.maze, "r", encoding="utf-8") as f:
            g = json.load(f)
        sr, sc = args.FROM
        tr, tc = args.TO
        nr, nc = astar_next_step(g, (sr,sc), (tr,tc))
        print(json.dumps({"next": {"r": nr, "c": nc}}, ensure_ascii=False))
        return

if __name__ == "__main__":
    main()
