import json
import random
import argparse
import sys
from heapq import heappush, heappop
from typing import List, Tuple, Optional

# Cell semantics: 1 = WALL, 0 = ROAD
WALL, ROAD = 1, 0


def generate_single_exit_maze(
    H: int = 20,
    W: int = 20,
    seed: Optional[int] = None,
    exit_side: Optional[str] = None,
    loop_density: float = 0.08
) -> Tuple[List[List[int]], Tuple[int, int]]:
    """
    Generate a rectangular maze with exactly one exit on the outer boundary.
    The core is a DFS-backtracking maze carved on odd coordinates (1,1), (1,3), ...
    and optional "loop" openings to reduce linearity.

    Args:
        H, W: Grid height and width in cells. Recommended to be odd numbers >= 5.
              Even values are accepted but the DFS grid is still carved on odd coordinates.
        seed: Optional random seed for reproducibility.
        exit_side: If provided, must be "bottom" or "right". Otherwise chosen at random.
                   Restricting exit placement to bottom/right simplifies camera/UI in some games.
        loop_density: Fraction [0.0, 0.5] of candidate wall positions to knock down to create loops.
                      Higher values => more cycles and shorter paths.

    Returns:
        (grid, exit_rc):
            grid: H x W list of ints (1 = WALL, 0 = ROAD)
            exit_rc: (row, col) of the unique boundary exit cell

    Invariants:
        - Outer boundary is walls except the single exit.
        - Exactly one exit on the outer boundary (bottom or right edge).
        - DFS guarantees global connectivity for carved cells prior to loop openings.

    Complexity:
        - DFS carve is O(H*W).
        - Loop selection pass is O(H*W).
    """
    assert H >= 5 and W >= 5, "Maze must be at least 5x5."

    # Use a local RNG for reproducibility if seed is provided; otherwise fall back to global RNG.
    rng = random.Random(seed) if seed is not None else random

    # Clamp loop_density to a sane range to avoid surprising behavior.
    loop_density = max(0.0, min(0.5, loop_density))

    # Initialize all walls.
    g: List[List[int]] = [[WALL] * W for _ in range(H)]

    # Carve "rooms" on the odd grid (classic DFS maze base).
    for r in range(1, H, 2):
        for c in range(1, W, 2):
            if r < H - 1 and c < W - 1:
                g[r][c] = ROAD

    def inb_cell(r: int, c: int) -> bool:
        """Return True if (r,c) is an *odd* interior cell usable by the DFS."""
        return 1 <= r < H - 1 and 1 <= c < W - 1 and r % 2 == 1 and c % 2 == 1

    DIRS2 = [(-2, 0), (2, 0), (0, -2), (0, 2)]  # Jump two cells to move between odd lattice points.

    # Depth-first search stack carve from (1,1).
    start = (1, 1)
    stack = [start]
    seen = {start}

    while stack:
        r, c = stack[-1]
        nbrs = []
        for dr, dc in DIRS2:
            rr, cc = r + dr, c + dc
            if inb_cell(rr, cc) and (rr, cc) not in seen:
                nbrs.append((rr, cc))

        if not nbrs:
            # Backtrack
            stack.pop()
            continue

        # Choose a neighbor at random, carve the wall in between, and proceed.
        rr, cc = rng.choice(nbrs)
        wr, wc = (r + rr) // 2, (c + cc) // 2
        g[wr][wc] = ROAD
        seen.add((rr, cc))
        stack.append((rr, cc))

    # Optionally add loops (i.e., break walls that separate opposite ROAD cells).
    # This reduces linearity and yields multiple valid routes.
    cand = []
    for r in range(1, H - 1):
        for c in range(1, W - 1):
            if g[r][c] != WALL:
                continue
            # A wall is a candidate if it separates two opposite ROAD cells.
            if (g[r][c - 1] == ROAD and g[r][c + 1] == ROAD) or (g[r - 1][c] == ROAD and g[r + 1][c] == ROAD):
                cand.append((r, c))
    rng.shuffle(cand)
    k = int(len(cand) * loop_density)
    for i in range(k):
        r, c = cand[i]
        g[r][c] = ROAD

    # Rebuild hard outer boundary (ensures a clean single-exit perimeter).
    for c in range(W):
        g[0][c] = WALL
        g[H - 1][c] = WALL
    for r in range(H):
        g[r][0] = WALL
        g[r][W - 1] = WALL

    # Exit placement policy: bottom or right edge.
    side = exit_side if exit_side in ("bottom", "right") else rng.choice(["bottom", "right"])

    def open_bottom() -> Tuple[int, int]:
        """
        Open a single exit on the bottom border aligned with an interior ROAD cell.
        If none exist in the bottom-adjacent row, carve a vertical tunnel upward first.
        """
        cols = [c for c in range(1, W - 1) if g[H - 2][c] == ROAD]
        if not cols:
            # Create a vertical passage from a random column up to the first ROAD.
            c = rng.randrange(1, W - 1)
            r = H - 2
            while r >= 1 and g[r][c] != ROAD:
                g[r][c] = ROAD
                r -= 1
            cols = [c]
        c = rng.choice(cols)
        g[H - 1][c] = ROAD
        return (H - 1, c)

    def open_right() -> Tuple[int, int]:
        """
        Open a single exit on the right border aligned with an interior ROAD cell.
        If none exist in the right-adjacent column, carve a horizontal tunnel first.
        """
        rows = [r for r in range(1, H - 1) if g[r][W - 2] == ROAD]
        if not rows:
            r = rng.randrange(1, H - 1)
            c = W - 2
            while c >= 1 and g[r][c] != ROAD:
                g[r][c] = ROAD
                c -= 1
            rows = [r]
        r = rng.choice(rows)
        g[r][W - 1] = ROAD
        return (r, W - 1)

    exit_rc = open_bottom() if side == "bottom" else open_right()

    # Final contract: exactly one outer-road cell, at exit_rc.
    assert _check_single_exit(g, exit_rc), "Maze does not have exactly one exit as expected."
    return g, exit_rc


def _check_single_exit(g: List[List[int]], exit_rc: Tuple[int, int]) -> bool:
    """
    Verify there is exactly one boundary ROAD cell, and it matches exit_rc.
    """
    H, W = len(g), len(g[0])
    er, ec = exit_rc
    if not ((er == H - 1 and 0 <= ec < W) or (ec == W - 1 and 0 <= er < H)):
        return False
    roads = 0
    for c in range(W):
        roads += (g[0][c] == ROAD) + (g[H - 1][c] == ROAD)
    for r in range(H):
        roads += (g[r][0] == ROAD) + (g[r][W - 1] == ROAD)
    return roads == 1


def _validate_maze_shape(g: List[List[int]]) -> None:
    """
    Lightweight validation for the maze grid:
      - Non-empty 2D list
      - All rows equal length
      - Cells are 0/1 only
    Raises:
        ValueError on invalid shape/content.
    """
    if not isinstance(g, list) or not g or not isinstance(g[0], list):
        raise ValueError("maze must be a 2D list")

    W0 = len(g[0])
    if W0 == 0:
        raise ValueError("maze width must be > 0")

    for r, row in enumerate(g):
        if not isinstance(row, list) or len(row) != W0:
            raise ValueError(f"maze row {r} has inconsistent width")
        for c, v in enumerate(row):
            if v not in (WALL, ROAD):
                raise ValueError(f"maze[{r}][{c}] must be 0/1, got {v}")


def sample_random_roads_from_file(path: str, k: int = 1, seed: Optional[int] = None):
    g = _read_json_maze(path)
    H, W = len(g), len(g[0])
    roads = [(r, c) for r in range(H) for c in range(W) if g[r][c] == ROAD]
    if not roads or k <= 0:
        return []

    rng = random.Random(seed) if seed is not None else random
    rng.shuffle(roads)
    roads = roads[:min(k, len(roads))]
    return [{"r": r, "c": c} for r, c in roads]


def astar_next_step(g: List[List[int]], start: Tuple[int, int], goal: Tuple[int, int]) -> Tuple[int, int]:
    """
    Compute the *next* step from `start` toward `goal` using A* with 4-neighborhood (Manhattan).
    If the goal is not reachable, pick the node with the best f = g + h that was seen,
    and step toward that intermediate target instead. If no progress is possible, return `start`.

    Design notes:
      - We push (f, g, node) so the heap tie-breaks on lower g (shorter paths) deterministically.
      - Heuristic h is Manhattan; consistent for 4-neighborhood grids without diagonal moves.
      - Returns the last element of the reconstructed path (i.e., the immediate next move).

    Args:
        g: 2D grid of 0/1 (ROAD/WALL).
        start: (row, col) starting coordinate.
        goal: (row, col) target coordinate.

    Returns:
        (nr, nc): next coordinate to move to (or start if stuck/already there).
    """
    if start == goal:
        return start

    _validate_maze_shape(g)
    H, W = len(g), len(g[0])

    def inb(r: int, c: int) -> bool:
        return 0 <= r < H and 0 <= c < W

    # Basic coordinate validity & traversability checks.
    if not inb(*start) or not inb(*goal):
        return start  # Out-of-bounds input -> no-op
    if g[start[0]][start[1]] == WALL:
        return start  # Start placed on a wall -> no-op
    if g[goal[0]][goal[1]] == WALL:
        # Goal on a wall -> A* cannot reach it; we will fall back to best f below.
        pass

    def h(r: int, c: int) -> int:
        """Manhattan distance heuristic."""
        return abs(r - goal[0]) + abs(c - goal[1])

    openh: List[Tuple[int, int, Tuple[int, int]]] = []
    gscore = {start: 0}
    fscore = {start: h(*start)}
    came: dict[Tuple[int, int], Tuple[int, int]] = {}

    heappush(openh, (fscore[start], 0, start))
    DIRS = [(-1, 0), (1, 0), (0, -1), (0, 1)]

    while openh:
        _, _, cur = heappop(openh)
        if cur == goal:
            break
        r, c = cur
        for dr, dc in DIRS:
            rr, cc = r + dr, c + dc
            if not inb(rr, cc) or g[rr][cc] == WALL:
                continue
            ng = gscore[cur] + 1
            if ng < gscore.get((rr, cc), 1 << 30):
                came[(rr, cc)] = cur
                gscore[(rr, cc)] = ng
                fscore[(rr, cc)] = ng + h(rr, cc)
                heappush(openh, (fscore[(rr, cc)], ng, (rr, cc)))

    target = goal
    if target not in came:
        # Unreachable: pick the explored node with the smallest f as a "best-effort" intermediate.
        best, bestf = None, 1 << 30
        for p, fs in fscore.items():
            if fs < bestf:
                bestf = fs
                best = p
        if best is None:
            return start
        target = best
        if target == start:
            return start

    # Reconstruct path (backwards) and return the last element (the immediate next step).
    path: List[Tuple[int, int]] = []
    cur = target
    while cur != start:
        path.append(cur)
        cur = came.get(cur, start)
        if cur == start:
            break
    if not path:
        return start
    return path[-1]


def _read_json_maze(path: str) -> List[List[int]]:
    """
    Read a maze JSON file. The expected format for this CLI is a direct 2D array of 0/1.
    (If you support other schemas elsewhere, translate them before calling this function.)
    """
    with open(path, "r", encoding="utf-8") as f:
        g = json.load(f)
    _validate_maze_shape(g)
    return g


def main():
    """
    CLI:
      - `generate`: produce a maze JSON file (default 20x20) with exactly one exit.
      - `next-step`: calculate a single A* step from --from (r c) to --to (r c) on a given maze.
      - No subcommand: generate a default "maze.json" in the current directory.

    Examples:
      python maze.py generate --H 31 --W 31 --seed 42 --exit bottom --loop 0.10 --out mymaze.json
      python maze.py next-step --maze mymaze.json --from 1 1 --to 29 29
    """
    parser = argparse.ArgumentParser(description="Maze generator & A* next-step")
    sub = parser.add_subparsers(dest="cmd")

    # Subcommand: sample coin positions on ROAD cells (optional utility)
    ploot = sub.add_parser("loot", help="sample k coin positions on ROAD cells")
    ploot.add_argument("--maze", type=str, default="maze.json")
    ploot.add_argument("--k", type=int, default=1)
    ploot.add_argument("--seed", type=int, default=None)


    # Subcommand: generate a maze file
    pgen = sub.add_parser("generate", help="generate maze.json")
    pgen.add_argument("--H", type=int, default=20, help="maze height (cells, recommended odd >= 5)")
    pgen.add_argument("--W", type=int, default=20, help="maze width  (cells, recommended odd >= 5)")
    pgen.add_argument("--seed", type=int, default=None, help="random seed for reproducibility")
    pgen.add_argument("--exit", choices=["bottom", "right"], default=None, help="force exit side")
    pgen.add_argument(
        "--loop",
        type=float,
        default=0.08,
        help="loop density [0.0~0.5]; more loops reduce linearity"
    )
    pgen.add_argument("--out", type=str, default="maze.json", help="output file path")

    # Subcommand: compute the next step using A* on an existing maze
    pns = sub.add_parser("next-step", help="compute next step by A*")
    pns.add_argument("--maze", type=str, default="maze.json", help="path to a JSON maze file")
    pns.add_argument("--from", dest="FROM", nargs=2, type=int, required=True, help="start r c")
    pns.add_argument("--to", dest="TO", nargs=2, type=int, required=True, help="goal r c")

    args = parser.parse_args()

    # No subcommand -> generate a default maze.json and exit.
    if args.cmd is None:
        g, _ = generate_single_exit_maze()
        with open("maze.json", "w", encoding="utf-8") as f:
            json.dump(g, f, ensure_ascii=False)
        print("generated default maze.json (20x20, single exit on boundary)")
        return

    if args.cmd == "generate":
        # Basic guardrails for dimensions.
        if args.H < 5 or args.W < 5:
            print("H and W must be >= 5.", file=sys.stderr)
            sys.exit(2)
        if args.loop < 0 or args.loop > 0.5:
            # We still clamp internally, but warn the user for clarity.
            print("Warning: --loop is clamped to [0.0, 0.5]", file=sys.stderr)

        g, exit_rc = generate_single_exit_maze(
            H=args.H, W=args.W, seed=args.seed, exit_side=args.exit, loop_density=args.loop
        )
        with open(args.out, "w", encoding="utf-8") as f:
            json.dump(g, f, ensure_ascii=False)
        print(json.dumps(
            {"ok": True, "file": args.out, "exit": {"r": exit_rc[0], "c": exit_rc[1]}},
            ensure_ascii=False
        ))
        return
    
    
    if args.cmd == "loot":
        coins = sample_random_roads_from_file(args.maze, args.k, args.seed)
        print(json.dumps({"coins": coins}, ensure_ascii=False))
        return


    if args.cmd == "next-step":
        # Load and validate maze; also validate coordinates are within range.
        g = _read_json_maze(args.maze)
        H, W = len(g), len(g[0])
        sr, sc = args.FROM
        tr, tc = args.TO

        def inb(r: int, c: int) -> bool:
            return 0 <= r < H and 0 <= c < W

        if not inb(sr, sc) or not inb(tr, tc):
            print(json.dumps({"error": "start/goal out of bounds"}), file=sys.stderr)
            sys.exit(2)

        nr, nc = astar_next_step(g, (sr, sc), (tr, tc))
        print(json.dumps({"next": {"r": nr, "c": nc}}, ensure_ascii=False))
        return


if __name__ == "__main__":
    main()
