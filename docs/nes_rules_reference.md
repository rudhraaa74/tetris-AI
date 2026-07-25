# NES Tetris Rules Reference

This document serves as the ground-truth spec for the engine. All mechanics and constants are derived from the original NES Tetris (USA ROM) disassembly analysis by MeatFighter.

## Gravity (Frames per row)
The number of frames it takes for a piece to drop one row, based on the current level:
```
Level:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29
Frames:48 43 38 33 28 23 18 13  8  6  5  5  5  4  4  4  3  3  3  2  2  2  2  2  2  2  2  2  2  1
```
For levels 29 and above, the drop speed is locked at 1 frame per drop.

## Delayed Auto Shift (DAS)
- **Initial Delay**: 16 frames after a directional button (Left/Right) is held.
- **Repeat Rate**: Every 6 frames thereafter.
- **Reset to 10**: When an auto-shift fires, the counter resets to 10 (not 0). Thus the sequence is: hold -> 16 frames (shift) -> 10 frames wait -> 6 frames (shift) -> repeats every 6.
- **State Freezing**: During Line Clearing and ARE (Entry Delay), the DAS counter is frozen but retains its charge.
- **Wall Charge**: If a shift attempt is blocked by a wall or the stack, the counter instantly jumps to fully-charged (16). This allows players to "pre-charge" their DAS for the next piece.

## Drop and Lock Delay
- **Lock Delay**: The lock delay is equal to the current drop delay (gravity). A piece locks immediately on the gravity frame it tries to drop but cannot due to collision.
- **Soft Drop**: Holding Down increments a counter. When the counter reaches 3, a soft drop occurs and the counter is reset to 1. Thus, the first soft drop takes 3 frames, and it repeats every 2 frames.
- **Opening Entry Delay**: The very first piece in the game has an entry delay of 96 frames.

## Entry Delay (ARE)
Depends on the height at which the previous piece locked:
- Bottom 2 rows: 10 frames.
- Every group of 4 rows above that adds 2 frames. Up to ~18 frames for the highest lock positions.

## Line Clear Delay
An additional 17-20 frames beyond ARE when lines are cleared. The 5-step clear animation advances every 4 frames.

## Scoring
- **Line Clears**: Base values are Single = 40, Double = 100, Triple = 300, Tetris = 1200.
- **Multiplier**: `(level + 1)`. 
- **Level-Up Timing**: If a line clear triggers a level-up, the multiplier is the *post-transition* level.
- **Soft Drop**: 1 point per row actively fallen while soft-drop is held. Only the last continuous soft-drop press before the lock counts, capped by board height.

## Level-Up Thresholds
- First transition: `min(startLevel * 10 + 10, max(100, startLevel * 10 - 50))` lines.
- After first transition: +10 lines per level.

## Randomizer (LFSR)
The NES uses a 16-bit Fibonacci LFSR seeded at `$8988`.
- Taps at bit 1 and bit 9 (0-indexed).
- `new_bit = (bit 1) ^ (bit 9)`
- `value = (new_bit << 15) | (value >> 1)`
- The PRNG is advanced at least once per frame globally.
- **Piece Selection**:
  1. Roll a random number 0-7.
  2. If the roll matches the previous piece, or is 7 (dummy value), reroll 0-6. Use the result unconditionally.

## Pieces and Rotation (NRS)
- Right-handed Nintendo Rotation System.
- No Wall Kicks. Rotations that would collide are rejected.
- **Spawn Coordinates (at X=5, Y=0)**:
  - T (down): `{ {-1, 0}, {0, 0}, {1, 0}, {0, 1} }`
  - J (down): `{ {-1, 0}, {0, 0}, {1, 0}, {1, 1} }`
  - Z (horiz): `{ {-1, 0}, {0, 0}, {0, 1}, {1, 1} }`
  - O: `{ {-1, 0}, {0, 0}, {-1, 1}, {0, 1} }`
  - S (horiz): `{ {0, 0}, {1, 0}, {-1, 1}, {0, 1} }`
  - L (down): `{ {-1, 0}, {0, 0}, {1, 0}, {-1, 1} }`
  - I (horiz): `{ {-2, 0}, {-1, 0}, {0, 0}, {1, 0} }`

## Board
- 10 columns by 22 rows.
- 0-indexed, where `y=0` and `y=1` are hidden buffer rows, and `y=2..21` are the visible playfield.
