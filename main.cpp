#include "raylib.h"
#include <cmath>
#include <iostream>
#include <vector>

//----------------------------------------------------------------------------------
// Defines and Global Constants
//----------------------------------------------------------------------------------
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define PADDLE_WIDTH 100
#define PADDLE_HEIGHT 20
#define PADDLE_SPEED 8.0f

#define BALL_RADIUS 10.0f
#define INITIAL_BALL_SPEED_X 4.0f
#define INITIAL_BALL_SPEED_Y -4.0f
#define MIN_BALL_SPEED_X 2.0f

#define BRICK_ROWS 6 // Increased for Hard mode
#define BRICKS_PER_ROW 10
#define BRICK_WIDTH (SCREEN_WIDTH / BRICKS_PER_ROW)
#define BRICK_HEIGHT 30
#define BRICK_SPACING 1

#define TIME_LIMIT_EASY 120.0f
#define TIME_LIMIT_MEDIUM 90.0f
#define TIME_LIMIT_HARD 60.0f

#define POWERUP_SIZE 20.0f
#define POWERUP_SPEED 2.0f
#define POWERUP_SPAWN_CHANCE 0.1f // 10% chance per brick

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Paddle {
  Rectangle rect;
  Color color;
} Paddle;

typedef struct Ball {
  Vector2 position;
  Vector2 speed;
  float radius;
  bool active;
  Color color;
} Ball;

typedef struct Brick {
  Rectangle rect;
  bool active;
  int hitsRequired; // For multi-hit bricks
  float moveSpeed;  // For moving bricks
  Color color;
} Brick;

typedef enum PowerUpType {
  NONE,
  PADDLE_SIZE_UP,
  BALL_SPEED_UP,
  EXTRA_LIFE
} PowerUpType;

typedef struct PowerUp {
  Rectangle rect;
  PowerUpType type;
  bool active;
  Color color;
} PowerUp;

typedef enum GameState { MENU, PLAYING, GAME_OVER, YOU_WIN } GameState;

typedef enum Difficulty { EASY, MEDIUM, HARD } Difficulty;

//------------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------------
static Paddle paddle = {0};
static Ball ball = {0};
static Brick bricks[BRICK_ROWS][BRICKS_PER_ROW] = {0};
static std::vector<PowerUp> powerUps;
static int score = 0;
static int lives = 3;
static GameState currentState = MENU;
static bool paused = false;
static int activeBricks = BRICK_ROWS * BRICKS_PER_ROW;
static float countdownTimer = 0.0f;
static Difficulty currentDifficulty = EASY;
static int currentLevel = 1;
static int selectedMenuOption = 0;

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);
static void UpdateGame(void);
static void DrawGame(void);
static void UnloadGame(void);
static void UpdateDrawFrame(void);
static void ResetBallAndPaddle(void);
static void SetupLevel(Difficulty diff);
static void UpdateMenu(void);
static void DrawMenu(void);
static void SpawnPowerUp(Vector2 position);
static void UpdatePowerUps(void);
static void ApplyPowerUp(PowerUpType type);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Breakout");
  SetTargetFPS(60);
  SetWindowState(FLAG_VSYNC_HINT);
  InitRandomSeed((unsigned int)GetTime()); // Initialize random seed

  currentState = MENU;
  selectedMenuOption = 0;

  while (!WindowShouldClose())
    UpdateDrawFrame();

  UnloadGame();
  CloseWindow();

  return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//------------------------------------------------------------------------------------

void InitGame(void) {
  score = 0;
  lives = 3;
  currentLevel = 1;
  paused = false;
  powerUps.clear();
  SetupLevel(currentDifficulty);
}

void SetupLevel(Difficulty diff) {
  paddle.rect.width = (diff == EASY) ? PADDLE_WIDTH * 1.5f
                      : (diff == MEDIUM)
                          ? PADDLE_WIDTH * 0.7f
                          : PADDLE_WIDTH * 0.5f; // Hard: smaller paddle
  paddle.rect.height = PADDLE_HEIGHT;
  paddle.rect.x = (SCREEN_WIDTH - paddle.rect.width) / 2.0f;
  paddle.rect.y = SCREEN_HEIGHT - paddle.rect.height - 30.0f;
  paddle.color = BLUE;

  ResetBallAndPaddle();
  ball.radius = BALL_RADIUS;
  ball.color = MAROON;
  ball.speed.x = (diff == EASY) ? INITIAL_BALL_SPEED_X * 0.8f
                 : (diff == MEDIUM)
                     ? INITIAL_BALL_SPEED_X * 1.2f
                     : INITIAL_BALL_SPEED_X * 1.5f; // Hard: faster
  ball.speed.y = (diff == EASY)     ? INITIAL_BALL_SPEED_Y * 0.8f
                 : (diff == MEDIUM) ? INITIAL_BALL_SPEED_Y * 1.2f
                                    : INITIAL_BALL_SPEED_Y * 1.5f;

  activeBricks = 0;
  int initialOffsetY = 50;
  int activeRows = (diff == EASY)     ? BRICK_ROWS - 3
                   : (diff == MEDIUM) ? BRICK_ROWS - 1
                                      : BRICK_ROWS;

  for (int i = 0; i < BRICK_ROWS; i++) {
    for (int j = 0; j < BRICKS_PER_ROW; j++) {
      bricks[i][j].rect.width = BRICK_WIDTH - BRICK_SPACING;
      bricks[i][j].rect.height = BRICK_HEIGHT - BRICK_SPACING;
      bricks[i][j].rect.x = j * BRICK_WIDTH + BRICK_SPACING / 2.0f;
      bricks[i][j].rect.y =
          initialOffsetY + i * BRICK_HEIGHT + BRICK_SPACING / 2.0f;
      bricks[i][j].active = (i < activeRows);
      bricks[i][j].hitsRequired =
          (diff == HARD && i < 2 && GetRandomValue(0, 100) < 50)
              ? 2
              : 1; // Top 2 rows in Hard may need 2 hits
      bricks[i][j].moveSpeed =
          (diff == HARD && i == activeRows - 1 && GetRandomValue(0, 100) < 30)
              ? 2.0f
              : 0.0f; // Last row may move
      if (bricks[i][j].active)
        activeBricks++;
      if (diff == EASY)
        bricks[i][j].color = YELLOW;
      else if (diff == MEDIUM)
        bricks[i][j].color = GREEN;
      else
        bricks[i][j].color = (bricks[i][j].hitsRequired > 1) ? RED : ORANGE;
    }
  }

  countdownTimer = (diff == EASY)     ? TIME_LIMIT_EASY
                   : (diff == MEDIUM) ? TIME_LIMIT_MEDIUM
                                      : TIME_LIMIT_HARD;
}

void ResetBallAndPaddle(void) {
  paddle.rect.x = (SCREEN_WIDTH - paddle.rect.width) / 2.0f;
  paddle.rect.y = SCREEN_HEIGHT - paddle.rect.height - 30.0f;

  ball.position = {paddle.rect.x + paddle.rect.width / 2.0f,
                   paddle.rect.y - BALL_RADIUS - 5.0f};
  float baseSpeedX = (currentDifficulty == EASY) ? INITIAL_BALL_SPEED_X * 0.8f
                     : (currentDifficulty == MEDIUM)
                         ? INITIAL_BALL_SPEED_X * 1.2f
                         : INITIAL_BALL_SPEED_X * 1.5f;
  ball.speed.x = baseSpeedX * (GetRandomValue(0, 1) ? 1.0f : -1.0f);
  ball.speed.y = (currentDifficulty == EASY)     ? INITIAL_BALL_SPEED_Y * 0.8f
                 : (currentDifficulty == MEDIUM) ? INITIAL_BALL_SPEED_Y * 1.2f
                                                 : INITIAL_BALL_SPEED_Y * 1.5f;
  ball.active = true;
}

void SpawnPowerUp(Vector2 position) {
  if (GetRandomValue(0, 100) / 100.0f > POWERUP_SPAWN_CHANCE)
    return;

  PowerUp powerUp = {0};
  powerUp.rect = {position.x, position.y, POWERUP_SIZE, POWERUP_SIZE};
  powerUp.active = true;

  int type = GetRandomValue(1, 3);
  powerUp.type = (PowerUpType)type;
  switch (powerUp.type) {
  case PADDLE_SIZE_UP:
    powerUp.color = SKYBLUE;
    break;
  case BALL_SPEED_UP:
    powerUp.color = RED;
    break;
  case EXTRA_LIFE:
    powerUp.color = GREEN;
    break;
  default:
    powerUp.color = WHITE;
    break;
  }

  powerUps.push_back(powerUp);
}

void UpdatePowerUps(void) {
  for (auto it = powerUps.begin(); it != powerUps.end();) {
    if (!it->active) {
      it = powerUps.erase(it);
      continue;
    }

    it->rect.y += POWERUP_SPEED;

    if (CheckCollisionRecs(it->rect, paddle.rect)) {
      ApplyPowerUp(it->type);
      it->active = false;
    } else if (it->rect.y > SCREEN_HEIGHT) {
      it->active = false;
    }

    if (it != powerUps.end())
      ++it;
  }
}

void ApplyPowerUp(PowerUpType type) {
  switch (type) {
  case PADDLE_SIZE_UP:
    paddle.rect.width = std::min(paddle.rect.width * 1.2f, PADDLE_WIDTH * 2.0f);
    paddle.rect.x = std::max(
        0.0f, std::min(paddle.rect.x, SCREEN_WIDTH - paddle.rect.width));
    break;
  case BALL_SPEED_UP:
    ball.speed.x *= 1.2f;
    ball.speed.y *= 1.2f;
    if (fabsf(ball.speed.x) < MIN_BALL_SPEED_X)
      ball.speed.x = (ball.speed.x >= 0 ? MIN_BALL_SPEED_X : -MIN_BALL_SPEED_X);
    break;
  case EXTRA_LIFE:
    lives++;
    break;
  default:
    break;
  }
}

void UpdateMenu(void) {
  if (IsKeyPressed(KEY_UP))
    selectedMenuOption = (selectedMenuOption - 1 + 3) % 3;
  if (IsKeyPressed(KEY_DOWN))
    selectedMenuOption = (selectedMenuOption + 1) % 3;

  if (IsKeyPressed(KEY_ENTER)) {
    currentDifficulty = (Difficulty)selectedMenuOption;
    InitGame();
    currentState = PLAYING;
  }
}

void UpdateGame(void) {
  if (currentState == MENU) {
    UpdateMenu();
    return;
  }

  if (IsKeyPressed(KEY_P))
    paused = !paused;

  if (currentState == GAME_OVER || currentState == YOU_WIN) {
    if (IsKeyPressed(KEY_ENTER)) {
      if (currentState == YOU_WIN && currentDifficulty < HARD) {
        currentLevel++;
        currentDifficulty = (Difficulty)(currentDifficulty + 1);
        SetupLevel(currentDifficulty);
        score += 100;
      } else {
        currentState = MENU;
        selectedMenuOption = 0;
      }
      paused = false;
    }
    return;
  }

  if (paused || currentState != PLAYING)
    return;

  countdownTimer -= GetFrameTime();
  if (countdownTimer <= 0.0f) {
    lives--;
    countdownTimer = 0.0f;
    ball.active = false;
    if (lives <= 0) {
      currentState = GAME_OVER;
    } else {
      ResetBallAndPaddle();
      countdownTimer = (currentDifficulty == EASY)     ? TIME_LIMIT_EASY
                       : (currentDifficulty == MEDIUM) ? TIME_LIMIT_MEDIUM
                                                       : TIME_LIMIT_HARD;
    }
  }

  if (IsKeyDown(KEY_LEFT))
    paddle.rect.x -= PADDLE_SPEED;
  if (IsKeyDown(KEY_RIGHT))
    paddle.rect.x += PADDLE_SPEED;

  if (paddle.rect.x < 0)
    paddle.rect.x = 0;
  if (paddle.rect.x + paddle.rect.width > SCREEN_WIDTH)
    paddle.rect.x = SCREEN_WIDTH - paddle.rect.width;

  if (ball.active) {
    ball.position.x += ball.speed.x;
    ball.position.y += ball.speed.y;
  }

  // Update moving bricks (Hard mode)
  if (currentDifficulty == HARD) {
    for (int i = 0; i < BRICK_ROWS; i++) {
      for (int j = 0; j < BRICKS_PER_ROW; j++) {
        if (bricks[i][j].active && bricks[i][j].moveSpeed != 0.0f) {
          bricks[i][j].rect.x += bricks[i][j].moveSpeed;
          if (bricks[i][j].rect.x <= 0 ||
              bricks[i][j].rect.x + bricks[i][j].rect.width >= SCREEN_WIDTH) {
            bricks[i][j].moveSpeed *= -1;
          }
        }
      }
    }
  }

  UpdatePowerUps();

  if (ball.position.x + ball.radius >= SCREEN_WIDTH ||
      ball.position.x - ball.radius <= 0) {
    ball.speed.x *= -1;
  }
  if (ball.position.y - ball.radius <= 0)
    ball.speed.y *= -1;
  if (ball.position.y + ball.radius >= SCREEN_HEIGHT) {
    lives--;
    ball.active = false;
    if (lives <= 0) {
      currentState = GAME_OVER;
    } else {
      ResetBallAndPaddle();
      countdownTimer = (currentDifficulty == EASY)     ? TIME_LIMIT_EASY
                       : (currentDifficulty == MEDIUM) ? TIME_LIMIT_MEDIUM
                                                       : TIME_LIMIT_HARD;
    }
  }

  if (CheckCollisionCircleRec(ball.position, ball.radius, paddle.rect)) {
    if (ball.speed.y > 0) {
      float shiftAmount = 0.3f;
      float speedMagnitude =
          sqrtf(ball.speed.x * ball.speed.x + ball.speed.y * ball.speed.y);
      ball.speed.y = -fabsf(ball.speed.y);
      float hitPoint = (ball.position.x - paddle.rect.x) / paddle.rect.width;
      float targetSpeedX;
      if (hitPoint < (0.5f - shiftAmount))
        targetSpeedX = -speedMagnitude * 0.6f;
      else if (hitPoint > (0.5f + shiftAmount))
        targetSpeedX = speedMagnitude * 0.6f;
      else
        targetSpeedX = (hitPoint - 0.5f) * 2.0f * speedMagnitude * 0.5f;
      if (fabsf(targetSpeedX) < MIN_BALL_SPEED_X) {
        targetSpeedX =
            (targetSpeedX >= 0 ? MIN_BALL_SPEED_X : -MIN_BALL_SPEED_X) *
            (1.0f + GetRandomValue(-10, 10) / 100.0f);
      }
      ball.speed.x = targetSpeedX;
      ball.position.y = paddle.rect.y - ball.radius - 0.1f;
    }
  }

  for (int i = 0; i < BRICK_ROWS; i++) {
    for (int j = 0; j < BRICKS_PER_ROW; j++) {
      if (bricks[i][j].active) {
        if (CheckCollisionCircleRec(ball.position, ball.radius,
                                    bricks[i][j].rect)) {
          bricks[i][j].hitsRequired--;
          if (bricks[i][j].hitsRequired <= 0) {
            bricks[i][j].active = false;
            activeBricks--;
            score += 10;
            SpawnPowerUp({bricks[i][j].rect.x + bricks[i][j].rect.width / 2,
                          bricks[i][j].rect.y + bricks[i][j].rect.height / 2});
          } else {
            bricks[i][j].color =
                Fade(bricks[i][j].color, 0.7f); // Dim color to show damage
          }
          float dx = ball.position.x -
                     (bricks[i][j].rect.x + bricks[i][j].rect.width / 2);
          float dy = ball.position.y -
                     (bricks[i][j].rect.y + bricks[i][j].rect.height / 2);
          if (fabsf(dx) > bricks[i][j].rect.width / 2 ||
              fabsf(dy) > bricks[i][j].rect.height / 2) {
            if (fabsf(dx) > fabsf(dy))
              ball.speed.x *= -1;
            else
              ball.speed.y *= -1;
          } else {
            ball.speed.y *= -1;
          }
          if (fabsf(ball.speed.x) < MIN_BALL_SPEED_X) {
            ball.speed.x =
                (ball.speed.x >= 0 ? MIN_BALL_SPEED_X : -MIN_BALL_SPEED_X);
          }
          if (activeBricks <= 0)
            currentState = YOU_WIN;
          goto brick_collision_handled;
        }
      }
    }
  }
brick_collision_handled:;
}

void DrawMenu(void) {
  ClearBackground(RAYWHITE);
  DrawText("Select Difficulty",
           SCREEN_WIDTH / 2 - MeasureText("Select Difficulty", 40) / 2,
           SCREEN_HEIGHT / 2 - 100, 40, DARKGRAY);

  Color easyColor = (selectedMenuOption == 0) ? SKYBLUE : LIGHTGRAY;
  Color mediumColor = (selectedMenuOption == 1) ? SKYBLUE : LIGHTGRAY;
  Color hardColor = (selectedMenuOption == 2) ? SKYBLUE : LIGHTGRAY;

  DrawText("EASY", SCREEN_WIDTH / 2 - MeasureText("EASY", 30) / 2,
           SCREEN_HEIGHT / 2 - 20, 30, easyColor);
  DrawText("MEDIUM", SCREEN_WIDTH / 2 - MeasureText("MEDIUM", 30) / 2,
           SCREEN_HEIGHT / 2 + 20, 30, mediumColor);
  DrawText("HARD", SCREEN_WIDTH / 2 - MeasureText("HARD", 30) / 2,
           SCREEN_HEIGHT / 2 + 60, 30, hardColor);

  DrawText("Use UP/DOWN to select, ENTER to start",
           SCREEN_WIDTH / 2 -
               MeasureText("Use UP/DOWN to select, ENTER to start", 20) / 2,
           SCREEN_HEIGHT / 2 + 120, 20, DARKGRAY);
}

void DrawGame(void) {
  BeginDrawing();
  ClearBackground(RAYWHITE);

  if (currentState == MENU) {
    DrawMenu();
  } else if (currentState == PLAYING || currentState == GAME_OVER ||
             currentState == YOU_WIN) {
    DrawRectangleRec(paddle.rect, paddle.color);

    for (int i = 0; i < BRICK_ROWS; i++) {
      for (int j = 0; j < BRICKS_PER_ROW; j++) {
        if (bricks[i][j].active) {
          DrawRectangleRec(bricks[i][j].rect, bricks[i][j].color);
          if (bricks[i][j].hitsRequired > 1) {
            DrawText("2", bricks[i][j].rect.x + bricks[i][j].rect.width / 2 - 5,
                     bricks[i][j].rect.y + 5, 20, WHITE);
          }
        }
      }
    }

    for (const auto &powerUp : powerUps) {
      if (powerUp.active) {
        DrawRectangleRec(powerUp.rect, powerUp.color);
        switch (powerUp.type) {
        case PADDLE_SIZE_UP:
          DrawText("P", powerUp.rect.x + 5, powerUp.rect.y + 5, 10, WHITE);
          break;
        case BALL_SPEED_UP:
          DrawText("S", powerUp.rect.x + 5, powerUp.rect.y + 5, 10, WHITE);
          break;
        case EXTRA_LIFE:
          DrawText("L", powerUp.rect.x + 5, powerUp.rect.y + 5, 10, WHITE);
          break;
        default:
          break;
        }
      }
    }

    if (ball.active || currentState == PLAYING)
      DrawCircleV(ball.position, ball.radius, ball.color);

    DrawText(TextFormat("SCORE: %04i", score), 10, 10, 20, DARKGRAY);
    DrawText(TextFormat("LIVES: %i", lives), SCREEN_WIDTH - 100, 10, 20,
             DARKGRAY);
    int minutes = (int)(countdownTimer / 60);
    int seconds = (int)(countdownTimer) % 60;
    DrawText(TextFormat("TIME: %02i:%02i", minutes, seconds),
             SCREEN_WIDTH / 2 - 50, 10, 20,
             countdownTimer <= 10.0f ? RED : DARKGRAY);
    const char *diffText = (currentDifficulty == EASY)     ? "EASY"
                           : (currentDifficulty == MEDIUM) ? "MEDIUM"
                                                           : "HARD";
    DrawText(TextFormat("LEVEL: %i (%s)", currentLevel, diffText), 10, 40, 20,
             DARKGRAY);

    if (paused && currentState == PLAYING) {
      DrawText("PAUSED", SCREEN_WIDTH / 2 - MeasureText("PAUSED", 40) / 2,
               SCREEN_HEIGHT / 2 - 20, 40, GRAY);
    }
  }

  if (currentState == GAME_OVER) {
    DrawRectangle(0, SCREEN_HEIGHT / 2 - 40, SCREEN_WIDTH, 80,
                  Fade(BLACK, 0.7f));
    DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2,
             SCREEN_HEIGHT / 2 - 20, 40, RED);
    DrawText("Press [ENTER] to MENU",
             SCREEN_WIDTH / 2 - MeasureText("Press [ENTER] to MENU", 20) / 2,
             SCREEN_HEIGHT / 2 + 25, 20, LIGHTGRAY);
  } else if (currentState == YOU_WIN) {
    DrawRectangle(0, SCREEN_HEIGHT / 2 - 40, SCREEN_WIDTH, 80,
                  Fade(BLACK, 0.7f));
    DrawText("YOU WIN!", SCREEN_WIDTH / 2 - MeasureText("YOU WIN!", 40) / 2,
             SCREEN_HEIGHT / 2 - 20, 40, GREEN);
    const char *nextText = (currentDifficulty < HARD)
                               ? "Press [ENTER] for NEXT LEVEL"
                               : "Press [ENTER] to MENU";
    DrawText(nextText, SCREEN_WIDTH / 2 - MeasureText(nextText, 20) / 2,
             SCREEN_HEIGHT / 2 + 25, 20, LIGHTGRAY);
  }

  EndDrawing();
}

void UnloadGame(void) { powerUps.clear(); }

void UpdateDrawFrame(void) {
  UpdateGame();
  DrawGame();
}
