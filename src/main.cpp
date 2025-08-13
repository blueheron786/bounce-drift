#include <gba_video.h>
#include <gba_input.h>
#include <gba_sound.h>
#include <gba_systemcalls.h>
#include <gba_interrupt.h>

// Constants
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160
#define BACKGROUND_COLOR RGB5(0, 0, 8)
#define BALL_COLOR RGB5(31, 31, 31)
#define BRICK_COLOR RGB5(31, 15, 0)
#define LAUNCHER_COLOR RGB5(15, 31, 15)
#define CHARGE_BAR_COLOR RGB5(31, 31, 0)

// Fixed point math (16.16)
#define FIXED_SHIFT 16
#define FIXED_ONE (1 << FIXED_SHIFT)
#define INT_TO_FIXED(x) ((x) << FIXED_SHIFT)
#define FIXED_TO_INT(x) ((x) >> FIXED_SHIFT)

// Physics constants
#define GRAVITY INT_TO_FIXED(1) / 12    // Slower gravity
#define MAX_VELOCITY INT_TO_FIXED(6)    // Much slower max speed
#define BOUNCE_DAMPING (FIXED_ONE * 8 / 10)  // Less damping for more lively bounces
#define NUDGE_FORCE INT_TO_FIXED(3)     // Reduced nudge force

// Track ball position for selective clearing
int lastBallX = -1, lastBallY = -1;

// Game structures
struct Ball {
    int x, y;        // Fixed point
    int vx, vy;      // Fixed point velocity
    int radius;
    bool active;
};

struct Brick {
    int x, y;
    int width, height;
    bool active;
    u16 color;
};

struct Launcher {
    int x, y;
    int width, height;
    int charge;      // 0-100
    bool charging;
};

// Game state
Ball ball;
Brick bricks[20];
Launcher launcher;
int numBricks;

void drawPixel(int x, int y, u16 color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        ((u16*)VRAM)[y * SCREEN_WIDTH + x] = color;
    }
}

void drawRect(int x, int y, int width, int height, u16 color) {
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            drawPixel(x + dx, y + dy, color);
        }
    }
}

void drawCircle(int centerX, int centerY, int radius, u16 color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                drawPixel(centerX + x, centerY + y, color);
            }
        }
    }
}

void clearScreen() {
    // Clear VRAM directly for speed
    u32* vram32 = (u32*)VRAM;
    u32 clearColor = (BACKGROUND_COLOR << 16) | BACKGROUND_COLOR;
    int words = (SCREEN_WIDTH * SCREEN_HEIGHT) / 2;
    
    for (int i = 0; i < words; i++) {
        *vram32++ = clearColor;
    }
}

void initGame() {
    // Initialize ball
    ball.x = INT_TO_FIXED(200);
    ball.y = INT_TO_FIXED(120);
    ball.vx = 0;
    ball.vy = 0;
    ball.radius = 3;
    ball.active = false;
    
    // Initialize launcher
    launcher.x = 200;
    launcher.y = 50;
    launcher.width = 30;
    launcher.height = 80;
    launcher.charge = 0;
    launcher.charging = false;
    
    // Initialize bricks - positioned below ball launch area
    numBricks = 0;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 5; col++) {
            if (numBricks < 20) {
                bricks[numBricks].x = col * 35 + 15;  // Left side of screen
                bricks[numBricks].y = row * 12 + 100; // Below launcher (y=100-148)
                bricks[numBricks].width = 30;
                bricks[numBricks].height = 8;  // Shorter blocks
                bricks[numBricks].active = true;
                bricks[numBricks].color = RGB5(31 - col * 5, row * 7, 15);
                numBricks++;
            }
        }
    }
}

void handleInput() {
    scanKeys();
    u16 keys = keysHeld();
    u16 keysPressed = keysDown();
    
    // Handle charging
    if (keys & KEY_A && !ball.active) {
        launcher.charging = true;
        if (launcher.charge < 100) {
            launcher.charge += 4;  // Increased from 2 to 4 for faster charging
        }
    } else if (launcher.charging) {
        // Release - launch ball with slower, more manageable velocity
        ball.active = true;
        ball.x = INT_TO_FIXED(launcher.x - 10);
        ball.y = INT_TO_FIXED(launcher.y + launcher.height / 2);
        ball.vx = -(launcher.charge * INT_TO_FIXED(5)) / 100;   // Reduced from 8 to 5
        ball.vy = -(launcher.charge * INT_TO_FIXED(3)) / 100;   // Reduced from 4 to 3
        launcher.charge = 0;
        launcher.charging = false;
        
        // Play launch sound
        REG_SOUND1CNT_L = 0x0040;
        REG_SOUND1CNT_H = 0x8000 | (7 << 12);
        REG_SOUND1CNT_X = 0x8000 | (1024 - 262);
    }
    
    // Handle nudging during ball flight - gentler control
    if (ball.active) {
        // Use keysHeld for very gentle continuous nudging
        if (keys & KEY_UP) {
            ball.vy -= NUDGE_FORCE / 4;  // Much smaller continuous nudge
        }
        if (keys & KEY_DOWN) {
            ball.vy += NUDGE_FORCE / 4;
        }
        if (keys & KEY_LEFT) {
            ball.vx -= NUDGE_FORCE / 4;
        }
        if (keys & KEY_RIGHT) {
            ball.vx += NUDGE_FORCE / 4;
        }
        
        // Moderate nudges on key press
        if (keysPressed & KEY_UP) {
            ball.vy -= NUDGE_FORCE / 2;  // Reduced force
        }
        if (keysPressed & KEY_DOWN) {
            ball.vy += NUDGE_FORCE / 2;
        }
        if (keysPressed & KEY_LEFT) {
            ball.vx -= NUDGE_FORCE / 2;
        }
        if (keysPressed & KEY_RIGHT) {
            ball.vx += NUDGE_FORCE / 2;
        }
    }
}

void handleWallCollisions() {
    if (!ball.active) return;
    
    int ballX = FIXED_TO_INT(ball.x);
    int ballY = FIXED_TO_INT(ball.y);
    
    // Left/right walls
    if (ballX <= ball.radius) {
        ball.x = INT_TO_FIXED(ball.radius);
        ball.vx = -ball.vx;
        ball.vx = (ball.vx * BOUNCE_DAMPING) >> FIXED_SHIFT;
    } else if (ballX >= SCREEN_WIDTH - ball.radius) {
        ball.x = INT_TO_FIXED(SCREEN_WIDTH - ball.radius);
        ball.vx = -ball.vx;
        ball.vx = (ball.vx * BOUNCE_DAMPING) >> FIXED_SHIFT;
    }
    
    // Top wall
    if (ballY <= ball.radius) {
        ball.y = INT_TO_FIXED(ball.radius);
        ball.vy = -ball.vy;
        ball.vy = (ball.vy * BOUNCE_DAMPING) >> FIXED_SHIFT;
    }
    
    // Bottom wall - reset ball
    if (ballY >= SCREEN_HEIGHT + 20) {
        ball.active = false;
        ball.vx = 0;
        ball.vy = 0;
    }
}

bool ballBrickCollision(Ball* b, Brick* brick) {
    if (!b->active || !brick->active) return false;
    
    int ballX = FIXED_TO_INT(b->x);
    int ballY = FIXED_TO_INT(b->y);
    
    int closestX = ballX;
    int closestY = ballY;
    
    if (ballX < brick->x) closestX = brick->x;
    else if (ballX > brick->x + brick->width) closestX = brick->x + brick->width;
    
    if (ballY < brick->y) closestY = brick->y;
    else if (ballY > brick->y + brick->height) closestY = brick->y + brick->height;
    
    int dx = ballX - closestX;
    int dy = ballY - closestY;
    
    return (dx * dx + dy * dy) <= (b->radius * b->radius);
}

void updateBall() {
    if (!ball.active) return;
    
    // Apply gravity
    ball.vy += GRAVITY;
    
    // Limit velocity
    if (ball.vx > MAX_VELOCITY) ball.vx = MAX_VELOCITY;
    if (ball.vx < -MAX_VELOCITY) ball.vx = -MAX_VELOCITY;
    if (ball.vy > MAX_VELOCITY) ball.vy = MAX_VELOCITY;
    if (ball.vy < -MAX_VELOCITY) ball.vy = -MAX_VELOCITY;
    
    // Update position
    ball.x += ball.vx;
    ball.y += ball.vy;
    
    // Handle wall collisions
    handleWallCollisions();
    
    // Check brick collisions
    for (int i = 0; i < numBricks; i++) {
        if (ballBrickCollision(&ball, &bricks[i])) {
            // Clear the brick area immediately when destroyed
            drawRect(bricks[i].x, bricks[i].y, 
                    bricks[i].width, bricks[i].height, BACKGROUND_COLOR);
            
            bricks[i].active = false;
            
            // Simple bounce
            ball.vy = -ball.vy;
            ball.vx = (ball.vx * BOUNCE_DAMPING) >> FIXED_SHIFT;
            ball.vy = (ball.vy * BOUNCE_DAMPING) >> FIXED_SHIFT;
            
            // Play hit sound
            REG_SOUND2CNT_L = 0x8040;
            REG_SOUND2CNT_H = 0x8000 | (6 << 12) | (1 << 6);
            break;
        }
    }
}

void render() {
    VBlankIntrWait();  // Wait for VBlank first
    
    // Idiomatic GBA: overdraw without clearing (faster)
    // Background objects will naturally cover old pixels
    
    // Draw launcher
    drawRect(launcher.x, launcher.y, launcher.width, launcher.height, LAUNCHER_COLOR);
    
    // Draw charge bar
    if (launcher.charging && launcher.charge > 0) {
        int barHeight = (launcher.charge * launcher.height) / 100;
        drawRect(launcher.x + launcher.width + 5, 
                 launcher.y + launcher.height - barHeight, 
                 5, barHeight, CHARGE_BAR_COLOR);
    } else {
        // Clear charge bar area when not charging
        drawRect(launcher.x + launcher.width + 5, launcher.y, 5, launcher.height, BACKGROUND_COLOR);
    }
    
    // Draw bricks
    for (int i = 0; i < numBricks; i++) {
        if (bricks[i].active) {
            drawRect(bricks[i].x, bricks[i].y, 
                    bricks[i].width, bricks[i].height, bricks[i].color);
        }
    }
    
    // Draw ball with selective clearing that respects other objects
    if (ball.active) {
        int currentBallX = FIXED_TO_INT(ball.x);
        int currentBallY = FIXED_TO_INT(ball.y);
        
        // Clear old ball position if it moved, but only if not overlapping objects
        if (lastBallX != -1 && lastBallY != -1 && 
            (lastBallX != currentBallX || lastBallY != currentBallY)) {
            
            // Check if old ball position overlaps with any active bricks
            bool overlapsBrick = false;
            for (int i = 0; i < numBricks; i++) {
                if (bricks[i].active) {
                    // Check if old ball position overlaps this brick
                    if (lastBallX + ball.radius >= bricks[i].x && 
                        lastBallX - ball.radius <= bricks[i].x + bricks[i].width &&
                        lastBallY + ball.radius >= bricks[i].y && 
                        lastBallY - ball.radius <= bricks[i].y + bricks[i].height) {
                        overlapsBrick = true;
                        break;
                    }
                }
            }
            
            // Check if old ball position overlaps launcher
            bool overlapsLauncher = (lastBallX + ball.radius >= launcher.x && 
                                   lastBallX - ball.radius <= launcher.x + launcher.width &&
                                   lastBallY + ball.radius >= launcher.y && 
                                   lastBallY - ball.radius <= launcher.y + launcher.height);
            
            // Only clear if not overlapping any objects
            if (!overlapsBrick && !overlapsLauncher) {
                drawCircle(lastBallX, lastBallY, ball.radius, BACKGROUND_COLOR);
            }
        }
        
        // Draw new ball position
        drawCircle(currentBallX, currentBallY, ball.radius, BALL_COLOR);
        
        // Update last position
        lastBallX = currentBallX;
        lastBallY = currentBallY;
    } else {
        // Clear ball when not active, but only if safe
        if (lastBallX != -1 && lastBallY != -1) {
            drawCircle(lastBallX, lastBallY, ball.radius, BACKGROUND_COLOR);
            lastBallX = -1;
            lastBallY = -1;
        }
    }
}

int main() {
    REG_DISPCNT = MODE_3 | BG2_ENABLE;
    irqInit();
    irqEnable(IRQ_VBLANK);
    
    // Initialize sound
    REG_SOUNDCNT_X = 0x80;
    REG_SOUNDCNT_L = 0xFF77;
    REG_SOUNDCNT_H = 2;
    
    initGame();
    
    // Clear screen once at startup (idiomatic GBA)
    clearScreen();
    
    while (1) {
        handleInput();
        updateBall();
        render();
    }
    
    return 0;
}
