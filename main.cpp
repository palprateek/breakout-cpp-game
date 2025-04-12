#include "raylib.h"
#include <iostream> // For std::cout, std::cerr (optional, for debugging)
#include <vector>   // For storing bricks (though we use a C-style array here for simplicity)
#include <cmath>    // For fabsf

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
#define INITIAL_BALL_SPEED_Y -4.0f // Start moving upwards

#define BRICK_ROWS 5
#define BRICKS_PER_ROW 10
#define BRICK_WIDTH (SCREEN_WIDTH / BRICKS_PER_ROW)
#define BRICK_HEIGHT 30
#define BRICK_SPACING 1 // Small gap between bricks

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
    Color color;
} Brick;

typedef enum GameState {
    PLAYING,
    GAME_OVER,
    YOU_WIN
} GameState;

//------------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------------
static Paddle paddle = { 0 };
static Ball ball = { 0 };
static Brick bricks[BRICK_ROWS][BRICKS_PER_ROW] = { 0 };
static int score = 0;
static int lives = 3;
static GameState currentState = PLAYING;
static bool paused = false;
static int activeBricks = BRICK_ROWS * BRICKS_PER_ROW;

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);         // Initialize game
static void UpdateGame(void);       // Update game (one frame)
static void DrawGame(void);         // Draw game (one frame)
static void UnloadGame(void);       // Unload game resources (if any needed beyond CloseWindow)
static void UpdateDrawFrame(void); // Update and Draw (combines Update and Draw)
static void ResetBallAndPaddle(void); // Helper to reset ball/paddle state

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Breakout");
    SetTargetFPS(60); // Set our game to run at 60 frames-per-second

    InitGame();
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadGame(); // Unload any game specific resources if needed
    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//------------------------------------------------------------------------------------

// Initialize game variables
void InitGame(void)
{
    // Initialize Paddle
    paddle.rect.width = PADDLE_WIDTH;
    paddle.rect.height = PADDLE_HEIGHT;
    paddle.rect.x = (SCREEN_WIDTH - paddle.rect.width) / 2.0f;
    paddle.rect.y = SCREEN_HEIGHT - paddle.rect.height - 30.0f; // Position near bottom
    paddle.color = BLUE;

    // Initialize Ball
    ResetBallAndPaddle(); // Set initial ball state
    ball.radius = BALL_RADIUS;
    ball.color = MAROON;

    // Initialize Bricks
    activeBricks = 0; // Reset count before initializing
    int initialOffsetY = 50; // Starting Y position for the first row of bricks

    for (int i = 0; i < BRICK_ROWS; i++)
    {
        for (int j = 0; j < BRICKS_PER_ROW; j++)
        {
            bricks[i][j].rect.width = BRICK_WIDTH - BRICK_SPACING;
            bricks[i][j].rect.height = BRICK_HEIGHT - BRICK_SPACING;
            bricks[i][j].rect.x = j * BRICK_WIDTH + BRICK_SPACING / 2.0f;
            bricks[i][j].rect.y = initialOffsetY + i * BRICK_HEIGHT + BRICK_SPACING / 2.0f;
            bricks[i][j].active = true;
            activeBricks++; // Increment count for each active brick

            // Simple color pattern based on row
            if ((i + j) % 2 == 0) bricks[i][j].color = GRAY;
            else bricks[i][j].color = DARKGRAY;
             // Add more colors based on row if desired
            if (i < 2) bricks[i][j].color = YELLOW;
            else if (i < 4) bricks[i][j].color = GREEN;
            else bricks[i][j].color = ORANGE;

        }
    }

    score = 0;
    lives = 3;
    currentState = PLAYING;
    paused = false;
}

// Reset ball and paddle to starting positions
void ResetBallAndPaddle(void)
{
    paddle.rect.x = (SCREEN_WIDTH - paddle.rect.width) / 2.0f;
    paddle.rect.y = SCREEN_HEIGHT - paddle.rect.height - 30.0f;

    ball.position = { paddle.rect.x + paddle.rect.width / 2.0f, paddle.rect.y - BALL_RADIUS - 5.0f };
    ball.speed = { INITIAL_BALL_SPEED_X, INITIAL_BALL_SPEED_Y };
    ball.active = true; // Make the ball ready to move
}


// Update game (one frame)
void UpdateGame(void)
{
    // Toggle pause (always allow toggling)
    if (IsKeyPressed(KEY_P)) paused = !paused;

    // --- Handle Game Over / Win states ---
    // Check for restart input FIRST, regardless of pause state
    if (currentState == GAME_OVER || currentState == YOU_WIN)
    {
        if (IsKeyPressed(KEY_ENTER))
        {
            InitGame(); // Restart the game
            currentState = PLAYING; // Explicitly set state back to playing
            paused = false;      // Ensure game isn't paused on restart
        }
        // If game is over/won, don't run the rest of the game logic for this frame
        return;
    }

    // --- Main Gameplay Logic ---
    // Only run the following if the game is currently PLAYING and not paused
    if (paused || currentState != PLAYING) {
        return; // Exit if paused or not in PLAYING state (redundant after above check, but safe)
    }


    // --- Paddle Movement --- (Runs only if PLAYING and not paused)
    if (IsKeyDown(KEY_LEFT)) paddle.rect.x -= PADDLE_SPEED;
    if (IsKeyDown(KEY_RIGHT)) paddle.rect.x += PADDLE_SPEED;

    // Keep paddle within screen bounds
    if (paddle.rect.x < 0) paddle.rect.x = 0;
    if (paddle.rect.x + paddle.rect.width > SCREEN_WIDTH) paddle.rect.x = SCREEN_WIDTH - paddle.rect.width;

    // --- Ball Movement --- (Runs only if PLAYING and not paused)
    if (ball.active)
    {
        ball.position.x += ball.speed.x;
        ball.position.y += ball.speed.y;
    }
    // ... (rest of ball related logic like launching if needed)


    // --- Collision Detection: Ball vs Walls --- (Runs only if PLAYING and not paused)
    // Left/Right walls
    if (ball.position.x + ball.radius >= SCREEN_WIDTH || ball.position.x - ball.radius <= 0)
    {
        ball.speed.x *= -1;
    }
    // Top wall
    if (ball.position.y - ball.radius <= 0)
    {
        ball.speed.y *= -1;
    }
    // Bottom wall (lose life)
    if (ball.position.y + ball.radius >= SCREEN_HEIGHT)
    {
        lives--;
        ball.active = false; // Temporarily deactivate ball

        if (lives <= 0)
        {
            currentState = GAME_OVER;
            // Don't immediately return here, let the state change be processed
            // The return at the top of the function will handle skipping logic next frame
        }
        else
        {
            ResetBallAndPaddle(); // Reset for the next life
        }
    }

    // --- Collision Detection: Ball vs Paddle --- (Runs only if PLAYING and not paused)
    if (CheckCollisionCircleRec(ball.position, ball.radius, paddle.rect))
    {
         if (ball.speed.y > 0) // Only bounce if ball is moving downwards
         {
             ball.speed.y *= -1;
             float hitPoint = (ball.position.x - paddle.rect.x) / paddle.rect.width; // 0 to 1
             if (hitPoint < 0.4f) ball.speed.x = -fabsf(INITIAL_BALL_SPEED_X) * 1.2f; // Hit left side
             else if (hitPoint > 0.6f) ball.speed.x = fabsf(INITIAL_BALL_SPEED_X) * 1.2f; // Hit right side
             else ball.speed.x = (hitPoint - 0.5f) * 2.0f * INITIAL_BALL_SPEED_X; // Hit middle
             ball.position.y = paddle.rect.y - ball.radius - 0.1f; // Prevent sticking
         }
    }

    // --- Collision Detection: Ball vs Bricks --- (Runs only if PLAYING and not paused)
    for (int i = 0; i < BRICK_ROWS; i++)
    {
        for (int j = 0; j < BRICKS_PER_ROW; j++)
        {
            if (bricks[i][j].active)
            {
                if (CheckCollisionCircleRec(ball.position, ball.radius, bricks[i][j].rect))
                {
                    bricks[i][j].active = false;
                    activeBricks--;
                    score += 10;
                    ball.speed.y *= -1;

                    if (activeBricks <= 0)
                    {
                        currentState = YOU_WIN;
                        // Don't immediately return, let the state change be processed
                    }
                    goto brick_collision_handled; // Exit loops once a collision is handled
                }
            }
        }
    }
    brick_collision_handled:;

} // End of UpdateGame

// Draw game (one frame)
void DrawGame(void)
{
    BeginDrawing();
    ClearBackground(RAYWHITE); // Or DARKBLUE, BLACK etc.

    if (currentState == PLAYING || currentState == GAME_OVER || currentState == YOU_WIN) // Draw even if game over/won
    {
        // Draw Paddle
        DrawRectangleRec(paddle.rect, paddle.color);

        // Draw Bricks
        for (int i = 0; i < BRICK_ROWS; i++)
        {
            for (int j = 0; j < BRICKS_PER_ROW; j++)
            {
                if (bricks[i][j].active)
                {
                    DrawRectangleRec(bricks[i][j].rect, bricks[i][j].color);
                }
            }
        }

         // Draw Ball (only if active, or maybe always show it?)
        if (ball.active || currentState == PLAYING) // Draw ball if playing or just before starting a new life
        {
             DrawCircleV(ball.position, ball.radius, ball.color);
        }


        // Draw UI (Score and Lives)
        DrawText(TextFormat("SCORE: %04i", score), 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("LIVES: %i", lives), SCREEN_WIDTH - 100, 10, 20, DARKGRAY);

        // Draw Pause Message
        if (paused && currentState == PLAYING)
        {
            DrawText("PAUSED", SCREEN_WIDTH / 2 - MeasureText("PAUSED", 40) / 2, SCREEN_HEIGHT / 2 - 20, 40, GRAY);
        }
    }


    // Draw Game Over / Win Messages
    if (currentState == GAME_OVER)
    {
        DrawRectangle(0, SCREEN_HEIGHT/2 - 40, SCREEN_WIDTH, 80, Fade(BLACK, 0.7f)); // Semi-transparent background
        DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, SCREEN_HEIGHT / 2 - 20, 40, RED);
        DrawText("Press [ENTER] to RESTART", SCREEN_WIDTH / 2 - MeasureText("Press [ENTER] to RESTART", 20) / 2, SCREEN_HEIGHT / 2 + 25, 20, LIGHTGRAY);
    }
    else if (currentState == YOU_WIN)
    {
        DrawRectangle(0, SCREEN_HEIGHT/2 - 40, SCREEN_WIDTH, 80, Fade(BLACK, 0.7f)); // Semi-transparent background
        DrawText("YOU WIN!", SCREEN_WIDTH / 2 - MeasureText("YOU WIN!", 40) / 2, SCREEN_HEIGHT / 2 - 20, 40, GREEN);
        DrawText("Press [ENTER] to PLAY AGAIN", SCREEN_WIDTH / 2 - MeasureText("Press [ENTER] to PLAY AGAIN", 20) / 2, SCREEN_HEIGHT / 2 + 25, 20, LIGHTGRAY);

    }


    EndDrawing();
}

// Unload game-specific resources
void UnloadGame(void)
{
    // TODO: Unload any textures, sounds, fonts if loaded
    // For this simple example, there's nothing extra beyond what CloseWindow handles.
}

// Update and Draw (one frame)
void UpdateDrawFrame(void)
{
    UpdateGame();
    DrawGame();
}