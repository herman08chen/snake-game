#include "raylib.h"
#include <array>
#include <utility>
#include <algorithm>
#include <list>
#include <ranges>
#include <deque>
#include <unordered_set>
#include <random>
#include <bit>
#include <format>

enum axis { POSX = 1, POSY = 2, POSZ = 3, NEGX = -1, NEGY = -2, NEGZ = -3};

struct point {
    int x; 
    int y;
    int z;
    friend bool operator==(const point&, const point&) = default;
    operator Vector3() const {
        return Vector3{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) };
    }
};

void draw_grid(const auto& coins) {
    static constexpr point cubePosition {0, 0, 0};
    static constexpr std::size_t side_length = 12;
    DrawCubeWires(cubePosition, side_length, side_length, side_length, MAROON);
    DrawGrid(12, 1.0f);
}

void draw_cube(const point& pos, const float& side_length = 1.0f, const Color& color = RED) {
    DrawCube(pos, side_length, side_length, side_length, color);
    Vector3 flat_point = pos;
    flat_point.y = 0;
    DrawCube(flat_point, 0.8f, 0, 0.8f, color);
}

struct snake {
    std::list<point> cubes {point{}, point{ -1, 0, 0 }, point{ -2, 0, 0 }};
    axis direction = POSX, prev_direction = POSX;

    point& head() {
        return cubes.front();
    }

    void increase_length() {
        cubes.emplace_back(0, 0, 0);
    }

    void update_direction() {
        if (IsKeyPressed(KEY_KP_8)) {
            direction = POSY;
        }
        else if (IsKeyPressed(KEY_KP_5)) {
            direction = NEGY;
        }
        else if (IsKeyPressed(KEY_KP_6)) {
            direction = NEGZ;
        }
        else if (IsKeyPressed(KEY_KP_4)) {
            direction = POSZ;
        }
        else if (IsKeyPressed(KEY_KP_3)) {
            direction = POSX;
        }
        else if (IsKeyPressed(KEY_KP_7)) {
            direction = NEGX;
        }
    }
    void move() {
        cubes.back() = cubes.front();
        cubes.splice(cubes.begin(), cubes, --cubes.end());

        if (direction == -1 * prev_direction)    direction = prev_direction;

        switch (direction) {
        case POSX:
            head().x++;
            break;
        case POSY:
            head().y++;
            break;
        case POSZ:
            head().z++;
            break;
        case NEGX:
            head().x--;
            break;
        case NEGY:
            head().y--;
            break;
        case NEGZ:
            head().z--;
            break;
        }
        head().x = std::clamp<int>(head().x, -6, 6);
        head().y = std::clamp<int>(head().y, -6, 6);
        head().z = std::clamp<int>(head().z, -6, 6);
        prev_direction = direction;
    }
    void draw() {
        for (auto&& cube : cubes | std::ranges::views::drop(1)) {
            draw_cube(cube, 1.0f, GREEN);
        }
        draw_cube(head(), 1.0f, RED);
    }
};

point rand_point() {
    std::random_device rd;  
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> distrib(-8, 8);
    return point{ distrib(gen), distrib(gen), distrib(gen) };
}

void draw_coins(const auto& coins) {
    for (auto&& i : coins) {
        draw_cube(i, 1.0f, PURPLE);
        DrawCubeWires(i, 1.0f, 1.0f, 1.0f, BLACK);
    }
}

struct game {
    std::array<std::array<std::array<uint8_t, 16>, 16>, 16> grid;

    std::unordered_set < point, decltype([](const auto& i) -> std::size_t {
        constexpr std::uint64_t prime{0x100000001B3};
        std::uint64_t result{0xcbf29ce484222325};
        result = (result * prime) ^ std::bit_cast<uint32_t>(i.x);
        result = (result * prime) ^ std::bit_cast<uint32_t>(i.y);
        result = (result * prime) ^ std::bit_cast<uint32_t>(i.z);
        return result;
        }) > coins;

    snake my_snake;

    std::size_t frame_count{};

    std::size_t score{};

    void update() {
        frame_count++;
        my_snake.update_direction();
        if (frame_count % 24 == 0) {
            auto it = coins.find(my_snake.head());
            if (it != coins.end()) {
                my_snake.increase_length();
                coins.erase(it);
                score++;
            }
            my_snake.move();
        }
        if (frame_count % 128 == 0 && coins.size() != 5) {
            auto coin = rand_point();
            if (!std::ranges::contains(my_snake.cubes, coin)) coins.insert(coin);
        }
    }

    void draw() {
        
    }
};

int main(void)
{
    constexpr int screenWidth = 900;
    constexpr int screenHeight = 900;

    InitWindow(screenWidth, screenHeight, "Snake!");

    Camera camera = { 0 };
    camera.position = Vector3{ 20.0f, 20.0f, 20.0f }; 
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;                              
    camera.projection = CAMERA_PERSPECTIVE;           

    game GAME;

    SetTargetFPS(30);                  

    // Main game loop
    while (!WindowShouldClose())
    {
        GAME.update();

        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText(std::format("Score: {}", GAME.score).c_str(), 30, 30, 32, BLACK);

        BeginMode3D(camera);

        draw_grid(GAME.grid);

        GAME.my_snake.draw();
        draw_coins(GAME.coins);

        EndMode3D();

        //DrawFPS(10, 10);

        EndDrawing();
    }

    CloseWindow();       

    return 0;
}