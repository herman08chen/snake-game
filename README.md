# 3D Snake

Web version: https://herman08chen.itch.io/3d-snake

A small 3D snake game built with [raylib](https://github.com/raysan5/raylib) and modern C++.
Fly a snake through a 12×12×12 cube, eat coins to grow, and avoid running into
your own tail or the walls. The game has a start menu, an in-game view with an
orbital 3D camera, and a game-over screen.

## Architecture

The program is a small scene-based game loop:

- **`scene_base` / `scene_table`** — a tiny scene manager. Each scene owns its
  update and draw logic; scenes are registered by name and can request a
  switch to another scene by name (start menu → play → game-over → play).
- **`snake`** — the snake's body as a list of grid points, its direction, and
  collision detection against the walls and its own body.
- **`game`** — the play session: the snake, the coins, the score, the coin
  sound effect, and the per-frame update logic (movement, coin spawning,
  eating, game over).
- **Scenes** — `STARTMENU_SCENE`, `PLAY_SCENE`, `GAMEOVER_SCENE` implement the
  three screens. `PLAY_SCENE` owns the `game` instance and the 3D camera;
  `GAMEOVER_SCENE` holds a reference to that same game so it can display the
  score and restart it.

All raylib resources (sounds, textures, the music stream) are owned by
move-only RAII wrappers (`include/raylib_raii.h`), so each resource is loaded
after the window/audio device is ready and unloaded exactly once before it is
closed.

## How it works

1. `main()` initializes the audio device and window, locates the
   `resources/` directory, loads the background music, and constructs the
   three scenes. The game always starts on the start menu.
2. Each frame: `UpdateMusicStream()`, then the scene manager updates and draws
   the current scene at 30 FPS.
3. In the play scene the snake moves one cell every 24 frames. The player
   changes direction with the numpad or WASDQC keys; reversing directly into
   the neck is ignored.
4. Coins spawn every 128 frames (up to 5 at a time) at a random grid cell not
   occupied by the snake. When the snake's head reaches a coin, the coin is
   eaten, the snake grows, the score increases, and the coin sound plays.
5. If the head leaves the cube or touches the body, the game is over and the
   scene manager switches to the game-over screen. Pressing any key there
   resets the game and returns to the play scene.

## Dependencies

Only external third-party library:

- **[raylib](https://github.com/raysan5/raylib)** — windowing, OpenGL rendering,
  audio playback, and input handling. Official repository:
  https://github.com/raysan5/raylib

The resource-directory helper (`include/resource_dir.h`) is a small included
utility (raylibExtras, MIT-licensed) and ships with this repository, so it is
not a separate dependency.

## Installation and usage

### Prerequisites

- A C++ compiler with C++20 support (the code is written to be compatible with
  C++20 and later)
- [premake5](https://premake.github.io/) to generate build files
- GNU Make (or mingw32-make on Windows)
- OpenGL and OpenAL development libraries for raylib

### Building

From the repository root:

```sh
cd build
premake5 gmake2
cd ..
make
```

This downloads and builds raylib from its official repository as part of the
project generation, then compiles the game. On Windows you can also use the
provided `build-MinGW-W64.bat` or `build-VisualStudio2022.bat` scripts.

### Running

Run the executable from the repository root so the `resources/` directory is
found:

```sh
./bin/<config>/snake-game
```

### Controls

- Numpad 8/W — forward
- Numpad 5/S — back
- Numpad 6/D — left
- Numpad 4/A — right
- Numpad 3/C — up
- Numpad 7/Q — down
- Any key — advance past the start and game-over screens

## Example

Start a game and steer with the numpad or WASDQC:

```sh
./bin/Debug/snake-game
# press any key to start
# use 8 / 5 / 6 / 4 (or W / S / D / A) to move
# 3 / 7 (or C / Q) to move up / down
```

Eat the purple coins to grow and score points. Hitting a wall or your own tail
ends the game; press any key to try again.
