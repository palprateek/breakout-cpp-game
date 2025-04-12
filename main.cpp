#include "raylib.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

//----------------------------------------------------------------------------------
// Defines and Global Constants
//----------------------------------------------------------------------------------
constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;

constexpr float PADDLE_WIDTH = 100.0f;
constexpr float PADDLE_HEIGHT = 20.0f;
constexpr float PADDLE_SPEED = 8.0f;

constexpr float BALL_RADIUS = 10.0f;
constexpr float INITIAL_BALL_SPEED_X = 4.0f;
constexpr float INITIAL_BALL_SPEED_Y = -4.0f;
constexpr float MIN_BALL_SPEED_X = 2.0f;

constexpr int BRICK_ROWS = 6;
constexpr int BRICKS_PER_ROW = 10;
constexpr float BRICK_WIDTH = static_cast<float>(SCREEN_WIDTH) / BRICKS_PER_ROW;
constexpr float BRICK_HEIGHT = 30.0f;
constexpr float BRICK_SPACING = 2.0f;

constexpr float TIME_LIMIT_EASY = 90.0f;   // 1.5 minutes
constexpr float TIME_LIMIT_MEDIUM = 120.0f; // 2 minutes
constexpr float TIME_LIMIT_HARD = 150.0f;   // 2.5 minutes

constexpr float POWERUP_SIZE = 20.0f;
constexpr float POWERUP_SPEED = 2.0f;
constexpr float POWERUP_SPAWN_CHANCE = 0.1f;

// Custom matte black color (#0F0F0F)
const Color MATTE_BLACK = { 15, 15, 15, 255 }; // RGB(15, 15, 15), fully opaque

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
struct Paddle {
    Rectangle rect;
    Color color;
};

struct Ball {
    Vector2 position;
    Vector2 speed;
    float radius;
    bool active;
    Color color;
};

struct Brick {
    Rectangle rect;
    bool active;
    int hitsRequired;
    float moveSpeed;
    Color color;
};

enum class PowerUpType {
    NONE,
    PADDLE_SIZE_UP,
    BALL_SPEED_UP,
    EXTRA_LIFE,
    MULTI_BALL
};

struct PowerUp {
    Rectangle rect;
    PowerUpType type;
    bool active;
    Color color;
};

enum class GameState {
    MENU,
    PLAYING,
    GAME_OVER,
    YOU_WIN
};

enum class Difficulty {
    EASY,
    MEDIUM,
    HARD
};

//------------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------------
static Paddle paddle = { 0 };
static std::vector<Ball> balls;
static Brick bricks[BRICK_ROWS][BRICKS_PER_ROW] = { 0 };
static std::vector<PowerUp> powerUps;
static int score = 0;
static int lives = 3;
static GameState currentState = GameState::MENU;
static bool paused = false;
static int activeBricks = 0;
static float countdownTimer = 0.0f;
static Difficulty currentDifficulty = Difficulty::EASY;
static int currentLevel = 1;
static int selectedMenuOption = 0;
static std::mt19937 rng(static_cast<unsigned int>(GetTime()));

//------------------------------------------------------------------------------------
// Module Functions Declaration
//------------------------------------------------------------------------------------
static void InitGame();
static void UpdateGame();
static void DrawGame();
static void UnloadGame();
static void UpdateDrawFrame();
static void ResetBallsAndPaddle();
static void SetupLevel(Difficulty diff);
static void UpdateMenu();
static void DrawMenu();
static void SpawnPowerUp(Vector2 position);
static void UpdatePowerUps();
static void ApplyPowerUp(PowerUpType type);
static bool HandleBrickCollision(Ball& ball);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Atari Breakout");
    SetTargetFPS(60);
    SetWindowState(FLAG_VSYNC_HINT);

    currentState = GameState::MENU;
    selectedMenuOption = 0;

    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }

    UnloadGame();
    CloseWindow();
    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definitions
//------------------------------------------------------------------------------------

void InitGame()
{
    score = 0;
    lives = 3;
    currentLevel = 1;
    paused = false;
    powerUps.clear();
    balls.clear();
    SetupLevel(currentDifficulty);
}

void SetupLevel(Difficulty diff)
{
    paddle.rect.width = (diff == Difficulty::EASY) ? PADDLE_WIDTH * 1.5f :
                        (diff == Difficulty::MEDIUM) ? PADDLE_WIDTH * 0.7f : PADDLE_WIDTH * 0.5f;
    paddle.rect.height = PADDLE_HEIGHT;
    paddle.rect.x = (SCREEN_WIDTH - paddle.rect.width) / 2.0f;
    paddle.rect.y = SCREEN_HEIGHT - paddle.rect.height - 30.0f;
    paddle.color = WHITE;

    ResetBallsAndPaddle();

    activeBricks = 0;
    const int initialOffsetY = 50;
    const int activeRows = (diff == Difficulty::EASY) ? BRICK_ROWS - 3 :
                          (diff == Difficulty::MEDIUM) ? BRICK_ROWS - 1 : BRICK_ROWS;

    std::vector<std::pair<int, int>> positions;
    for (int i = 0; i < activeRows; i++)
        for (int j = 0; j < BRICKS_PER_ROW; j++)
            positions.emplace_back(i, j);

    std::shuffle(positions.begin(), positions.end(), rng);
    const int maxBricks = positions.size();
    const int numBricks = maxBricks * (GetRandomValue(70, 90) / 100.0f);

    for (int i = 0; i < BRICK_ROWS; i++)
    {
        for (int j = 0; j < BRICKS_PER_ROW; j++)
        {
            bricks[i][j] = { 0 };
            bricks[i][j].rect.width = BRICK_WIDTH - BRICK_SPACING;
            bricks[i][j].rect.height = BRICK_HEIGHT - BRICK_SPACING;
            bricks[i][j].rect.x = j * BRICK_WIDTH + BRICK_SPACING / 2.0f;
            bricks[i][j].rect.y = initialOffsetY + i * BRICK_HEIGHT + BRICK_SPACING / 2.0f;
            bricks[i][j].active = false;
        }
    }

    for (int k = 0; k < numBricks && k < static_cast<int>(positions.size()); k++)
    {
        int i = positions[k].first;
        int j = positions[k].second;
        bricks[i][j].active = true;
        if (diff == Difficulty::EASY)
            bricks[i][j].hitsRequired = 1;
        else if (diff == Difficulty::MEDIUM)
            bricks[i][j].hitsRequired = GetRandomValue(1, 2);
        else
            bricks[i][j].hitsRequired = GetRandomValue(1, 3);
        bricks[i][j].moveSpeed = (diff == Difficulty::HARD && i == activeRows - 1 && GetRandomValue(0, 100) < 30) ? 2.0f : 0.0f;
        bricks[i][j].color = (i % 4 == 0) ? RED : (i % 4 == 1) ? ORANGE : (i % 4 == 2) ? YELLOW : GREEN;
        activeBricks++;
    }

    countdownTimer = (diff == Difficulty::EASY) ? TIME_LIMIT_EASY :
                     (diff == Difficulty::MEDIUM) ? TIME_LIMIT_MEDIUM : TIME_LIMIT_HARD;
}

void ResetBallsAndPaddle()
{
    paddle.rect.x = (SCREEN_WIDTH - paddle.rect.width) / 2.0f;
    paddle.rect.y = SCREEN_HEIGHT - paddle.rect.height - 30.0f;

    balls.clear();
    Ball newBall = { 0 };
    newBall.position = { paddle.rect.x + paddle.rect.width / 2.0f, paddle.rect.y - BALL_RADIUS - 5.0f };
    newBall.radius = BALL_RADIUS;
    newBall.color = WHITE;
    const float baseSpeedX = (currentDifficulty == Difficulty::EASY) ? INITIAL_BALL_SPEED_X * 0.8f :
                             (currentDifficulty == Difficulty::MEDIUM) ? INITIAL_BALL_SPEED_X * 1.2f : INITIAL_BALL_SPEED_X * 1.5f;
    newBall.speed.x = baseSpeedX * (GetRandomValue(0, 1) ? 1.0f : -1.0f);
    newBall.speed.y = (currentDifficulty == Difficulty::EASY) ? INITIAL_BALL_SPEED_Y * 0.8f :
                      (currentDifficulty == Difficulty::MEDIUM) ? INITIAL_BALL_SPEED_Y * 1.2f : INITIAL_BALL_SPEED_Y * 1.5f;
    newBall.active = true;
    balls.push_back(newBall);
}

void SpawnPowerUp(Vector2 position)
{
    if (GetRandomValue(0, 100) / 100.0f > POWERUP_SPAWN_CHANCE) return;

    PowerUp powerUp = { 0 };
    powerUp.rect = { position.x, position.y, POWERUP_SIZE, POWERUP_SIZE };
    powerUp.active = true;

    const int type = GetRandomValue(1, 4);
    powerUp.type = static_cast<PowerUpType>(type);
    switch (powerUp.type)
    {
        case PowerUpType::PADDLE_SIZE_UP: powerUp.color = SKYBLUE; break;
        case PowerUpType::BALL_SPEED_UP: powerUp.color = RED; break;
        case PowerUpType::EXTRA_LIFE: powerUp.color = GREEN; break;
        case PowerUpType::MULTI_BALL: powerUp.color = PURPLE; break;
        default: powerUp.color = WHITE; break;
    }

    powerUps.push_back(powerUp);
}

void UpdatePowerUps()
{
    for (size_t i = 0; i < powerUps.size();)
    {
        PowerUp& powerUp = powerUps[i];
        if (!powerUp.active)
        {
            powerUps.erase(powerUps.begin() + i);
            continue;
        }

        powerUp.rect.y += POWERUP_SPEED;

        if (CheckCollisionRecs(powerUp.rect, paddle.rect))
        {
            ApplyPowerUp(powerUp.type);
            powerUp.active = false;
        }
        else if (powerUp.rect.y > SCREEN_HEIGHT)
        {
            powerUp.active = false;
        }

        if (powerUp.active) ++i;
    }
}

void ApplyPowerUp(PowerUpType type)
{
    switch (type)
    {
        case PowerUpType::PADDLE_SIZE_UP:
            paddle.rect.width = std::min(paddle.rect.width * 1.2f, PADDLE_WIDTH * 2.0f);
            paddle.rect.x = std::max(0.0f, std::min(paddle.rect.x, static_cast<float>(SCREEN_WIDTH) - paddle.rect.width));
            break;
        case PowerUpType::BALL_SPEED_UP:
            for (auto& ball : balls)
            {
                ball.speed.x *= 1.2f;
                ball.speed.y *= 1.2f;
                if (std::abs(ball.speed.x) < MIN_BALL_SPEED_X)
                    ball.speed.x = (ball.speed.x >= 0 ? MIN_BALL_SPEED_X : -MIN_BALL_SPEED_X);
            }
            break;
        case PowerUpType::EXTRA_LIFE:
            lives++;
            break;
        case PowerUpType::MULTI_BALL:
            for (int i = 0; i < 2 && !balls.empty(); i++)
            {
                Ball newBall = balls[0];
                newBall.speed.x *= (i == 0 ? -1.0f : 1.0f);
                newBall.speed.y = -std::abs(newBall.speed.y);
                newBall.active = true;
                balls.push_back(newBall);
            }
            break;
        default:
            break;
    }
}

void UpdateMenu()
{
    if (IsKeyPressed(KEY_UP)) selectedMenuOption = (selectedMenuOption - 1 + 3) % 3;
    if (IsKeyPressed(KEY_DOWN)) selectedMenuOption = (selectedMenuOption + 1) % 3;

    if (IsKeyPressed(KEY_ENTER))
    {
        currentDifficulty = static_cast<Difficulty>(selectedMenuOption);
        InitGame();
        currentState = GameState::PLAYING;
    }
}

bool HandleBrickCollision(Ball& ball)
{
    for (int i = 0; i < BRICK_ROWS; i++)
    {
        for (int j = 0; j < BRICKS_PER_ROW; j++)
        {
            if (bricks[i][j].active && CheckCollisionCircleRec(ball.position, ball.radius, bricks[i][j].rect))
            {
                bricks[i][j].hitsRequired--;
                if (bricks[i][j].hitsRequired <= 0)
                {
                    bricks[i][j].active = false;
                    activeBricks--;
                    score += 10;
                    SpawnPowerUp({ bricks[i][j].rect.x + bricks[i][j].rect.width / 2, bricks[i][j].rect.y + bricks[i][j].rect.height / 2 });
                }
                else
                {
                    bricks[i][j].color = Fade(bricks[i][j].color, 0.8f);
                }

                const float dx = ball.position.x - (bricks[i][j].rect.x + bricks[i][j].rect.width / 2);
                const float dy = ball.position.y - (bricks[i][j].rect.y + bricks[i][j].rect.height / 2);
                if (std::abs(dx) > bricks[i][j].rect.width / 2 || std::abs(dy) > bricks[i][j].rect.height / 2)
                {
                    if (std::abs(dx) > std::abs(dy))
                        ball.speed.x *= -1;
                    else
                        ball.speed.y *= -1;
                }
                else
                {
                    ball.speed.y *= -1;
                }

                if (std::abs(ball.speed.x) < MIN_BALL_SPEED_X)
                    ball.speed.x = (ball.speed.x >= 0 ? MIN_BALL_SPEED_X : -MIN_BALL_SPEED_X);

                return true;
            }
        }
    }
    return false;
}

void UpdateGame()
{
    switch (currentState)
    {
        case GameState::MENU:
            UpdateMenu();
            break;

        case GameState::PLAYING:
        {
            if (IsKeyPressed(KEY_P)) paused = !paused;

            if (IsKeyPressed(KEY_B))
            {
                currentState = GameState::MENU;
                selectedMenuOption = 0;
                paused = false;
                powerUps.clear();
                return;
            }

            if (paused) return;

            countdownTimer -= GetFrameTime();
            if (countdownTimer <= 0.0f)
            {
                lives--;
                for (auto& ball : balls) ball.active = false;
                if (lives <= 0)
                {
                    currentState = GameState::GAME_OVER;
                }
                else
                {
                    ResetBallsAndPaddle();
                }
            }

            if (IsKeyDown(KEY_LEFT)) paddle.rect.x -= PADDLE_SPEED;
            if (IsKeyDown(KEY_RIGHT)) paddle.rect.x += PADDLE_SPEED;
            paddle.rect.x = std::max(0.0f, std::min(paddle.rect.x, static_cast<float>(SCREEN_WIDTH) - paddle.rect.width));

            bool anyBallActive = false;
            for (auto& ball : balls)
            {
                if (!ball.active) continue;

                anyBallActive = true;
                ball.position.x += ball.speed.x;
                ball.position.y += ball.speed.y;

                if (ball.position.x + ball.radius >= SCREEN_WIDTH || ball.position.x - ball.radius <= 0)
                    ball.speed.x *= -1;
                if (ball.position.y - ball.radius <= 0)
                    ball.speed.y *= -1;

                if (ball.position.y + ball.radius >= SCREEN_HEIGHT)
                {
                    ball.active = false;
                }

                if (CheckCollisionCircleRec(ball.position, ball.radius, paddle.rect) && ball.speed.y > 0)
                {
                    const float shiftAmount = 0.3f;
                    const float speedMagnitude = std::sqrt(ball.speed.x * ball.speed.x + ball.speed.y * ball.speed.y);
                    ball.speed.y = -std::abs(ball.speed.y);
                    const float hitPoint = (ball.position.x - paddle.rect.x) / paddle.rect.width;
                    float targetSpeedX;
                    if (hitPoint < (0.5f - shiftAmount))
                        targetSpeedX = -speedMagnitude * 0.6f;
                    else if (hitPoint > (0.5f + shiftAmount))
                        targetSpeedX = speedMagnitude * 0.6f;
                    else
                        targetSpeedX = (hitPoint - 0.5f) * 2.0f * speedMagnitude * 0.5f;
                    if (std::abs(targetSpeedX) < MIN_BALL_SPEED_X)
                        targetSpeedX = (targetSpeedX >= 0 ? MIN_BALL_SPEED_X : -MIN_BALL_SPEED_X) * (1.0f + GetRandomValue(-10, 10) / 100.0f);
                    ball.speed.x = targetSpeedX;
                    ball.position.y = paddle.rect.y - ball.radius - 0.1f;
                }

                HandleBrickCollision(ball);
            }

            if (!anyBallActive)
            {
                lives--;
                if (lives <= 0)
                {
                    currentState = GameState::GAME_OVER;
                }
                else
                {
                    ResetBallsAndPaddle();
                }
            }

            if (currentDifficulty == Difficulty::HARD)
            {
                for (int i = 0; i < BRICK_ROWS; i++)
                {
                    for (int j = 0; j < BRICKS_PER_ROW; j++)
                    {
                        if (bricks[i][j].active && bricks[i][j].moveSpeed != 0.0f)
                        {
                            bricks[i][j].rect.x += bricks[i][j].moveSpeed;
                            if (bricks[i][j].rect.x <= 0 || bricks[i][j].rect.x + bricks[i][j].rect.width >= SCREEN_WIDTH)
                                bricks[i][j].moveSpeed *= -1;
                        }
                    }
                }
            }

            UpdatePowerUps();

            if (activeBricks <= 0)
                currentState = GameState::YOU_WIN;
            break;
        }

        case GameState::GAME_OVER:
        case GameState::YOU_WIN:
            if (IsKeyPressed(KEY_ENTER))
            {
                if (currentState == GameState::YOU_WIN && currentDifficulty != Difficulty::HARD)
                {
                    currentLevel++;
                    currentDifficulty = static_cast<Difficulty>(static_cast<int>(currentDifficulty) + 1);
                    SetupLevel(currentDifficulty);
                    score += 100;
                    currentState = GameState::PLAYING;
                }
                else
                {
                    currentState = GameState::MENU;
                    selectedMenuOption = 0;
                }
                paused = false;
            }
            break;
    }
}

void DrawMenu()
{
    ClearBackground(MATTE_BLACK); // Use matte black for menu
    DrawText("ATARI BREAKOUT", SCREEN_WIDTH / 2 - MeasureText("ATARI BREAKOUT", 40) / 2, SCREEN_HEIGHT / 2 - 120, 40, WHITE);

    const Color easyColor = (selectedMenuOption == 0) ? YELLOW : GRAY;
    const Color mediumColor = (selectedMenuOption == 1) ? YELLOW : GRAY;
    const Color hardColor = (selectedMenuOption == 2) ? YELLOW : GRAY;

    DrawText("EASY", SCREEN_WIDTH / 2 - MeasureText("EASY", 30) / 2, SCREEN_HEIGHT / 2 - 20, 30, easyColor);
    DrawText("MEDIUM", SCREEN_WIDTH / 2 - MeasureText("MEDIUM", 30) / 2, SCREEN_HEIGHT / 2 + 20, 30, mediumColor);
    DrawText("HARD", SCREEN_WIDTH / 2 - MeasureText("HARD", 30) / 2, SCREEN_HEIGHT / 2 + 60, 30, hardColor);

    DrawText("Use UP/DOWN, ENTER to start", SCREEN_WIDTH / 2 - MeasureText("Use UP/DOWN, ENTER to start", 20) / 2, SCREEN_HEIGHT / 2 + 120, 20, GRAY);
}

void DrawGame()
{
    BeginDrawing();
    ClearBackground(MATTE_BLACK); // Use matte black for gameplay

    switch (currentState)
    {
        case GameState::MENU:
            DrawMenu();
            break;

        case GameState::PLAYING:
        case GameState::GAME_OVER:
        case GameState::YOU_WIN:
        {
            DrawRectangleRec(paddle.rect, paddle.color);

            for (int i = 0; i < BRICK_ROWS; i++)
            {
                for (int j = 0; j < BRICKS_PER_ROW; j++)
                {
                    if (bricks[i][j].active)
                    {
                        DrawRectangleRec(bricks[i][j].rect, bricks[i][j].color);
                        if (bricks[i][j].hitsRequired > 1)
                            DrawText(TextFormat("%i", bricks[i][j].hitsRequired),
                                     bricks[i][j].rect.x + bricks[i][j].rect.width / 2 - 5,
                                     bricks[i][j].rect.y + 5, 20, WHITE);
                    }
                }
            }

            for (const auto& powerUp : powerUps)
            {
                if (powerUp.active)
                {
                    DrawRectangleRec(powerUp.rect, powerUp.color);
                    const char* label = nullptr;
                    switch (powerUp.type)
                    {
                        case PowerUpType::PADDLE_SIZE_UP: label = "P"; break;
                        case PowerUpType::BALL_SPEED_UP: label = "S"; break;
                        case PowerUpType::EXTRA_LIFE: label = "L"; break;
                        case PowerUpType::MULTI_BALL: label = "M"; break;
                        default: break;
                    }
                    if (label)
                        DrawText(label, powerUp.rect.x + 5, powerUp.rect.y + 5, 10, WHITE);
                }
            }

            for (const auto& ball : balls)
                if (ball.active)
                    DrawCircleV(ball.position, ball.radius, ball.color);

            DrawText(TextFormat("SCORE: %04i", score), 10, 10, 20, WHITE);
            DrawText(TextFormat("LIVES: %i", lives), SCREEN_WIDTH - 100, 10, 20, WHITE);
            const int minutes = static_cast<int>(countdownTimer / 60);
            const int seconds = static_cast<int>(countdownTimer) % 60;
            DrawText(TextFormat("TIME: %02i:%02i", minutes, seconds), SCREEN_WIDTH / 2 - 50, 10, 20, countdownTimer <= 10.0f ? RED : WHITE);
            const char* diffText = (currentDifficulty == Difficulty::EASY) ? "EASY" :
                                  (currentDifficulty == Difficulty::MEDIUM) ? "MEDIUM" : "HARD";
            DrawText(TextFormat("LEVEL: %i (%s)", currentLevel, diffText), 10, 40, 20, WHITE);
            DrawText("Press [B] for MENU", 10, SCREEN_HEIGHT - 30, 20, GRAY);

            if (paused && currentState == GameState::PLAYING)
                DrawText("PAUSED", SCREEN_WIDTH / 2 - MeasureText("PAUSED", 40) / 2, SCREEN_HEIGHT / 2 - 20, 40, GRAY);
            break;
        }
    }

    if (currentState == GameState::GAME_OVER)
    {
        DrawRectangle(0, SCREEN_HEIGHT / 2 - 40, SCREEN_WIDTH, 80, Fade(MATTE_BLACK, 0.7f)); // Overlay with matte black
        DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, SCREEN_HEIGHT / 2 - 20, 40, RED);
        DrawText("Press [ENTER] to MENU", SCREEN_WIDTH / 2 - MeasureText("Press [ENTER] to MENU", 20) / 2, SCREEN_HEIGHT / 2 + 25, 20, WHITE);
    }
    else if (currentState == GameState::YOU_WIN)
    {
        DrawRectangle(0, SCREEN_HEIGHT / 2 - 40, SCREEN_WIDTH, 80, Fade(MATTE_BLACK, 0.7f)); // Overlay with matte black
        DrawText("YOU WIN!", SCREEN_WIDTH / 2 - MeasureText("YOU WIN!", 40) / 2, SCREEN_HEIGHT / 2 - 20, 40, GREEN);
        const char* nextText = (currentDifficulty != Difficulty::HARD) ? "Press [ENTER] for NEXT LEVEL" : "Press [ENTER] to MENU";
        DrawText(nextText, SCREEN_WIDTH / 2 - MeasureText(nextText, 20) / 2, SCREEN_HEIGHT / 2 + 25, 20, WHITE);
    }

    EndDrawing();
}

void UnloadGame()
{
    powerUps.clear();
    balls.clear();
}

void UpdateDrawFrame()
{
    UpdateGame();
    DrawGame();
}