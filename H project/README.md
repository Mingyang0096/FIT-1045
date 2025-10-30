# Maze Chase Game (Latest) — README

> Last updated: 2025-10-12 

This project consists of a **C++ main program (SplashKit rendering + nlohmann::json parsing)** and **Python scripts (maze generation + A\* navigation)**.

Goals:
- Auto-generate a 20×20 maze with exactly one boundary exit where wall equals one and road equals zero. The exit is on the bottom or right edge.  
- C++ reads a two dimensional array from JSON. The player and the monster spawn at random. The player moves smoothly and the monster pursues step by step using A star.  
- Asynchronous A star with std::async prevents stutter. After win or loss the program clears the screen to show the message and allows one key to start a new round.  
- On first run maze.json is generated automatically. Python does not need to be run beforehand.

---

## Suggested Directory Layout
```
/project-root
├── main.cpp                # C++ main program (latest)
├── generator.py            # Python: maze generation & A* next step
├── maze.json               # Runtime map (auto-generated on first run)
├── Floor.bmp               # Floor texture (optional; falls back to solid color if missing)
├── Wall.bmp                # Wall texture (optional)
├── Player.bmp              # Player texture (optional)
└── Monster.png             # Monster texture (optional)
```

---

## Data Conventions
- Maze file maze.json is a top level two dimensional array with size H by W. Elements are zero for ROAD and one for WALL. There is exactly one boundary zero as the exit on the bottom or right edge.
- A star next step which is the output of generator.py next-step:
  ```json
  {"next": {"r": <int>, "c": <int>}}
  ```

---

## Dependencies and Libraries
- C++ standard library:
  - iostream, fstream, vector, string, random, cstdio, stdexcept, array, sstream, filesystem, iomanip with std::quoted, future, chrono, algorithm
- SplashKit for windowing drawing textures input text and screen refresh.
- nlohmann::json which is a single header JSON library used to read maze.json and parse Python output.
- Python 3 to run generator.py for the maze and A star computations.

On Windows if Python is launched with py change the string python to py in main.cpp.

---

## Build and Run
```bash
# Generate a maze. On the first run of ./main this is not required, but you can generate manually:
python generator.py generate --H 20 --W 20 --loop 0.08 --out maze.json

# Compile. Example using Clang with SplashKit
clang++ main.cpp -std=c++17 -l splashkit -o main

# Run
./main
```

Controls: arrow keys or WASD to move. After a win or a loss press Y for a new round or press N or ESC to quit.

---

## Key Design and Algorithms

### Smooth Movement
- The Mover structure tracks grid coordinates, target cell, pixel start and end positions, progress from zero to one, and speed in pixels per second.
- Each frame increases progress by speed times delta time divided by tile size and interpolates pixel position. When progress reaches one the mover snaps to the target cell.
- This preserves grid collision while rendering smooth motion.

### Asynchronous A star
- Run python generator.py next-step with std::async so the main loop does not block.
- Poll for completion with future.wait_for. When ready move the monster by one cell and throttle requests using AI_INTERVAL.

### Maze Generation in Python
- Build a perfect maze using an odd cell grid and depth first search wall carving.
- Optionally knock down walls by a ratio to introduce loops and multiple paths.
- Seal borders and open exactly one exit on the bottom or right edge. Validate that only one exit exists.

### First Run Autogeneration and Messages
- If maze.json is missing or cannot be parsed automatically generate a default twenty by twenty maze on the C plus plus side.
- On a win or a loss clear the screen first draw only the message and skip map and actor drawing for that frame.

---

## main.cpp Highlights
- load_maze reads and validates a two dimensional array from json.
- find_exit_cell ensures that exactly one border exit exists.
- run_python_next_step and run_python_generate_maze assemble commands with quoted paths use popen to interoperate with Python and parse json responses.
- Mover helpers start_move snap_to_cell and update_mover handle smooth interpolation.
- The main loop clamps delta time handles input schedules and polls asynchronous A star updates movers checks win and loss then renders.

---

## Future Extensions
- Coins and score system with a user interface display and optional blinking or expiry.
- Adaptive monster speed based on time or score with a reduced AI interval.
- Difficulty ramp by increasing maze size adjusting loop density and placing exits far from spawn. Optionally add multiple monsters or different AI behaviors.

---

## Troubleshooting
- cannot open maze.json. The program should auto generate this on first run. If it fails check Python and consider using py on Windows.
- json.exception.type_error.302. The top level of maze.json must be a two dimensional array. Regenerate the maze or delete the file and let it be created again.
- Stutter or lag. A star already runs asynchronously. If needed increase AI_INTERVAL or have Python return a full path and cache it in the C plus plus code.
