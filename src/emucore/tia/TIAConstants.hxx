//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIA_CONSTANTS_HXX
#define TIA_CONSTANTS_HXX

#include "BitmaskEnum.hxx"
#include "bspf.hxx"

/**
  Compile-time TIA geometry and timing constants: pixel dimensions, clock
  frequencies, and blanking intervals. Also defines register name and
  collision bit enumerations used throughout the emulation core.
*/
namespace TIAConstants {
  // TIA output pixels per scanline (visible area)
  static constexpr uInt32 frameBufferWidth = 160;
  // Maximum scanlines in the internal pixel buffer (2x PAL height)
  static constexpr uInt32 frameBufferHeight = 320;
  // Lower bound on vertical centering offset
  static constexpr Int32  minVcenter = -20;
  // Upper bound on vertical centering offset
  static constexpr Int32  maxVcenter = 20;
  // Display width after 2x horizontal scaling
  static constexpr uInt32 viewableWidth = 320;
  // Display height for the viewable region
  static constexpr uInt32 viewableHeight = 240;
  // Frames to discard at startup while the ROM stabilizes
  static constexpr uInt32 initialGarbageFrames = 10;

  static constexpr uInt16
    // Visible color clocks per scanline
    H_PIXEL = 160,
    // Total CPU cycles per scanline
    H_CYCLES = 76,
    // Color clocks per CPU cycle
    CYCLE_CLOCKS = 3,
    // Total color clocks per scanline (= 228)
    H_CLOCKS = H_CYCLES * CYCLE_CLOCKS,
    // Color clocks in the horizontal blank region (= 68)
    H_BLANK_CLOCKS = H_CLOCKS - H_PIXEL;
}  // namespace TIAConstants

enum class BitState: uInt8 {
  Off    = 0,
  On     = 1,
  Toggle = 2,
  Query  = 3
};

enum class TIABit: uInt8 {
  None     = 0,
  P0       = Bitmask::bit<TIABit>(0),  // Bit for Player 0
  M0       = Bitmask::bit<TIABit>(1),  // Bit for Missile 0
  P1       = Bitmask::bit<TIABit>(2),  // Bit for Player 1
  M1       = Bitmask::bit<TIABit>(3),  // Bit for Missile 1
  BL       = Bitmask::bit<TIABit>(4),  // Bit for Ball
  PF       = Bitmask::bit<TIABit>(5),  // Bit for Playfield
  Score    = Bitmask::bit<TIABit>(6),  // Bit for Playfield score mode
  Priority = Bitmask::bit<TIABit>(7),  // Bit for Playfield priority
  All      = 0xFF
};
template<> inline constexpr bool Bitmask::is_enum_v<TIABit> = true;

enum TIAColor: uInt8 {
  BKColor     = 0,  // Color index for Background
  PFColor     = 1,  // Color index for Playfield
  P0Color     = 2,  // Color index for Player 0
  P1Color     = 3,  // Color index for Player 1
  M0Color     = 4,  // Color index for Missile 0
  M1Color     = 5,  // Color index for Missile 1
  BLColor     = 6,  // Color index for Ball
  HBLANKColor = 7   // Color index for HMove blank area
};

enum class CollisionBit: uInt16
{
  M0P1 = Bitmask::bit<CollisionBit>(0),  // Missile0 - Player1
  M0P0 = Bitmask::bit<CollisionBit>(1),  // Missile0 - Player0
  M1P0 = Bitmask::bit<CollisionBit>(2),  // Missile1 - Player0
  M1P1 = Bitmask::bit<CollisionBit>(3),  // Missile1 - Player1
  P0PF = Bitmask::bit<CollisionBit>(4),  // Player0  - Playfield
  P0BL = Bitmask::bit<CollisionBit>(5),  // Player0  - Ball
  P1PF = Bitmask::bit<CollisionBit>(6),  // Player1  - Playfield
  P1BL = Bitmask::bit<CollisionBit>(7),  // Player1  - Ball
  M0PF = Bitmask::bit<CollisionBit>(8),  // Missile0 - Playfield
  M0BL = Bitmask::bit<CollisionBit>(9),  // Missile0 - Ball
  M1PF = Bitmask::bit<CollisionBit>(10), // Missile1 - Playfield
  M1BL = Bitmask::bit<CollisionBit>(11), // Missile1 - Ball
  BLPF = Bitmask::bit<CollisionBit>(12), // Ball     - Playfield
  P0P1 = Bitmask::bit<CollisionBit>(13), // Player0  - Player1
  M0M1 = Bitmask::bit<CollisionBit>(14)  // Missile0 - Missile1
};

// TIA Write/Read register names
enum TIARegister: uInt8 {
  VSYNC   = 0x00,  // Write: vertical sync set-clear (D1)
  VBLANK  = 0x01,  // Write: vertical blank set-clear (D7-6,D1)
  WSYNC   = 0x02,  // Write: wait for leading edge of hrz. blank (strobe)
  RSYNC   = 0x03,  // Write: reset hrz. sync counter (strobe)
  NUSIZ0  = 0x04,  // Write: number-size player-missle 0 (D5-0)
  NUSIZ1  = 0x05,  // Write: number-size player-missle 1 (D5-0)
  COLUP0  = 0x06,  // Write: color-lum player 0 (D7-1)
  COLUP1  = 0x07,  // Write: color-lum player 1 (D7-1)
  COLUPF  = 0x08,  // Write: color-lum playfield (D7-1)
  COLUBK  = 0x09,  // Write: color-lum background (D7-1)
  CTRLPF  = 0x0a,  // Write: cntrl playfield ballsize & coll. (D5-4,D2-0)
  REFP0   = 0x0b,  // Write: reflect player 0 (D3)
  REFP1   = 0x0c,  // Write: reflect player 1 (D3)
  PF0     = 0x0d,  // Write: playfield register byte 0 (D7-4)
  PF1     = 0x0e,  // Write: playfield register byte 1 (D7-0)
  PF2     = 0x0f,  // Write: playfield register byte 2 (D7-0)
  RESP0   = 0x10,  // Write: reset player 0 (strobe)
  RESP1   = 0x11,  // Write: reset player 1 (strobe)
  RESM0   = 0x12,  // Write: reset missle 0 (strobe)
  RESM1   = 0x13,  // Write: reset missle 1 (strobe)
  RESBL   = 0x14,  // Write: reset ball (strobe)
  AUDC0   = 0x15,  // Write: audio control 0 (D3-0)
  AUDC1   = 0x16,  // Write: audio control 1 (D4-0)
  AUDF0   = 0x17,  // Write: audio frequency 0 (D4-0)
  AUDF1   = 0x18,  // Write: audio frequency 1 (D3-0)
  AUDV0   = 0x19,  // Write: audio volume 0 (D3-0)
  AUDV1   = 0x1a,  // Write: audio volume 1 (D3-0)
  GRP0    = 0x1b,  // Write: graphics player 0 (D7-0)
  GRP1    = 0x1c,  // Write: graphics player 1 (D7-0)
  ENAM0   = 0x1d,  // Write: graphics (enable) missle 0 (D1)
  ENAM1   = 0x1e,  // Write: graphics (enable) missle 1 (D1)
  ENABL   = 0x1f,  // Write: graphics (enable) ball (D1)
  HMP0    = 0x20,  // Write: horizontal motion player 0 (D7-4)
  HMP1    = 0x21,  // Write: horizontal motion player 1 (D7-4)
  HMM0    = 0x22,  // Write: horizontal motion missle 0 (D7-4)
  HMM1    = 0x23,  // Write: horizontal motion missle 1 (D7-4)
  HMBL    = 0x24,  // Write: horizontal motion ball (D7-4)
  VDELP0  = 0x25,  // Write: vertical delay player 0 (D0)
  VDELP1  = 0x26,  // Write: vertical delay player 1 (D0)
  VDELBL  = 0x27,  // Write: vertical delay ball (D0)
  RESMP0  = 0x28,  // Write: reset missle 0 to player 0 (D1)
  RESMP1  = 0x29,  // Write: reset missle 1 to player 1 (D1)
  HMOVE   = 0x2a,  // Write: apply horizontal motion (strobe)
  HMCLR   = 0x2b,  // Write: clear horizontal motion registers (strobe)
  CXCLR   = 0x2c,  // Write: clear collision latches (strobe)

  CXM0P   = 0x00,  // Read collision: D7=(M0,P1); D6=(M0,P0)
  CXM1P   = 0x01,  // Read collision: D7=(M1,P0); D6=(M1,P1)
  CXP0FB  = 0x02,  // Read collision: D7=(P0,PF); D6=(P0,BL)
  CXP1FB  = 0x03,  // Read collision: D7=(P1,PF); D6=(P1,BL)
  CXM0FB  = 0x04,  // Read collision: D7=(M0,PF); D6=(M0,BL)
  CXM1FB  = 0x05,  // Read collision: D7=(M1,PF); D6=(M1,BL)
  CXBLPF  = 0x06,  // Read collision: D7=(BL,PF); D6=(unused)
  CXPPMM  = 0x07,  // Read collision: D7=(P0,P1); D6=(M0,M1)
  INPT0   = 0x08,  // Read pot port: D7
  INPT1   = 0x09,  // Read pot port: D7
  INPT2   = 0x0a,  // Read pot port: D7
  INPT3   = 0x0b,  // Read pot port: D7
  INPT4   = 0x0c,  // Read P1 joystick trigger: D7
  INPT5   = 0x0d   // Read P2 joystick trigger: D7
};

#endif  // TIA_CONSTANTS_HXX
