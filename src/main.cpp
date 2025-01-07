#include "raylib.h"
#include "scenes.cpp"
#include <bits/stdc++.h>

enum axis { POSX = 1, POSY = 2, POSZ = 3, NEGX = -1, NEGY = -2, NEGZ = -3};

static constexpr int SIDE = 12;
static constexpr int HSIDE = 6;

constexpr int screenWidth = 900;
constexpr int screenHeight = 900;

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
    static constexpr std::size_t side_length = SIDE;
    DrawCubeWires(cubePosition, side_length, side_length, side_length, MAROON);
    DrawGrid(SIDE, 1.0f);
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
    bool collided = false;
    

    point& head() {
        return cubes.front();
    }

    void increase_length() {
        cubes.emplace_back(-1000, -1000, -1000);
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

        prev_direction = direction;

        if (head().x < -HSIDE || head().x > HSIDE) collided = true;
        if (head().y < -HSIDE || head().y > HSIDE) collided = true;
        if (head().z < -HSIDE || head().z > HSIDE) collided = true;
        if (std::ranges::contains(cubes | std::ranges::views::drop(1), head()))  collided = true;
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
    std::uniform_int_distribution<> distrib(-HSIDE, HSIDE);
    return point{ distrib(gen), distrib(gen), distrib(gen) };
}

void draw_coins(const auto& coins) {
    for (auto&& i : coins) {
        draw_cube(i, 1.0f, PURPLE);
        DrawCubeWires(i, 1.0f, 1.0f, 1.0f, BLACK);
    }
}

struct game {
    std::array<std::array<std::array<uint8_t, 16>, 16>, 16> grid{};
    Sound coin_sound;

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

    bool game_over = false;

    game() {
        coin_sound = LoadSound("resources/examples_audio_resources_coin.wav");
    }
    ~game() {
        UnloadSound(coin_sound);
    }

    void update() {
        frame_count++;
        my_snake.update_direction();
        if (frame_count % 24 == 0) {
            my_snake.move();
            if (my_snake.collided) {
                game_over = true;
                return;
            }
            auto it = coins.find(my_snake.head());
            if (it != coins.end()) {
                my_snake.increase_length();
                coins.erase(it);
                score++;
                PlaySound(coin_sound);
            }
            
        }
        if (frame_count % 128 == 0 && coins.size() != 5 && !game_over) {
            auto coin = rand_point();
            while (std::ranges::contains(my_snake.cubes, coin)) {
                coin = rand_point();
            } 
            coins.insert(coin);
        }
    }

};

struct STARTMENU_SCENE : scene_base {
    const std::string _name = "STARTMENU";
    Texture2D texture = LoadTexture("resources/directions.png");

    STARTMENU_SCENE() {};
    ~STARTMENU_SCENE() {
        UnloadTexture(texture);
    }

    const std::string& name() const override { return _name; }
    void update() override {
        if (GetKeyPressed()) [[unlikely]] {
            scene_base::request_scene("PLAY");
        }
    }
    void draw() override {
        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawText("SNAKE 3D!!!!", 30, 30, 64, BLACK);
        DrawText("by herman chen", 600, 50, 32, BLACK);
        DrawText("Press any key to play :3", 30, 110, 32, BLACK);
        DrawTexture(texture, screenWidth / 2 - texture.width / 2, screenHeight / 2 - texture.height / 2, RAYWHITE);
        DrawText("use your numpad to control the snake", 30, 160, 32, BLACK);

        EndDrawing();
    }
};

struct PLAY_SCENE : scene_base {
    game GAME;
    Texture2D texture = LoadTexture("resources/directions.png");
    const std::string _name = "PLAY";
    Camera camera = { 0 };

    PLAY_SCENE() {
        camera.position = Vector3{ 20.0f, 20.0f, 20.0f };
        camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
        camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;
    }
    ~PLAY_SCENE() {
        UnloadTexture(texture);
    }

    const std::string& name() const override { return _name; }
    void update() override {
        GAME.update();
        if (GAME.game_over)  scene_base::request_scene("GAMEOVER");
    }
    void draw() override {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText(std::format("Score: {}", GAME.score).c_str(), 30, 30, 32, BLACK);
        //DrawTexture(texture, screenWidth / 2 - texture.width / 2, screenHeight / 2 - texture.height / 2, RAYWHITE);

        BeginMode3D(camera);

        draw_grid(GAME.grid);

        GAME.my_snake.draw();
        draw_coins(GAME.coins);

        EndMode3D();

        EndDrawing();
    }

};

struct GAMEOVER_SCENE : scene_base {
    game& GAME;
    const std::string _name = "GAMEOVER";
    Texture2D texture = LoadTexture("resources/prety_girl.png");

    GAMEOVER_SCENE(game& GAME) : GAME(GAME) {}
    ~GAMEOVER_SCENE() = default;

    const std::string& name() const override { return _name; }
    void update() override {
        if (GetKeyPressed()) [[unlikely]] {
            GAME = game{};
            scene_base::request_scene("PLAY");
        }
    }
    void draw() override {
        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawText("Game Over!", 30, 30, 32, BLACK);
        DrawText(std::format("Score: {}", GAME.score).c_str(), 30, 70, 32, BLACK);
        DrawText("Press any key to try again", 30, 110, 32, BLACK);

        DrawTexture(texture, screenWidth / 2 - texture.width / 2, screenHeight / 2 - texture.height / 2, RAYWHITE);

        DrawText("\nThanks for playing my game!!", 200, 700, 32, BLACK);
        DrawText("\nthis game was built by the hopes \nand dreams of an idiot :3", 200, 750, 32, BLACK);

        EndDrawing();
    }

} ;

int main() noexcept
{
    InitAudioDevice();
    InitWindow(screenWidth, screenHeight, "Snake!");
    Music music = LoadMusicStream("resources/mini1111.xm");

    STARTMENU_SCENE startmenu_scene;
    PLAY_SCENE play_scene;
    GAMEOVER_SCENE gameover_scene(play_scene.GAME);

    SetTargetFPS(30);

    PlayMusicStream(music);

    scene_table scenes{startmenu_scene, play_scene, gameover_scene};

    while (!WindowShouldClose())
    {
        UpdateMusicStream(music);
        
        scenes.draw();
        scenes.update();
    }

    UnloadMusicStream(music);
    
    CloseWindow();
    CloseAudioDevice();

    return 0;
}