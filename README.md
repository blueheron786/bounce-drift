# Bounce Drift - Pinball Breakout Hybrid

A Game Boy Advance game that combines pinball physics with breakout gameplay mechanics.

## Game Overview
This is a pinball-breakout hybrid where you launch a ball from a charging tube to destroy bricks. The ball bounces around the screen with physics, and you can influence its movement with nudging controls.

## How to Play

### Controls
- **A Button**: Hold to charge the launcher, release to fire the ball
- **Left/Right Arrows**: Nudge the ball while it's in flight (limited force)

### Gameplay
1. **Aiming Phase**: The ball sits in the launcher tube on the right side. Press and hold A to start charging.

2. **Charging Phase**: Hold A to build up launch power (yellow charge bar fills up). The longer you hold, the more powerful the launch will be.

3. **Ball Active Phase**: Release A to rocket the ball upward! The ball will:
   - Bounce off walls and obstacles
   - Be affected by gravity (falls down)
   - Destroy green bricks on contact
   - Can be nudged left/right with arrow keys while in flight
   - Has maximum velocity limits to keep gameplay manageable

4. **Reset**: When the ball falls off the bottom of the screen, it returns to the aiming phase.

### Game Elements
- **Launcher Tube**: Dark grey tube on the right side with charging indicator
- **Ball**: Red circle that bounces around
- **Bricks**: Green rectangular targets arranged in a 3x4 grid
- **Charge Bar**: Yellow bar inside the launcher showing charge level

## Technical Details
- Uses GBA Mode 3 (bitmap graphics)
- 240x160 resolution with 16-bit color
- Fixed-point physics system for smooth ball movement
- Sound effects for launching and brick hits
- Real-time collision detection

## Building the Project
Requirements:
- DevkitPro with devkitARM
- libgba library

Build command:
```
make
```

The output ROM will be `hello.gba` in the build directory.

## Game Features Implemented
- ✅ Charging launcher with visual feedback
- ✅ Ball physics with gravity and bouncing
- ✅ Brick destruction on collision
- ✅ Wall collision detection
- ✅ Ball nudging with arrow keys
- ✅ Sound effects for actions
- ✅ Game state management (Aiming → Charging → Ball Active)
- ✅ Velocity limiting for balanced gameplay

## Future Enhancements
- Score system
- Multiple levels with different brick patterns
- Power-ups
- Better collision detection
- Particle effects
- More sophisticated sound
- Ball trails/effects