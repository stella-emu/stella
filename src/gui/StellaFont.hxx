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
// Generated by src/tools/convbdf on Wed Jul 31 13:02:19 2013.
//============================================================================

#ifndef STELLA_FONT_DATA_HXX
#define STELLA_FONT_DATA_HXX

#include "Font.hxx"

/* Font information:
   name: 6x10-ISO8859-1
   facename: -Misc-Fixed-Medium-R-Normal--10-100-75-75-C-60-ISO8859-1
   w x h: 6x10
   bbx: 6 10 0 -2
   size: 95
   ascent: 8
   descent: 2
   first char: 29 (0x1d)
   last char: 126 (0x7e)
   default char: 32 (0x20)
   proportional: no
   Public domain terminal emulator font.  Share and enjoy.
*/

namespace GUI {

// Font character bitmap data.
static const uInt16 stella_font_bits[] = {  // NOLINT : too complicated to convert

  /* Character 29 (0x1d):
  width 6
  bbx ( 6, 10, 0, -2 )

  +------+
  |      |
  |      |
  |      |
  |      |
  |      |
  |      |
  |      |
  |* * * |
  |      |
  |      |
  +------+
  */
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0b1010100000000000,
  0x0000,
  0x0000,

  /* Character 30 (0x1e):
  width 6
  bbx ( 6, 10, 0, -2 )

  +------+
  |      |
  |      |
  |      |
  |      |
  |      |
  |      |
  |      |
  |      |
  |      |
  |      |
  +------+
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

  /* Character 31 (0x1f):
  width 6
  bbx ( 6, 10, 0, -2 )

  +------+
  |      |
  |      |
  |      |
  |      |
  |      |
  |      |
  |      |
  |      |
  |      |
  |      |
  +------+
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

/* Character 32 (0x20):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   +------+
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

/* Character 33 (0x21):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   |      |
   |  *   |
   |      |
   |      |
   +------+
*/
0x0000,
0x2000,
0x2000,
0x2000,
0x2000,
0x2000,
0x0000,
0x2000,
0x0000,
0x0000,

/* Character 34 (0x22):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | * *  |
   | * *  |
   | * *  |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   +------+
*/
0x0000,
0x5000,
0x5000,
0x5000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 35 (0x23):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | * *  |
   | * *  |
   |***** |
   | * *  |
   |***** |
   | * *  |
   | * *  |
   |      |
   |      |
   +------+
*/
0x0000,
0x5000,
0x5000,
0xf800,
0x5000,
0xf800,
0x5000,
0x5000,
0x0000,
0x0000,

/* Character 36 (0x24):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |  *   |
   | ***  |
   |* *   |
   | ***  |
   |  * * |
   | ***  |
   |  *   |
   |      |
   |      |
   +------+
*/
0x0000,
0x2000,
0x7000,
0xa000,
0x7000,
0x2800,
0x7000,
0x2000,
0x0000,
0x0000,

/* Character 37 (0x25):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | *  * |
   |* * * |
   | * *  |
   |  *   |
   | * *  |
   |* * * |
   |*  *  |
   |      |
   |      |
   +------+
*/
0x0000,
0x4800,
0xa800,
0x5000,
0x2000,
0x5000,
0xa800,
0x9000,
0x0000,
0x0000,

/* Character 38 (0x26):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | *    |
   |* *   |
   |* *   |
   | *    |
   |* * * |
   |*  *  |
   | ** * |
   |      |
   |      |
   +------+
*/
0x0000,
0x4000,
0xa000,
0xa000,
0x4000,
0xa800,
0x9000,
0x6800,
0x0000,
0x0000,

/* Character 39 (0x27):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |  *   |
   |  *   |
   |  *   |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   +------+
*/
0x0000,
0x2000,
0x2000,
0x2000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 40 (0x28):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |   *  |
   |  *   |
   | *    |
   | *    |
   | *    |
   |  *   |
   |   *  |
   |      |
   |      |
   +------+
*/
0x0000,
0x1000,
0x2000,
0x4000,
0x4000,
0x4000,
0x2000,
0x1000,
0x0000,
0x0000,

/* Character 41 (0x29):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | *    |
   |  *   |
   |   *  |
   |   *  |
   |   *  |
   |  *   |
   | *    |
   |      |
   |      |
   +------+
*/
0x0000,
0x4000,
0x2000,
0x1000,
0x1000,
0x1000,
0x2000,
0x4000,
0x0000,
0x0000,

/* Character 42 (0x2a):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |*   * |
   | * *  |
   |***** |
   | * *  |
   |*   * |
   |      |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x8800,
0x5000,
0xf800,
0x5000,
0x8800,
0x0000,
0x0000,
0x0000,

/* Character 43 (0x2b):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |  *   |
   |  *   |
   |***** |
   |  *   |
   |  *   |
   |      |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x2000,
0x2000,
0xf800,
0x2000,
0x2000,
0x0000,
0x0000,
0x0000,

/* Character 44 (0x2c):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   |  **  |
   |  *   |
   | *    |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x3000,
0x2000,
0x4000,
0x0000,

/* Character 45 (0x2d):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |      |
   |***** |
   |      |
   |      |
   |      |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0xf800,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 46 (0x2e):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   | **   |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x6000,
0x0000,
0x0000,

/* Character 47 (0x2f):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |    * |
   |    * |
   |   *  |
   |  *   |
   | *    |
   |*     |
   |*     |
   |      |
   |      |
   +------+
*/
0x0000,
0x0800,
0x0800,
0x1000,
0x2000,
0x4000,
0x8000,
0x8000,
0x0000,
0x0000,

/* Character 48 (0x30):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |  *   |
   | * *  |
   |*   * |
   |*   * |
   |*   * |
   | * *  |
   |  *   |
   |      |
   |      |
   +------+
*/
0x0000,
0x2000,
0x5000,
0x8800,
0x8800,
0x8800,
0x5000,
0x2000,
0x0000,
0x0000,

/* Character 49 (0x31):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |  *   |
   | **   |
   |* *   |
   |  *   |
   |  *   |
   |  *   |
   |***** |
   |      |
   |      |
   +------+
*/
0x0000,
0x2000,
0x6000,
0xa000,
0x2000,
0x2000,
0x2000,
0xf800,
0x0000,
0x0000,

/* Character 50 (0x32):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   |*   * |
   |    * |
   |  **  |
   | *    |
   |*     |
   |***** |
   |      |
   |      |
   +------+
*/
0x0000,
0x7000,
0x8800,
0x0800,
0x3000,
0x4000,
0x8000,
0xf800,
0x0000,
0x0000,

/* Character 51 (0x33):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |***** |
   |    * |
   |   *  |
   |  **  |
   |    * |
   |*   * |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0xf800,
0x0800,
0x1000,
0x3000,
0x0800,
0x8800,
0x7000,
0x0000,
0x0000,

/* Character 52 (0x34):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |   *  |
   |  **  |
   | * *  |
   |*  *  |
   |***** |
   |   *  |
   |   *  |
   |      |
   |      |
   +------+
*/
0x0000,
0x1000,
0x3000,
0x5000,
0x9000,
0xf800,
0x1000,
0x1000,
0x0000,
0x0000,

/* Character 53 (0x35):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |***** |
   |*     |
   |* **  |
   |**  * |
   |    * |
   |*   * |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0xf800,
0x8000,
0xb000,
0xc800,
0x0800,
0x8800,
0x7000,
0x0000,
0x0000,

/* Character 54 (0x36):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |  **  |
   | *    |
   |*     |
   |* **  |
   |**  * |
   |*   * |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x3000,
0x4000,
0x8000,
0xb000,
0xc800,
0x8800,
0x7000,
0x0000,
0x0000,

/* Character 55 (0x37):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |***** |
   |    * |
   |   *  |
   |   *  |
   |  *   |
   | *    |
   | *    |
   |      |
   |      |
   +------+
*/
0x0000,
0xf800,
0x0800,
0x1000,
0x1000,
0x2000,
0x4000,
0x4000,
0x0000,
0x0000,

/* Character 56 (0x38):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   |*   * |
   |*   * |
   | ***  |
   |*   * |
   |*   * |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x7000,
0x8800,
0x8800,
0x7000,
0x8800,
0x8800,
0x7000,
0x0000,
0x0000,

/* Character 57 (0x39):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   |*   * |
   |*  ** |
   | ** * |
   |    * |
   |   *  |
   | **   |
   |      |
   |      |
   +------+
*/
0x0000,
0x7000,
0x8800,
0x9800,
0x6800,
0x0800,
0x1000,
0x6000,
0x0000,
0x0000,

/* Character 58 (0x3a):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   | **   |
   |      |
   |      |
   |      |
   | **   |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x6000,
0x0000,
0x0000,
0x0000,
0x6000,
0x0000,
0x0000,

/* Character 59 (0x3b):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |  **  |
   |      |
   |      |
   |  **  |
   |  *   |
   | *    |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x3000,
0x0000,
0x0000,
0x3000,
0x2000,
0x4000,
0x0000,

/* Character 60 (0x3c):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |    * |
   |   *  |
   |  *   |
   | *    |
   |  *   |
   |   *  |
   |    * |
   |      |
   |      |
   +------+
*/
0x0000,
0x0800,
0x1000,
0x2000,
0x4000,
0x2000,
0x1000,
0x0800,
0x0000,
0x0000,

/* Character 61 (0x3d):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |***** |
   |      |
   |***** |
   |      |
   |      |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0xf800,
0x0000,
0xf800,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 62 (0x3e):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | *    |
   |  *   |
   |   *  |
   |    * |
   |   *  |
   |  *   |
   | *    |
   |      |
   |      |
   +------+
*/
0x0000,
0x4000,
0x2000,
0x1000,
0x0800,
0x1000,
0x2000,
0x4000,
0x0000,
0x0000,

/* Character 63 (0x3f):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   |*   * |
   |   *  |
   |  *   |
   |  *   |
   |      |
   |  *   |
   |      |
   |      |
   +------+
*/
0x0000,
0x7000,
0x8800,
0x1000,
0x2000,
0x2000,
0x0000,
0x2000,
0x0000,
0x0000,

/* Character 64 (0x40):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   |*   * |
   |*  ** |
   |* * * |
   |* **  |
   |*     |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x7000,
0x8800,
0x9800,
0xa800,
0xb000,
0x8000,
0x7000,
0x0000,
0x0000,

/* Character 65 (0x41):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |  *   |
   | * *  |
   |*   * |
   |*   * |
   |***** |
   |*   * |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0x2000,
0x5000,
0x8800,
0x8800,
0xf800,
0x8800,
0x8800,
0x0000,
0x0000,

/* Character 66 (0x42):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |****  |
   | *  * |
   | *  * |
   | ***  |
   | *  * |
   | *  * |
   |****  |
   |      |
   |      |
   +------+
*/
0x0000,
0xf000,
0x4800,
0x4800,
0x7000,
0x4800,
0x4800,
0xf000,
0x0000,
0x0000,

/* Character 67 (0x43):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   |*   * |
   |*     |
   |*     |
   |*     |
   |*   * |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x7000,
0x8800,
0x8000,
0x8000,
0x8000,
0x8800,
0x7000,
0x0000,
0x0000,

/* Character 68 (0x44):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |****  |
   | *  * |
   | *  * |
   | *  * |
   | *  * |
   | *  * |
   |****  |
   |      |
   |      |
   +------+
*/
0x0000,
0xf000,
0x4800,
0x4800,
0x4800,
0x4800,
0x4800,
0xf000,
0x0000,
0x0000,

/* Character 69 (0x45):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |***** |
   |*     |
   |*     |
   |****  |
   |*     |
   |*     |
   |***** |
   |      |
   |      |
   +------+
*/
0x0000,
0xf800,
0x8000,
0x8000,
0xf000,
0x8000,
0x8000,
0xf800,
0x0000,
0x0000,

/* Character 70 (0x46):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |***** |
   |*     |
   |*     |
   |****  |
   |*     |
   |*     |
   |*     |
   |      |
   |      |
   +------+
*/
0x0000,
0xf800,
0x8000,
0x8000,
0xf000,
0x8000,
0x8000,
0x8000,
0x0000,
0x0000,

/* Character 71 (0x47):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   |*   * |
   |*     |
   |*     |
   |*  ** |
   |*   * |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x7000,
0x8800,
0x8000,
0x8000,
0x9800,
0x8800,
0x7000,
0x0000,
0x0000,

/* Character 72 (0x48):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*   * |
   |*   * |
   |*   * |
   |***** |
   |*   * |
   |*   * |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0x8800,
0x8800,
0x8800,
0xf800,
0x8800,
0x8800,
0x8800,
0x0000,
0x0000,

/* Character 73 (0x49):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x7000,
0x2000,
0x2000,
0x2000,
0x2000,
0x2000,
0x7000,
0x0000,
0x0000,

/* Character 74 (0x4a):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |  *** |
   |   *  |
   |   *  |
   |   *  |
   |   *  |
   |*  *  |
   | **   |
   |      |
   |      |
   +------+
*/
0x0000,
0x3800,
0x1000,
0x1000,
0x1000,
0x1000,
0x9000,
0x6000,
0x0000,
0x0000,

/* Character 75 (0x4b):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*   * |
   |*  *  |
   |* *   |
   |**    |
   |* *   |
   |*  *  |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0x8800,
0x9000,
0xa000,
0xc000,
0xa000,
0x9000,
0x8800,
0x0000,
0x0000,

/* Character 76 (0x4c):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*     |
   |*     |
   |*     |
   |*     |
   |*     |
   |*     |
   |***** |
   |      |
   |      |
   +------+
*/
0x0000,
0x8000,
0x8000,
0x8000,
0x8000,
0x8000,
0x8000,
0xf800,
0x0000,
0x0000,

/* Character 77 (0x4d):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*   * |
   |*   * |
   |** ** |
   |* * * |
   |*   * |
   |*   * |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0x8800,
0x8800,
0xd800,
0xa800,
0x8800,
0x8800,
0x8800,
0x0000,
0x0000,

/* Character 78 (0x4e):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*   * |
   |*   * |
   |**  * |
   |* * * |
   |*  ** |
   |*   * |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0x8800,
0x8800,
0xc800,
0xa800,
0x9800,
0x8800,
0x8800,
0x0000,
0x0000,

/* Character 79 (0x4f):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   |*   * |
   |*   * |
   |*   * |
   |*   * |
   |*   * |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x7000,
0x8800,
0x8800,
0x8800,
0x8800,
0x8800,
0x7000,
0x0000,
0x0000,

/* Character 80 (0x50):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |****  |
   |*   * |
   |*   * |
   |****  |
   |*     |
   |*     |
   |*     |
   |      |
   |      |
   +------+
*/
0x0000,
0xf000,
0x8800,
0x8800,
0xf000,
0x8000,
0x8000,
0x8000,
0x0000,
0x0000,

/* Character 81 (0x51):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   |*   * |
   |*   * |
   |*   * |
   |*   * |
   |* * * |
   | ***  |
   |    * |
   |      |
   +------+
*/
0x0000,
0x7000,
0x8800,
0x8800,
0x8800,
0x8800,
0xa800,
0x7000,
0x0800,
0x0000,

/* Character 82 (0x52):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |****  |
   |*   * |
   |*   * |
   |****  |
   |* *   |
   |*  *  |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0xf000,
0x8800,
0x8800,
0xf000,
0xa000,
0x9000,
0x8800,
0x0000,
0x0000,

/* Character 83 (0x53):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   |*   * |
   |*     |
   | ***  |
   |    * |
   |*   * |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x7000,
0x8800,
0x8000,
0x7000,
0x0800,
0x8800,
0x7000,
0x0000,
0x0000,

/* Character 84 (0x54):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |***** |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   |      |
   |      |
   +------+
*/
0x0000,
0xf800,
0x2000,
0x2000,
0x2000,
0x2000,
0x2000,
0x2000,
0x0000,
0x0000,

/* Character 85 (0x55):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*   * |
   |*   * |
   |*   * |
   |*   * |
   |*   * |
   |*   * |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x8800,
0x8800,
0x8800,
0x8800,
0x8800,
0x8800,
0x7000,
0x0000,
0x0000,

/* Character 86 (0x56):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*   * |
   |*   * |
   |*   * |
   | * *  |
   | * *  |
   | * *  |
   |  *   |
   |      |
   |      |
   +------+
*/
0x0000,
0x8800,
0x8800,
0x8800,
0x5000,
0x5000,
0x5000,
0x2000,
0x0000,
0x0000,

/* Character 87 (0x57):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*   * |
   |*   * |
   |*   * |
   |* * * |
   |* * * |
   |** ** |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0x8800,
0x8800,
0x8800,
0xa800,
0xa800,
0xd800,
0x8800,
0x0000,
0x0000,

/* Character 88 (0x58):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*   * |
   |*   * |
   | * *  |
   |  *   |
   | * *  |
   |*   * |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0x8800,
0x8800,
0x5000,
0x2000,
0x5000,
0x8800,
0x8800,
0x0000,
0x0000,

/* Character 89 (0x59):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*   * |
   |*   * |
   | * *  |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   |      |
   |      |
   +------+
*/
0x0000,
0x8800,
0x8800,
0x5000,
0x2000,
0x2000,
0x2000,
0x2000,
0x0000,
0x0000,

/* Character 90 (0x5a):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |***** |
   |    * |
   |   *  |
   |  *   |
   | *    |
   |*     |
   |***** |
   |      |
   |      |
   +------+
*/
0x0000,
0xf800,
0x0800,
0x1000,
0x2000,
0x4000,
0x8000,
0xf800,
0x0000,
0x0000,

/* Character 91 (0x5b):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   | *    |
   | *    |
   | *    |
   | *    |
   | *    |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x7000,
0x4000,
0x4000,
0x4000,
0x4000,
0x4000,
0x7000,
0x0000,
0x0000,

/* Character 92 (0x5c):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*     |
   |*     |
   | *    |
   |  *   |
   |   *  |
   |    * |
   |    * |
   |      |
   |      |
   +------+
*/
0x0000,
0x8000,
0x8000,
0x4000,
0x2000,
0x1000,
0x0800,
0x0800,
0x0000,
0x0000,

/* Character 93 (0x5d):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | ***  |
   |   *  |
   |   *  |
   |   *  |
   |   *  |
   |   *  |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x7000,
0x1000,
0x1000,
0x1000,
0x1000,
0x1000,
0x7000,
0x0000,
0x0000,

/* Character 94 (0x5e):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |  *   |
   | * *  |
   |*   * |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   +------+
*/
0x0000,
0x2000,
0x5000,
0x8800,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 95 (0x5f):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   |***** |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0xf800,
0x0000,

/* Character 96 (0x60):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |  *   |
   |   *  |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   +------+
*/
0x2000,
0x1000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,

/* Character 97 (0x61):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   | ***  |
   |    * |
   | **** |
   |*   * |
   | **** |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x7000,
0x0800,
0x7800,
0x8800,
0x7800,
0x0000,
0x0000,

/* Character 98 (0x62):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*     |
   |*     |
   |* **  |
   |**  * |
   |*   * |
   |**  * |
   |* **  |
   |      |
   |      |
   +------+
*/
0x0000,
0x8000,
0x8000,
0xb000,
0xc800,
0x8800,
0xc800,
0xb000,
0x0000,
0x0000,

/* Character 99 (0x63):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   | ***  |
   |*   * |
   |*     |
   |*   * |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x7000,
0x8800,
0x8000,
0x8800,
0x7000,
0x0000,
0x0000,

/* Character 100 (0x64):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |    * |
   |    * |
   | ** * |
   |*  ** |
   |*   * |
   |*  ** |
   | ** * |
   |      |
   |      |
   +------+
*/
0x0000,
0x0800,
0x0800,
0x6800,
0x9800,
0x8800,
0x9800,
0x6800,
0x0000,
0x0000,

/* Character 101 (0x65):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   | ***  |
   |*   * |
   |***** |
   |*     |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x7000,
0x8800,
0xf800,
0x8000,
0x7000,
0x0000,
0x0000,

/* Character 102 (0x66):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |  **  |
   | *  * |
   | *    |
   |****  |
   | *    |
   | *    |
   | *    |
   |      |
   |      |
   +------+
*/
0x0000,
0x3000,
0x4800,
0x4000,
0xf000,
0x4000,
0x4000,
0x4000,
0x0000,
0x0000,

/* Character 103 (0x67):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   | **** |
   |*   * |
   |*   * |
   | **** |
   |    * |
   |*   * |
   | ***  |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x7800,
0x8800,
0x8800,
0x7800,
0x0800,
0x8800,
0x7000,

/* Character 104 (0x68):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*     |
   |*     |
   |* **  |
   |**  * |
   |*   * |
   |*   * |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0x8000,
0x8000,
0xb000,
0xc800,
0x8800,
0x8800,
0x8800,
0x0000,
0x0000,

/* Character 105 (0x69):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |  *   |
   |      |
   | **   |
   |  *   |
   |  *   |
   |  *   |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x2000,
0x0000,
0x6000,
0x2000,
0x2000,
0x2000,
0x7000,
0x0000,
0x0000,

/* Character 106 (0x6a):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |    * |
   |      |
   |   ** |
   |    * |
   |    * |
   |    * |
   | *  * |
   | *  * |
   |  **  |
   +------+
*/
0x0000,
0x0800,
0x0000,
0x1800,
0x0800,
0x0800,
0x0800,
0x4800,
0x4800,
0x3000,

/* Character 107 (0x6b):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |*     |
   |*     |
   |*   * |
   |*  *  |
   |***   |
   |*  *  |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0x8000,
0x8000,
0x8800,
0x9000,
0xe000,
0x9000,
0x8800,
0x0000,
0x0000,

/* Character 108 (0x6c):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | **   |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x6000,
0x2000,
0x2000,
0x2000,
0x2000,
0x2000,
0x7000,
0x0000,
0x0000,

/* Character 109 (0x6d):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |** *  |
   |* * * |
   |* * * |
   |* * * |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0xd000,
0xa800,
0xa800,
0xa800,
0x8800,
0x0000,
0x0000,

/* Character 110 (0x6e):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |* **  |
   |**  * |
   |*   * |
   |*   * |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0xb000,
0xc800,
0x8800,
0x8800,
0x8800,
0x0000,
0x0000,

/* Character 111 (0x6f):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   | ***  |
   |*   * |
   |*   * |
   |*   * |
   | ***  |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x7000,
0x8800,
0x8800,
0x8800,
0x7000,
0x0000,
0x0000,

/* Character 112 (0x70):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |* **  |
   |**  * |
   |*   * |
   |**  * |
   |* **  |
   |*     |
   |*     |
   +------+
*/
0x0000,
0x0000,
0x0000,
0xb000,
0xc800,
0x8800,
0xc800,
0xb000,
0x8000,
0x8000,

/* Character 113 (0x71):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   | ** * |
   |*  ** |
   |*   * |
   |*  ** |
   | ** * |
   |    * |
   |    * |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x6800,
0x9800,
0x8800,
0x9800,
0x6800,
0x0800,
0x0800,

/* Character 114 (0x72):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |* **  |
   |**  * |
   |*     |
   |*     |
   |*     |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0xb000,
0xc800,
0x8000,
0x8000,
0x8000,
0x0000,
0x0000,

/* Character 115 (0x73):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   | ***  |
   |*     |
   | ***  |
   |    * |
   |****  |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x7000,
0x8000,
0x7000,
0x0800,
0xf000,
0x0000,
0x0000,

/* Character 116 (0x74):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | *    |
   | *    |
   |****  |
   | *    |
   | *    |
   | *  * |
   |  **  |
   |      |
   |      |
   +------+
*/
0x0000,
0x4000,
0x4000,
0xf000,
0x4000,
0x4000,
0x4800,
0x3000,
0x0000,
0x0000,

/* Character 117 (0x75):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |*   * |
   |*   * |
   |*   * |
   |*  ** |
   | ** * |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x8800,
0x8800,
0x8800,
0x9800,
0x6800,
0x0000,
0x0000,

/* Character 118 (0x76):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |*   * |
   |*   * |
   | * *  |
   | * *  |
   |  *   |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x8800,
0x8800,
0x5000,
0x5000,
0x2000,
0x0000,
0x0000,

/* Character 119 (0x77):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |*   * |
   |*   * |
   |* * * |
   |* * * |
   | * *  |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x8800,
0x8800,
0xa800,
0xa800,
0x5000,
0x0000,
0x0000,

/* Character 120 (0x78):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |*   * |
   | * *  |
   |  *   |
   | * *  |
   |*   * |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x8800,
0x5000,
0x2000,
0x5000,
0x8800,
0x0000,
0x0000,

/* Character 121 (0x79):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |*   * |
   |*   * |
   |*  ** |
   | ** * |
   |    * |
   |*   * |
   | ***  |
   +------+
*/
0x0000,
0x0000,
0x0000,
0x8800,
0x8800,
0x9800,
0x6800,
0x0800,
0x8800,
0x7000,

/* Character 122 (0x7a):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |      |
   |      |
   |***** |
   |   *  |
   |  *   |
   | *    |
   |***** |
   |      |
   |      |
   +------+
*/
0x0000,
0x0000,
0x0000,
0xf800,
0x1000,
0x2000,
0x4000,
0xf800,
0x0000,
0x0000,

/* Character 123 (0x7b):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |   ** |
   |  *   |
   |   *  |
   | **   |
   |   *  |
   |  *   |
   |   ** |
   |      |
   |      |
   +------+
*/
0x0000,
0x1800,
0x2000,
0x1000,
0x6000,
0x1000,
0x2000,
0x1800,
0x0000,
0x0000,

/* Character 124 (0x7c):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   |  *   |
   |      |
   |      |
   +------+
*/
0x0000,
0x2000,
0x2000,
0x2000,
0x2000,
0x2000,
0x2000,
0x2000,
0x0000,
0x0000,

/* Character 125 (0x7d):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | **   |
   |   *  |
   |  *   |
   |   ** |
   |  *   |
   |   *  |
   | **   |
   |      |
   |      |
   +------+
*/
0x0000,
0x6000,
0x1000,
0x2000,
0x1800,
0x2000,
0x1000,
0x6000,
0x0000,
0x0000,

/* Character 126 (0x7e):
   width 6
   bbx ( 6, 10, 0, -2 )

   +------+
   |      |
   | *  * |
   |* * * |
   |*  *  |
   |      |
   |      |
   |      |
   |      |
   |      |
   |      |
   +------+
*/
0x0000,
0x4800,
0xa800,
0x9000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
0x0000,
};

/* Exported structure definition. */
static const FontDesc stellaDesc = {
  "6x10-ISO8859-1",
  6,
  10,
  6, 10, 0, -2,
  8,
  29,
  98,
  stella_font_bits,
  nullptr,  /* no encode table*/
  nullptr,  /* fixed width*/
  nullptr,  /* fixed bbox*/
  32,
  sizeof(stella_font_bits)/sizeof(uInt16)
};

} // End of namespace GUI

#endif
