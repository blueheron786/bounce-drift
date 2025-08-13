#ifndef GBA_H
#define GBA_H

// GBA-specific constants and definitions

// Video modes
#define MODE_0 0
#define MODE_1 1
#define MODE_2 2
#define MODE_3 3
#define MODE_4 4
#define MODE_5 5

// Background enable flags
#define BG0_ENABLE (1 << 8)
#define BG1_ENABLE (1 << 9)
#define BG2_ENABLE (1 << 10)
#define BG3_ENABLE (1 << 11)

// Color definitions
#define RGB15(r, g, b) ((r) | ((g) << 5) | ((b) << 10))

// Function prototypes
void VBlankIntrWait(void);
void m3_plot(int x, int y, unsigned short color);

#endif // GBA_H