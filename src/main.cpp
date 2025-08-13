#include <gba.h>
#include <gba_video.h>
#include <gba_input.h>
#include <gba_sound.h>
#include <gba_systemcalls.h>
#include <gba_interrupt.h>

// Player struct
struct Player {
    int x, y;
    int w, h;
    u16 color;
};

// Static obstacle positions
struct Obstacle {
    int x, y;
    int w, h;
    u16 color;
};

// Function to draw a filled rectangle
void drawRect(u16* fb, int x, int y, int w, int h, u16 color) {
    for (int dy = 0; dy < h; ++dy) {
        for (int dx = 0; dx < w; ++dx) {
            if (x + dx >= 0 && x + dx < 240 && y + dy >= 0 && y + dy < 160) {
                fb[(y + dy) * 240 + (x + dx)] = color;
            }
        }
    }
}

// AABB collision detection
bool checkCollision(const Player& player, const Obstacle& obs) {
    return player.x < obs.x + obs.w &&
           player.x + player.w > obs.x &&
           player.y < obs.y + obs.h &&
           player.y + player.h > obs.y;
}

// Simple sound effect (beep)
void playThudSound() {
    // Set up sound channel 1 for a simple square wave
    REG_SOUND1CNT_L = 0x0040; // Sweep off
    REG_SOUND1CNT_H = 0x8000 | (0 << 6); // Square wave, duty cycle 12.5%
    REG_SOUND1CNT_X = 0x8400 | (1000); // Enable, frequency ~1000Hz
}

// Draw button states
void drawButtons(u16* fb, u16 keys) {
    u16 grey = RGB5(15, 15, 15);
    u16 blue = RGB5(0, 15, 31);
    
    // A button (right)
    drawRect(fb, 200, 120, 16, 16, (keys & KEY_A) ? blue : grey);
    
    // B button (left of A)
    drawRect(fb, 180, 130, 16, 16, (keys & KEY_B) ? blue : grey);
    
    // L button (top left)
    drawRect(fb, 10, 10, 20, 8, (keys & KEY_L) ? blue : grey);
    
    // R button (top right)
    drawRect(fb, 210, 10, 20, 8, (keys & KEY_R) ? blue : grey);
}

int main() {
    // Set video mode 3 (bitmap) and background 2
    REG_DISPCNT = MODE_3 | BG2_ENABLE;
    
    // Initialize input system
    irqInit();
    irqEnable(IRQ_VBLANK);
    
    // Enable sound
    REG_SOUNDCNT_X = 0x0080; // Master sound enable
    REG_SOUNDCNT_L = 0x1177; // Enable all channels
    REG_SOUNDCNT_H = 0x0002; // Sound A/B to 100%, timer 0/1
    
    u16* fb = (u16*)VRAM;
    
    // Initialize player (blue block)
    Player player = {120, 80, 16, 16, RGB5(0, 15, 31)};
    
    // Initialize obstacles (white blocks)
    Obstacle obstacles[] = {
        {50, 50, 20, 20, RGB5(31, 31, 31)},
        {180, 60, 20, 20, RGB5(31, 31, 31)},
        {100, 120, 20, 20, RGB5(31, 31, 31)},
        {160, 30, 20, 20, RGB5(31, 31, 31)}
    };
    int numObstacles = sizeof(obstacles) / sizeof(obstacles[0]);
    
    Player oldPlayer = player;
    u16 oldKeys = 0;
    
    // Initial screen clear
    for (int i = 0; i < 240 * 160; i++) {
        fb[i] = RGB5(0, 0, 0);
    }
    
    // Draw initial obstacles
    for (int i = 0; i < numObstacles; i++) {
        drawRect(fb, obstacles[i].x, obstacles[i].y, obstacles[i].w, obstacles[i].h, obstacles[i].color);
    }
    
    // Draw initial player
    drawRect(fb, player.x, player.y, player.w, player.h, player.color);
    
    // Draw initial button states (all grey)
    drawButtons(fb, 0);
    
    while (1) {
        VBlankIntrWait(); // Wait for VBlank first to reduce flicker
        
        // Read input
        scanKeys();
        u16 keys = keysHeld();
        
        // Store old position for collision handling
        oldPlayer = player;
        
        // Move player with WASD (using D-pad)
        if (keys & KEY_LEFT) player.x -= 2;
        if (keys & KEY_RIGHT) player.x += 2;
        if (keys & KEY_UP) player.y -= 2;
        if (keys & KEY_DOWN) player.y += 2;
        
        // Keep player on screen
        if (player.x < 0) player.x = 0;
        if (player.x > 240 - player.w) player.x = 240 - player.w;
        if (player.y < 0) player.y = 0;
        if (player.y > 160 - player.h) player.y = 160 - player.h;
        
        // Check collisions with obstacles
        bool collided = false;
        for (int i = 0; i < numObstacles; i++) {
            if (checkCollision(player, obstacles[i])) {
                // Collision detected - revert to old position and play sound
                player = oldPlayer;
                playThudSound();
                collided = true;
                break;
            }
        }
        
        // Only redraw if player moved
        if (player.x != oldPlayer.x || player.y != oldPlayer.y) {
            // Erase old player position
            drawRect(fb, oldPlayer.x, oldPlayer.y, oldPlayer.w, oldPlayer.h, RGB5(0, 0, 0));
            
            // Redraw any obstacles that might have been erased
            for (int i = 0; i < numObstacles; i++) {
                if (oldPlayer.x < obstacles[i].x + obstacles[i].w &&
                    oldPlayer.x + oldPlayer.w > obstacles[i].x &&
                    oldPlayer.y < obstacles[i].y + obstacles[i].h &&
                    oldPlayer.y + oldPlayer.h > obstacles[i].y) {
                    drawRect(fb, obstacles[i].x, obstacles[i].y, obstacles[i].w, obstacles[i].h, obstacles[i].color);
                }
            }
            
            // Draw player at new position
            drawRect(fb, player.x, player.y, player.w, player.h, player.color);
        }
        
        // Only redraw buttons if keys changed
        if (keys != oldKeys) {
            // Erase old button area (just clear the button regions)
            drawRect(fb, 200, 120, 16, 16, RGB5(0, 0, 0)); // A
            drawRect(fb, 180, 130, 16, 16, RGB5(0, 0, 0)); // B  
            drawRect(fb, 10, 10, 20, 8, RGB5(0, 0, 0));    // L
            drawRect(fb, 210, 10, 20, 8, RGB5(0, 0, 0));   // R
            
            // Draw buttons with new state
            drawButtons(fb, keys);
            oldKeys = keys;
        }
        
        // Update old player position
        oldPlayer = player;
    }
    return 0;
}