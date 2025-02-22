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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// Generated by src/tools/convbdf on Sat Aug 24 11:39:18 2013.
//============================================================================

#ifndef CONSOLEB_FONT_DATA_HXX
#define CONSOLEB_FONT_DATA_HXX

#include "Font.hxx"

/* Font information:
   name: 8x13B-ISO8859-1
   facename: -Misc-Fixed-Bold-R-Normal--13-120-75-75-C-80-ISO8859-1
   w x h: 8x13
   bbx: 8 13 0 -2
   size: 97
   ascent: 11
   descent: 2
   first char: 30 (0x1e)
   last char: 126 (0x7e)
   default char: 30 (0x1e)
   proportional: no
   Public domain font.  Share and enjoy.
*/

namespace GUI {

// Font character bitmap data.
static const uInt16 consoleB_font_bits[] = {  // NOLINT : too complicated to convert

  /* MODIFIED
  Character 28 (0x1c):
  width 8
  bbx ( 8, 13, 0, -2 )

  +--------+
  |        |
  |  XXXX  |
  | XX  XX |
  | XX  XX |
  | XX  XX |
  |  XXXX  |
  |        |
  |        |
  |        |
  |        |
  |        |
  |        |
  |        |
  +--------+
  */
  0x0000,
  0b0011110000000000,
  0b0110011000000000,
  0b0110011000000000,
  0b0110011000000000,
  0b0011110000000000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,


  /* MODIFIED
  Character 29 (0x1d):
  width 8
  bbx ( 8, 13, 0, -2 )

  +--------+
  |        |
  |        |
  |        |
  |        |
  |        |
  |        |
  |        |
  |        |
  |        |
  |XX XX XX|
  |XX XX XX|
  |        |
  |        |
  +--------+
  */
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0b1101101100000000,
  0b1101101100000000,
  0x0000,
  0x0000,

/* MODIFIED
   Character 30 (0x1e): large centered rounded rectangle
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |  ****  |
   | ****** |
   | ****** |
   | ****** |
   | ****** |
   | ****** |
   | ****** |
   | ****** |
   | ****** |
   | ****** |
   | ****** |
   | ****** |
   |  ****  |
   +--------+
*/
0x3c00,
0x7e00,
0x7e00,
0x7e00,
0x7e00,
0x7e00,
0x7e00,
0x7e00,
0x7e00,
0x7e00,
0x7e00,
0x7e00,
0x3c00,

/* MODIFIED
   Character 31 (0x1f): large centered circle
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |        |
   |  ****  |
   | ****** |
   | ****** |
   |  ****  |
   |        |
   |        |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x3c00,
0x7e00,
0x7e00,
0x3c00,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 32 (0x20):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 33 (0x21):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |        |
   |   **   |
   |   **   |
   |        |
   |        |
   +--------+
*/
0x0000,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x0000,
0x1800,
0x1800,
0x0000,
0x0000,

/* Character 34 (0x22):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | ** **  |
   | ** **  |
   | ** **  |
   | ** **  |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   +--------+
*/
0x0000,
0x6c00,
0x6c00,
0x6c00,
0x6c00,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 35 (0x23):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   | ** **  |
   | ** **  |
   |******* |
   |******* |
   | ** **  |
   |******* |
   |******* |
   | ** **  |
   | ** **  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x6c00,
0x6c00,
0xfe00,
0xfe00,
0x6c00,
0xfe00,
0xfe00,
0x6c00,
0x6c00,
0x0000,
0x0000,

/* Character 36 (0x24):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |   *    |
   | *****  |
   |** * ** |
   |** *    |
   |****    |
   | *****  |
   |   **** |
   |   * ** |
   |** * ** |
   | *****  |
   |   *    |
   |        |
   +--------+
*/
0x0000,
0x1000,
0x7c00,
0xd600,
0xd000,
0xf000,
0x7c00,
0x1e00,
0x1600,
0xd600,
0x7c00,
0x1000,
0x0000,

/* Character 37 (0x25):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |***  ** |
   |* *  ** |
   |*** **  |
   |   **   |
   |   **   |
   |  **    |
   |  **    |
   | ** *** |
   |**  * * |
   |**  *** |
   |        |
   |        |
   +--------+
*/
0x0000,
0xe600,
0xa600,
0xec00,
0x1800,
0x1800,
0x3000,
0x3000,
0x6e00,
0xca00,
0xce00,
0x0000,
0x0000,

/* Character 38 (0x26):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   | ****   |
   |**  **  |
   |**  **  |
   | ****   |
   |**  *** |
   |**  **  |
   | ****** |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x7800,
0xcc00,
0xcc00,
0x7800,
0xce00,
0xcc00,
0x7e00,
0x0000,
0x0000,

/* Character 39 (0x27):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   +--------+
*/
0x0000,
0x1800,
0x1800,
0x1800,
0x1800,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 40 (0x28):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |    **  |
   |   **   |
   |  **    |
   |  **    |
   | **     |
   | **     |
   | **     |
   |  **    |
   |  **    |
   |   **   |
   |    **  |
   |        |
   +--------+
*/
0x0000,
0x0c00,
0x1800,
0x3000,
0x3000,
0x6000,
0x6000,
0x6000,
0x3000,
0x3000,
0x1800,
0x0c00,
0x0000,

/* Character 41 (0x29):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | **     |
   |  **    |
   |   **   |
   |   **   |
   |    **  |
   |    **  |
   |    **  |
   |   **   |
   |   **   |
   |  **    |
   | **     |
   |        |
   +--------+
*/
0x0000,
0x6000,
0x3000,
0x1800,
0x1800,
0x0c00,
0x0c00,
0x0c00,
0x1800,
0x1800,
0x3000,
0x6000,
0x0000,

/* Character 42 (0x2a):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |   *    |
   |   *    |
   |******* |
   |  ***   |
   |  ***   |
   | ** **  |
   | *   *  |
   |        |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x1000,
0x1000,
0xfe00,
0x3800,
0x3800,
0x6c00,
0x4400,
0x0000,
0x0000,
0x0000,

/* Character 43 (0x2b):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |   **   |
   |   **   |
   | ****** |
   | ****** |
   |   **   |
   |   **   |
   |        |
   |        |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x1800,
0x1800,
0x7e00,
0x7e00,
0x1800,
0x1800,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 44 (0x2c):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |  ****  |
   |   ***  |
   |   ***  |
   |   **   |
   |  **    |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x3c00,
0x1c00,
0x1c00,
0x1800,
0x3000,
0x0000,

/* Character 45 (0x2d):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   | ****** |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x7e00,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 46 (0x2e):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |   **   |
   |  ****  |
   |   **   |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x1800,
0x3c00,
0x1800,
0x0000,
0x0000,

/* Character 47 (0x2f):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |      * |
   |     ** |
   |     ** |
   |    **  |
   |   **   |
   |  **    |
   | **     |
   |**      |
   |**      |
   |*       |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0200,
0x0600,
0x0600,
0x0c00,
0x1800,
0x3000,
0x6000,
0xc000,
0xc000,
0x8000,
0x0000,
0x0000,

/* Character 48 (0x30):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |  ***   |
   | ** **  |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   | ** **  |
   |  ***   |
   |        |
   |        |
   +--------+
*/
0x0000,
0x3800,
0x6c00,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0x6c00,
0x3800,
0x0000,
0x0000,

/* Character 49 (0x31):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |   **   |
   |  ***   |
   | ****   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   | ****** |
   |        |
   |        |
   +--------+
*/
0x0000,
0x1800,
0x3800,
0x7800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x7e00,
0x0000,
0x0000,

/* Character 50 (0x32):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | *****  |
   |**   ** |
   |**   ** |
   |     ** |
   |    **  |
   |   **   |
   |  **    |
   | **     |
   |**      |
   |******* |
   |        |
   |        |
   +--------+
*/
0x0000,
0x7c00,
0xc600,
0xc600,
0x0600,
0x0c00,
0x1800,
0x3000,
0x6000,
0xc000,
0xfe00,
0x0000,
0x0000,

/* Character 51 (0x33):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |******* |
   |     ** |
   |    **  |
   |   **   |
   |  ****  |
   |     ** |
   |     ** |
   |     ** |
   |**   ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0xfe00,
0x0600,
0x0c00,
0x1800,
0x3c00,
0x0600,
0x0600,
0x0600,
0xc600,
0x7c00,
0x0000,
0x0000,

/* Character 52 (0x34):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |    **  |
   |   ***  |
   |  ****  |
   | ** **  |
   |**  **  |
   |**  **  |
   |******* |
   |    **  |
   |    **  |
   |    **  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0c00,
0x1c00,
0x3c00,
0x6c00,
0xcc00,
0xcc00,
0xfe00,
0x0c00,
0x0c00,
0x0c00,
0x0000,
0x0000,

/* Character 53 (0x35):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |******* |
   |**      |
   |**      |
   |******  |
   |***  ** |
   |     ** |
   |     ** |
   |     ** |
   |**   ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0xfe00,
0xc000,
0xc000,
0xfc00,
0xe600,
0x0600,
0x0600,
0x0600,
0xc600,
0x7c00,
0x0000,
0x0000,

/* Character 54 (0x36):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |  ****  |
   | **     |
   |**      |
   |**      |
   |******  |
   |***  ** |
   |**   ** |
   |**   ** |
   |***  ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x3c00,
0x6000,
0xc000,
0xc000,
0xfc00,
0xe600,
0xc600,
0xc600,
0xe600,
0x7c00,
0x0000,
0x0000,

/* Character 55 (0x37):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |******* |
   |     ** |
   |     ** |
   |    **  |
   |   **   |
   |   **   |
   |  **    |
   |  **    |
   |  **    |
   |  **    |
   |        |
   |        |
   +--------+
*/
0x0000,
0xfe00,
0x0600,
0x0600,
0x0c00,
0x1800,
0x1800,
0x3000,
0x3000,
0x3000,
0x3000,
0x0000,
0x0000,

/* Character 56 (0x38):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | *****  |
   |**   ** |
   |**   ** |
   |**   ** |
   | *****  |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x7c00,
0xc600,
0xc600,
0xc600,
0x7c00,
0xc600,
0xc600,
0xc600,
0xc600,
0x7c00,
0x0000,
0x0000,

/* Character 57 (0x39):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | *****  |
   |**  *** |
   |**   ** |
   |**   ** |
   |**  *** |
   | ****** |
   |     ** |
   |     ** |
   |    **  |
   | ****   |
   |        |
   |        |
   +--------+
*/
0x0000,
0x7c00,
0xce00,
0xc600,
0xc600,
0xce00,
0x7e00,
0x0600,
0x0600,
0x0c00,
0x7800,
0x0000,
0x0000,

/* Character 58 (0x3a):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |   **   |
   |  ****  |
   |   **   |
   |        |
   |        |
   |   **   |
   |  ****  |
   |   **   |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x1800,
0x3c00,
0x1800,
0x0000,
0x0000,
0x1800,
0x3c00,
0x1800,
0x0000,
0x0000,

/* Character 59 (0x3b):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |   **   |
   |  ****  |
   |   **   |
   |        |
   |  ****  |
   |   ***  |
   |   ***  |
   |   **   |
   |  **    |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x1800,
0x3c00,
0x1800,
0x0000,
0x3c00,
0x1c00,
0x1c00,
0x1800,
0x3000,
0x0000,

/* Character 60 (0x3c):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |     ** |
   |    **  |
   |   **   |
   |  **    |
   | **     |
   |  **    |
   |   **   |
   |    **  |
   |     ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0600,
0x0c00,
0x1800,
0x3000,
0x6000,
0x3000,
0x1800,
0x0c00,
0x0600,
0x0000,
0x0000,

/* Character 61 (0x3d):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |        |
   | ****** |
   |        |
   |        |
   | ****** |
   |        |
   |        |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x7e00,
0x0000,
0x0000,
0x7e00,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 62 (0x3e):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   | **     |
   |  **    |
   |   **   |
   |    **  |
   |     ** |
   |    **  |
   |   **   |
   |  **    |
   | **     |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x6000,
0x3000,
0x1800,
0x0c00,
0x0600,
0x0c00,
0x1800,
0x3000,
0x6000,
0x0000,
0x0000,

/* Character 63 (0x3f):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | *****  |
   |**   ** |
   |**   ** |
   |     ** |
   |    **  |
   |   **   |
   |   **   |
   |        |
   |   **   |
   |   **   |
   |        |
   |        |
   +--------+
*/
0x0000,
0x7c00,
0xc600,
0xc600,
0x0600,
0x0c00,
0x1800,
0x1800,
0x0000,
0x1800,
0x1800,
0x0000,
0x0000,

/* Character 64 (0x40):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   | *****  |
   |******* |
   |**  *** |
   |** **** |
   |** *  * |
   |** *  * |
   |** **** |
   |***     |
   | ****** |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x7c00,
0xfe00,
0xce00,
0xde00,
0xd200,
0xd200,
0xde00,
0xe000,
0x7e00,
0x0000,
0x0000,

/* Character 65 (0x41):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |  ***   |
   | *****  |
   |**   ** |
   |**   ** |
   |**   ** |
   |******* |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0x3800,
0x7c00,
0xc600,
0xc600,
0xc600,
0xfe00,
0xc600,
0xc600,
0xc600,
0xc600,
0x0000,
0x0000,

/* Character 66 (0x42):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |******  |
   | **  ** |
   | **  ** |
   | **  ** |
   | *****  |
   | **  ** |
   | **  ** |
   | **  ** |
   | **  ** |
   |******  |
   |        |
   |        |
   +--------+
*/
0x0000,
0xfc00,
0x6600,
0x6600,
0x6600,
0x7c00,
0x6600,
0x6600,
0x6600,
0x6600,
0xfc00,
0x0000,
0x0000,

/* Character 67 (0x43):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | *****  |
   |***  ** |
   |**   ** |
   |**      |
   |**      |
   |**      |
   |**      |
   |**   ** |
   |***  ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x7c00,
0xe600,
0xc600,
0xc000,
0xc000,
0xc000,
0xc000,
0xc600,
0xe600,
0x7c00,
0x0000,
0x0000,

/* Character 68 (0x44):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |******  |
   | **  ** |
   | **  ** |
   | **  ** |
   | **  ** |
   | **  ** |
   | **  ** |
   | **  ** |
   | **  ** |
   |******  |
   |        |
   |        |
   +--------+
*/
0x0000,
0xfc00,
0x6600,
0x6600,
0x6600,
0x6600,
0x6600,
0x6600,
0x6600,
0x6600,
0xfc00,
0x0000,
0x0000,

/* Character 69 (0x45):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |******* |
   |**      |
   |**      |
   |**      |
   |*****   |
   |**      |
   |**      |
   |**      |
   |**      |
   |******* |
   |        |
   |        |
   +--------+
*/
0x0000,
0xfe00,
0xc000,
0xc000,
0xc000,
0xf800,
0xc000,
0xc000,
0xc000,
0xc000,
0xfe00,
0x0000,
0x0000,

/* Character 70 (0x46):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |******* |
   |**      |
   |**      |
   |**      |
   |*****   |
   |**      |
   |**      |
   |**      |
   |**      |
   |**      |
   |        |
   |        |
   +--------+
*/
0x0000,
0xfe00,
0xc000,
0xc000,
0xc000,
0xf800,
0xc000,
0xc000,
0xc000,
0xc000,
0xc000,
0x0000,
0x0000,

/* Character 71 (0x47):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | *****  |
   |**   ** |
   |**   ** |
   |**      |
   |**      |
   |**      |
   |**  *** |
   |**   ** |
   |**   ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x7c00,
0xc600,
0xc600,
0xc000,
0xc000,
0xc000,
0xce00,
0xc600,
0xc600,
0x7c00,
0x0000,
0x0000,

/* Character 72 (0x48):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |******* |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0xc600,
0xc600,
0xc600,
0xc600,
0xfe00,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0x0000,
0x0000,

/* Character 73 (0x49):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |  ****  |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |  ****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x3c00,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x3c00,
0x0000,
0x0000,

/* Character 74 (0x4a):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |    *** |
   |     ** |
   |     ** |
   |     ** |
   |     ** |
   |     ** |
   |     ** |
   |**   ** |
   |**   ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0e00,
0x0600,
0x0600,
0x0600,
0x0600,
0x0600,
0x0600,
0xc600,
0xc600,
0x7c00,
0x0000,
0x0000,

/* Character 75 (0x4b):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |**   ** |
   |**   ** |
   |**  **  |
   |** **   |
   |****    |
   |****    |
   |** **   |
   |**  **  |
   |**   ** |
   |**   ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0xc600,
0xc600,
0xcc00,
0xd800,
0xf000,
0xf000,
0xd800,
0xcc00,
0xc600,
0xc600,
0x0000,
0x0000,

/* Character 76 (0x4c):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |**      |
   |**      |
   |**      |
   |**      |
   |**      |
   |**      |
   |**      |
   |**      |
   |**    * |
   |******* |
   |        |
   |        |
   +--------+
*/
0x0000,
0xc000,
0xc000,
0xc000,
0xc000,
0xc000,
0xc000,
0xc000,
0xc000,
0xc200,
0xfe00,
0x0000,
0x0000,

/* Character 77 (0x4d):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |**   ** |
   |**   ** |
   |*** *** |
   |******* |
   |** * ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0xc600,
0xc600,
0xee00,
0xfe00,
0xd600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0x0000,
0x0000,

/* Character 78 (0x4e):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |**   ** |
   |**   ** |
   |***  ** |
   |***  ** |
   |**** ** |
   |** **** |
   |**  *** |
   |**  *** |
   |**   ** |
   |**   ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0xc600,
0xc600,
0xe600,
0xe600,
0xf600,
0xde00,
0xce00,
0xce00,
0xc600,
0xc600,
0x0000,
0x0000,

/* Character 79 (0x4f):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | *****  |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x7c00,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0x7c00,
0x0000,
0x0000,

/* Character 80 (0x50):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |******  |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |******  |
   |**      |
   |**      |
   |**      |
   |**      |
   |        |
   |        |
   +--------+
*/
0x0000,
0xfc00,
0xc600,
0xc600,
0xc600,
0xc600,
0xfc00,
0xc000,
0xc000,
0xc000,
0xc000,
0x0000,
0x0000,

/* Character 81 (0x51):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | *****  |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |** **** |
   | *****  |
   |     ** |
   |        |
   +--------+
*/
0x0000,
0x7c00,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xde00,
0x7c00,
0x0600,
0x0000,

/* Character 82 (0x52):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |******  |
   |**   ** |
   |**   ** |
   |**   ** |
   |******  |
   |*****   |
   |**  **  |
   |**  **  |
   |**   ** |
   |**   ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0xfc00,
0xc600,
0xc600,
0xc600,
0xfc00,
0xf800,
0xcc00,
0xcc00,
0xc600,
0xc600,
0x0000,
0x0000,

/* Character 83 (0x53):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | *****  |
   |**   ** |
   |**   ** |
   |**      |
   | *****  |
   |     ** |
   |     ** |
   |**   ** |
   |**   ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x7c00,
0xc600,
0xc600,
0xc000,
0x7c00,
0x0600,
0x0600,
0xc600,
0xc600,
0x7c00,
0x0000,
0x0000,

/* Character 84 (0x54):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | ****** |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |        |
   |        |
   +--------+
*/
0x0000,
0x7e00,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x0000,
0x0000,

/* Character 85 (0x55):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0x7c00,
0x0000,
0x0000,

/* Character 86 (0x56):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   | *   *  |
   | ** **  |
   | ** **  |
   |  ***   |
   |  ***   |
   |   *    |
   |        |
   |        |
   +--------+
*/
0x0000,
0xc600,
0xc600,
0xc600,
0xc600,
0x4400,
0x6c00,
0x6c00,
0x3800,
0x3800,
0x1000,
0x0000,
0x0000,

/* Character 87 (0x57):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |** * ** |
   |** * ** |
   |******* |
   | ** **  |
   |        |
   |        |
   +--------+
*/
0x0000,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xd600,
0xd600,
0xfe00,
0x6c00,
0x0000,
0x0000,

/* Character 88 (0x58):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |**   ** |
   |**   ** |
   | ** **  |
   | ** **  |
   |  ***   |
   |  ***   |
   | ** **  |
   | ** **  |
   |**   ** |
   |**   ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0xc600,
0xc600,
0x6c00,
0x6c00,
0x3800,
0x3800,
0x6c00,
0x6c00,
0xc600,
0xc600,
0x0000,
0x0000,

/* Character 89 (0x59):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | **  ** |
   | **  ** |
   | **  ** |
   |  ****  |
   |  ****  |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |        |
   |        |
   +--------+
*/
0x0000,
0x6600,
0x6600,
0x6600,
0x3c00,
0x3c00,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x0000,
0x0000,

/* Character 90 (0x5a):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |******* |
   |     ** |
   |     ** |
   |    **  |
   |   **   |
   |  **    |
   | **     |
   |**      |
   |**      |
   |******* |
   |        |
   |        |
   +--------+
*/
0x0000,
0xfe00,
0x0600,
0x0600,
0x0c00,
0x1800,
0x3000,
0x6000,
0xc000,
0xc000,
0xfe00,
0x0000,
0x0000,

/* Character 91 (0x5b):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | *****  |
   | **     |
   | **     |
   | **     |
   | **     |
   | **     |
   | **     |
   | **     |
   | **     |
   | **     |
   | *****  |
   |        |
   +--------+
*/
0x0000,
0x7c00,
0x6000,
0x6000,
0x6000,
0x6000,
0x6000,
0x6000,
0x6000,
0x6000,
0x6000,
0x7c00,
0x0000,

/* Character 92 (0x5c):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |*       |
   |**      |
   |**      |
   | **     |
   |  **    |
   |   **   |
   |    **  |
   |     ** |
   |     ** |
   |      * |
   |        |
   |        |
   +--------+
*/
0x0000,
0x8000,
0xc000,
0xc000,
0x6000,
0x3000,
0x1800,
0x0c00,
0x0600,
0x0600,
0x0200,
0x0000,
0x0000,

/* Character 93 (0x5d):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | *****  |
   |    **  |
   |    **  |
   |    **  |
   |    **  |
   |    **  |
   |    **  |
   |    **  |
   |    **  |
   |    **  |
   | *****  |
   |        |
   +--------+
*/
0x0000,
0x7c00,
0x0c00,
0x0c00,
0x0c00,
0x0c00,
0x0c00,
0x0c00,
0x0c00,
0x0c00,
0x0c00,
0x7c00,
0x0000,

/* Character 94 (0x5e):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |   *    |
   |  ***   |
   | ** **  |
   |**   ** |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   +--------+
*/
0x0000,
0x1000,
0x3800,
0x6c00,
0xc600,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 95 (0x5f):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |******* |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0xfe00,
0x0000,

/* Character 96 (0x60):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |  **    |
   |   **   |
   |    **  |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   +--------+
*/
0x0000,
0x3000,
0x1800,
0x0c00,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 97 (0x61):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   | *****  |
   |     ** |
   | ****** |
   |**   ** |
   |**   ** |
   |**  *** |
   | *** ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x7c00,
0x0600,
0x7e00,
0xc600,
0xc600,
0xce00,
0x7600,
0x0000,
0x0000,

/* Character 98 (0x62):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |**      |
   |**      |
   |**      |
   |** ***  |
   |***  ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |***  ** |
   |** ***  |
   |        |
   |        |
   +--------+
*/
0x0000,
0xc000,
0xc000,
0xc000,
0xdc00,
0xe600,
0xc600,
0xc600,
0xc600,
0xe600,
0xdc00,
0x0000,
0x0000,

/* Character 99 (0x63):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   | *****  |
   |***  ** |
   |**      |
   |**      |
   |**      |
   |***  ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x7c00,
0xe600,
0xc000,
0xc000,
0xc000,
0xe600,
0x7c00,
0x0000,
0x0000,

/* Character 100 (0x64):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |     ** |
   |     ** |
   |     ** |
   | *** ** |
   |**  *** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**  *** |
   | *** ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0600,
0x0600,
0x0600,
0x7600,
0xce00,
0xc600,
0xc600,
0xc600,
0xce00,
0x7600,
0x0000,
0x0000,

/* Character 101 (0x65):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   | *****  |
   |**   ** |
   |**   ** |
   |******* |
   |**      |
   |**   ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x7c00,
0xc600,
0xc600,
0xfe00,
0xc000,
0xc600,
0x7c00,
0x0000,
0x0000,

/* Character 102 (0x66):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |  ****  |
   | **  ** |
   | **     |
   | **     |
   | **     |
   |******  |
   | **     |
   | **     |
   | **     |
   | **     |
   |        |
   |        |
   +--------+
*/
0x0000,
0x3c00,
0x6600,
0x6000,
0x6000,
0x6000,
0xfc00,
0x6000,
0x6000,
0x6000,
0x6000,
0x0000,
0x0000,

/* Character 103 (0x67):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   | ****** |
   |**  **  |
   |**  **  |
   |**  **  |
   | ****   |
   |****    |
   | *****  |
   |**   ** |
   | *****  |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x7e00,
0xcc00,
0xcc00,
0xcc00,
0x7800,
0xf000,
0x7c00,
0xc600,
0x7c00,

/* Character 104 (0x68):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |**      |
   |**      |
   |**      |
   |** ***  |
   |***  ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0xc000,
0xc000,
0xc000,
0xdc00,
0xe600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0x0000,
0x0000,

/* Character 105 (0x69):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |   **   |
   |   **   |
   |        |
   |  ***   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |  ****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x1800,
0x1800,
0x0000,
0x3800,
0x1800,
0x1800,
0x1800,
0x1800,
0x3c00,
0x0000,
0x0000,

/* Character 106 (0x6a):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |     ** |
   |     ** |
   |        |
   |    *** |
   |     ** |
   |     ** |
   |     ** |
   |     ** |
   |**   ** |
   |**   ** |
   | *****  |
   +--------+
*/
0x0000,
0x0000,
0x0600,
0x0600,
0x0000,
0x0e00,
0x0600,
0x0600,
0x0600,
0x0600,
0xc600,
0xc600,
0x7c00,

/* Character 107 (0x6b):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |**      |
   |**      |
   |**      |
   |**  **  |
   |** **   |
   |****    |
   |****    |
   |** **   |
   |**  **  |
   |**   ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0xc000,
0xc000,
0xc000,
0xcc00,
0xd800,
0xf000,
0xf000,
0xd800,
0xcc00,
0xc600,
0x0000,
0x0000,

/* Character 108 (0x6c):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |  ***   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |  ****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x3800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x3c00,
0x0000,
0x0000,

/* Character 109 (0x6d):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   | ** **  |
   |******* |
   |** * ** |
   |** * ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x6c00,
0xfe00,
0xd600,
0xd600,
0xc600,
0xc600,
0xc600,
0x0000,
0x0000,

/* Character 110 (0x6e):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |** ***  |
   |***  ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0xdc00,
0xe600,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0x0000,
0x0000,

/* Character 111 (0x6f):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   | *****  |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x7c00,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0x7c00,
0x0000,
0x0000,

/* Character 112 (0x70):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |** ***  |
   |***  ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |***  ** |
   |** ***  |
   |**      |
   |**      |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0xdc00,
0xe600,
0xc600,
0xc600,
0xc600,
0xe600,
0xdc00,
0xc000,
0xc000,

/* Character 113 (0x71):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   | *** ** |
   |**  *** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**  *** |
   | *** ** |
   |     ** |
   |     ** |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x7600,
0xce00,
0xc600,
0xc600,
0xc600,
0xce00,
0x7600,
0x0600,
0x0600,

/* Character 114 (0x72):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |** ***  |
   |***  ** |
   |**      |
   |**      |
   |**      |
   |**      |
   |**      |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0xdc00,
0xe600,
0xc000,
0xc000,
0xc000,
0xc000,
0xc000,
0x0000,
0x0000,

/* Character 115 (0x73):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   | *****  |
   |**   ** |
   | **     |
   |  ***   |
   |    **  |
   |**   ** |
   | *****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x7c00,
0xc600,
0x6000,
0x3800,
0x0c00,
0xc600,
0x7c00,
0x0000,
0x0000,

/* Character 116 (0x74):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | **     |
   | **     |
   | **     |
   | **     |
   |******  |
   | **     |
   | **     |
   | **     |
   | **  ** |
   |  ****  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x6000,
0x6000,
0x6000,
0x6000,
0xfc00,
0x6000,
0x6000,
0x6000,
0x6600,
0x3c00,
0x0000,
0x0000,

/* Character 117 (0x75):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**  *** |
   | *** ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0xc600,
0xc600,
0xc600,
0xc600,
0xc600,
0xce00,
0x7600,
0x0000,
0x0000,

/* Character 118 (0x76):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   | ** **  |
   | ** **  |
   |  ***   |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0xc600,
0xc600,
0xc600,
0xc600,
0x6c00,
0x6c00,
0x3800,
0x0000,
0x0000,

/* Character 119 (0x77):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |**   ** |
   |**   ** |
   |**   ** |
   |** * ** |
   |** * ** |
   |******* |
   | ** **  |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0xc600,
0xc600,
0xc600,
0xd600,
0xd600,
0xfe00,
0x6c00,
0x0000,
0x0000,

/* Character 120 (0x78):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |**   ** |
   |**   ** |
   | ** **  |
   |  ***   |
   | ** **  |
   |**   ** |
   |**   ** |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0xc600,
0xc600,
0x6c00,
0x3800,
0x6c00,
0xc600,
0xc600,
0x0000,
0x0000,

/* Character 121 (0x79):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |**   ** |
   |**   ** |
   |**   ** |
   |**   ** |
   |**  *** |
   | *** ** |
   |     ** |
   |**   ** |
   | *****  |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0xc600,
0xc600,
0xc600,
0xc600,
0xce00,
0x7600,
0x0600,
0xc600,
0x7c00,

/* Character 122 (0x7a):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   |        |
   |        |
   |******* |
   |    **  |
   |   **   |
   |  **    |
   | **     |
   |**      |
   |******* |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0xfe00,
0x0c00,
0x1800,
0x3000,
0x6000,
0xc000,
0xfe00,
0x0000,
0x0000,

/* Character 123 (0x7b):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |   **** |
   |  **    |
   |  **    |
   |  **    |
   |   **   |
   | ***    |
   |   **   |
   |  **    |
   |  **    |
   |  **    |
   |   **** |
   |        |
   +--------+
*/
0x0000,
0x1e00,
0x3000,
0x3000,
0x3000,
0x1800,
0x7000,
0x1800,
0x3000,
0x3000,
0x3000,
0x1e00,
0x0000,

/* Character 124 (0x7c):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |   **   |
   |        |
   |        |
   +--------+
*/
0x0000,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x1800,
0x0000,
0x0000,

/* Character 125 (0x7d):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   | ****   |
   |    **  |
   |    **  |
   |    **  |
   |   **   |
   |    *** |
   |   **   |
   |    **  |
   |    **  |
   |    **  |
   | ****   |
   |        |
   +--------+
*/
0x0000,
0x7800,
0x0c00,
0x0c00,
0x0c00,
0x1800,
0x0e00,
0x1800,
0x0c00,
0x0c00,
0x0c00,
0x7800,
0x0000,

/* Character 126 (0x7e):
   width 8
   bbx ( 8, 13, 0, -2 )

   +--------+
   |        |
   |        |
   | ***  * |
   |******* |
   |*  ***  |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   |        |
   +--------+
*/
0x0000,
0x0000,
0x7200,
0xfe00,
0x9c00,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
};

/* Exported structure definition. */
static const FontDesc consoleBDesc = {
  "8x13B-ISO8859-1",
  8,
  13,
  8, 13, 0, -2,
  11,
  28,
  99,
  consoleB_font_bits,
  nullptr,  /* no encode table*/
  nullptr,  /* fixed width*/
  nullptr,  /* fixed bbox*/
  32,                       // Originally 30
  sizeof(consoleB_font_bits)/sizeof(uInt16)
};

} // End of namespace GUI

#endif
