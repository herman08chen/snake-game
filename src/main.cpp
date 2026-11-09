#include "raylib.h"
#include "resource_dir.h"
#include "raylib_raii.h"
#include "scenes.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
#include <list>
#include <random>
#include <ranges>
#include <string>
#include <unordered_set>
#include <utility>

enum class axis : int {
    POSX = 1,
    POSY = 2,
    POSZ = 3,
    NEGX = -1,
    NEGY = -2,
    NEGZ = -3
};

inline constexpr int screen_width = 900;
inline constexpr int screen_height = 900;

inline constexpr std::size_t side = 12;
inline constexpr std::size_t half_side = 6;

inline constexpr int move_interval_frames = 24;
inline constexpr int coin_spawn_interval_frames = 128;
inline constexpr int max_coins = 5;
inline constexpr int max_coin_spawn_attempts = 1000;

struct point {
    int x = 0;
    int y = 0;
    int z = 0;

    [[nodiscard]] friend bool operator==(const point&, const point&) = default;

    // Converts a grid point to a raylib Vector3. Explicit to avoid silent
    // conversions in arithmetic contexts.
    [[nodiscard]] explicit operator Vector3() const noexcept {
        return Vector3{ static_cast<float>(x), static_cast<float>(y),
                        static_cast<float>(z) };
    }
};

struct point_hash {
    [[nodiscard]] std::size_t operator()(const point& p) const noexcept {
        constexpr std::uint64_t prime = 0x100000001B3ull;
        std::uint64_t result = 0xcbf29ce484222325ull;

        auto mix = [&result](std::uint32_t value) {
            result = (result * prime) ^ value;
        };

        mix(std::bit_cast<std::uint32_t>(p.x));
        mix(std::bit_cast<std::uint32_t>(p.y));
        mix(std::bit_cast<std::uint32_t>(p.z));

        return static_cast<std::size_t>(result);
    }
};

void draw_grid() {
    static constexpr Vector3 cube_center{ 0.0f, 0.0f, 0.0f };
    DrawCubeWires(cube_center, static_cast<float>(side), static_cast<float>(side),
                  static_cast<float>(side), MAROON);
    DrawGrid(static_cast<int>(side), 1.0f);
}

void draw_cube(const point& pos, float side_length = 1.0f, Color color = RED) {
    DrawCube(static_cast<Vector3>(pos), side_length, side_length, side_length,
             color);

    Vector3 flat_point = static_cast<Vector3>(pos);
    flat_point.y = 0.0f;
    DrawCube(flat_point, 0.8f, 0.0f, 0.8f, color);
}

struct snake {
    std::list<point> cubes { point{}, point{ -1, 0, 0 }, point{ -2, 0, 0 } };
    axis direction = axis::POSX;
    axis prev_direction = axis::POSX;
    bool collided = false;

    [[nodiscard]] point& head() { return cubes.front(); }
    [[nodiscard]] const point& head() const { return cubes.front(); }

    void increase_length() {
        cubes.emplace_back(-1000, -1000, -1000);
    }

    void update_direction() {
        if (IsKeyPressed(KEY_KP_8) || IsKeyPressed(KEY_W)) {
            direction = axis::POSY;
        }
        else if (IsKeyPressed(KEY_KP_5) || IsKeyPressed(KEY_S)) {
            direction = axis::NEGY;
        }
        else if (IsKeyPressed(KEY_KP_6) || IsKeyPressed(KEY_D)) {
            direction = axis::NEGZ;
        }
        else if (IsKeyPressed(KEY_KP_4) || IsKeyPressed(KEY_A)) {
            direction = axis::POSZ;
        }
        else if (IsKeyPressed(KEY_KP_3) || IsKeyPressed(KEY_C)) {
            direction = axis::POSX;
        }
        else if (IsKeyPressed(KEY_KP_7) || IsKeyPressed(KEY_Q)) {
            direction = axis::NEGX;
        }
    }

    // Advances the snake one cell: the tail segment is pulled to the front
    // and becomes the new head, moved one step in the current direction.
    // Every body segment follows the segment in front of it.
    void move() {
        cubes.back() = cubes.front();
        cubes.splice(cubes.begin(), cubes, --cubes.end());

        if (static_cast<int>(direction) == -static_cast<int>(prev_direction)) {
            // Disallow reversing directly into the neck.
            direction = prev_direction;
        }

        switch (direction) {
        case axis::POSX:
            ++head().x;
            break;
        case axis::POSY:
            ++head().y;
            break;
        case axis::POSZ:
            ++head().z;
            break;
        case axis::NEGX:
            --head().x;
            break;
        case axis::NEGY:
            --head().y;
            break;
        case axis::NEGZ:
            --head().z;
            break;
        }

        prev_direction = direction;

        const auto& h = head();
        if (h.x < -static_cast<int>(half_side) || h.x > static_cast<int>(half_side)) {
            collided = true;
        }
        if (h.y < -static_cast<int>(half_side) || h.y > static_cast<int>(half_side)) {
            collided = true;
        }
        if (h.z < -static_cast<int>(half_side) || h.z > static_cast<int>(half_side)) {
            collided = true;
        }

        if (std::ranges::any_of(cubes | std::ranges::views::drop(1),
                                [&](const point& p) { return p == head(); })) {
            collided = true;
        }
    }

    void draw() const {
        for (const auto& cube : cubes | std::ranges::views::drop(1)) {
            draw_cube(cube, 1.0f, GREEN);
        }
        draw_cube(head(), 1.0f, RED);
    }
};

point rand_point() {
    thread_local std::mt19937 generator{ std::random_device{}() };
    std::uniform_int_distribution<int> distribution(
        -static_cast<int>(half_side), static_cast<int>(half_side));
    return point{ distribution(generator), distribution(generator),
                  distribution(generator) };
}

void draw_coins(const auto& coins) {
    for (const auto& coin : coins) {
        draw_cube(coin, 1.0f, PURPLE);
        DrawCubeWires(static_cast<Vector3>(coin), 1.0f, 1.0f, 1.0f, BLACK);
    }
}

// The game state: snake, coins, score and the coin sound effect.
struct game {
    raii_sound coin_sound;
    std::unordered_set<point, point_hash> coins;
    snake my_snake;
    std::size_t frame_count = 0;
    std::size_t score = 0;
    bool game_over = false;

    game() : coin_sound(LoadSound("examples_audio_resources_coin.wav")) {
        if (!coin_sound) {
            TraceLog(LOG_WARNING, "game: failed to load coin sound effect");
        }
    }

    // Restarts the game. The previous sound is unloaded before the new one
    // is loaded, so there is no handle leak or double-unload.
    void reset() noexcept {
        coin_sound.reload(LoadSound("examples_audio_resources_coin.wav"));
        coins.clear();
        my_snake = snake{};
        frame_count = 0;
        score = 0;
        game_over = false;
    }

    void update() {
        ++frame_count;
        my_snake.update_direction();

        if (frame_count % move_interval_frames == 0) {
            my_snake.move();

            if (my_snake.collided) {
                game_over = true;
                return;
            }

            if (auto node = coins.extract(my_snake.head()); !node.empty()) {
                my_snake.increase_length();
                ++score;
                PlaySound(coin_sound.get());
            }
        }

        if (frame_count % coin_spawn_interval_frames == 0 &&
            coins.size() < max_coins && !game_over) {
            point coin = rand_point();
            int attempts = 0;
            while (std::ranges::any_of(my_snake.cubes,
                                       [&](const point& p) { return p == coin; }) &&
                   ++attempts < max_coin_spawn_attempts) {
                coin = rand_point();
            }

            if (attempts < max_coin_spawn_attempts) {
                coins.insert(coin);
            }
        }
    }
};

class STARTMENU_SCENE : public scene_base {
public:
    STARTMENU_SCENE()
        : scene_base("STARTMENU")
        , texture(LoadTexture("directions.png")) {
        if (!texture) {
            TraceLog(LOG_WARNING, "start menu: failed to load directions.png");
        }
    }

    ~STARTMENU_SCENE() override = default;

    void update() override {
        if (GetKeyPressed() != 0) [[unlikely]] {
            scene_base::request_scene("PLAY");
        }
    }

    void draw() override {
        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawText("SNAKE 3D!!!!", 30, 30, 64, BLACK);
        DrawText("by herman chen", 600, 50, 32, BLACK);
        DrawText("Press any key to play :3", 30, 110, 32, BLACK);
        DrawTexture(texture.get(),
                    screen_width / 2 - texture.get().width / 2,
                    screen_height / 2 - texture.get().height / 2, RAYWHITE);
        DrawText("use your numpad or WASDQC to control the snake", 30, 160, 32,
                 BLACK);

        EndDrawing();
    }

private:
    raii_texture texture;
};

class PLAY_SCENE : public scene_base {
public:
    PLAY_SCENE() : scene_base("PLAY") {
        camera.position = { 20.0f, 20.0f, 20.0f };
        camera.target = { 0.0f, 0.0f, 0.0f };
        camera.up = { 0.0f, 1.0f, 0.0f };
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;
    }

    ~PLAY_SCENE() override = default;

    [[nodiscard]] game& get_game() noexcept { return game_; }

    void update() override {
        game_.update();
        if (game_.game_over) [[unlikely]] {
            scene_base::request_scene("GAMEOVER");
        }
    }

    void draw() override {
        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawText(std::format("Score: {}", game_.score).c_str(), 30, 30, 32,
                 BLACK);

        BeginMode3D(camera);

        draw_grid();
        game_.my_snake.draw();
        draw_coins(game_.coins);

        EndMode3D();

        EndDrawing();
    }

private:
    Camera camera{};
    game game_;
};

class GAMEOVER_SCENE : public scene_base {
public:
    explicit GAMEOVER_SCENE(game& target)
        : scene_base("GAMEOVER")
        , game_(target)
        , texture(LoadTexture("prety_girl.png")) {
        if (!texture) {
            TraceLog(LOG_WARNING, "game over: failed to load prety_girl.png");
        }
    }

    ~GAMEOVER_SCENE() override = default;

    void update() override {
        if (GetKeyPressed() != 0) [[unlikely]] {
            game_.reset();
            scene_base::request_scene("PLAY");
        }
    }

    void draw() override {
        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawText("Game Over!", 30, 30, 32, BLACK);
        DrawText(std::format("Score: {}", game_.score).c_str(), 30, 70, 32,
                 BLACK);
        DrawText("Press any key to try again", 30, 110, 32, BLACK);

        DrawTexture(texture.get(),
                    screen_width / 2 - texture.get().width / 2,
                    screen_height / 2 - texture.get().height / 2, RAYWHITE);

        DrawText("\nThanks for playing my game!!", 200, 700, 32, BLACK);
        DrawText("\nthis game was built by the hopes \nand dreams of an idiot :3",
                 200, 750, 32, BLACK);

        EndDrawing();
    }

private:
    game& game_;
    raii_texture texture;
};

int main() noexcept {
    // Resolves the resources directory so the game can be run from any
    // working directory. Asset paths in the code are relative to it.
    if (!SearchAndSetResourceDir("resources")) {
        TraceLog(LOG_WARNING,
                 "snake: 'resources' directory not found; run the program "
                 "from the project root directory");
    }

    InitAudioDevice();
    InitWindow(screen_width, screen_height, "Snake!");

    raii_music music(LoadMusicStream("mini1111.xm"));
    if (!music) {
        TraceLog(LOG_WARNING, "snake: failed to load background music");
    }
    PlayMusicStream(music.get());

    STARTMENU_SCENE start_menu;
    PLAY_SCENE play_scene;
    GAMEOVER_SCENE game_over_scene(play_scene.get_game());

    SetTargetFPS(30);

    // The game always starts on the start menu.
    scene_table scenes{ start_menu, play_scene, game_over_scene };

    while (!WindowShouldClose()) {
        UpdateMusicStream(music.get());
        scenes.update();
        scenes.draw();
    }

    CloseWindow();
    CloseAudioDevice();

    return 0;
}
