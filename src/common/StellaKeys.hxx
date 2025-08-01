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
//============================================================================

#ifndef STELLA_KEYS_HXX
#define STELLA_KEYS_HXX

#ifdef SDL_SUPPORT
  #include "SDL_lib.hxx"
#endif

/**
  This class implements a thin wrapper around the SDL keysym enumerations,
  such that SDL-specific code doesn't have to go into the internal parts of
  the codebase.  The keycodes are exactly the same, but from the POV of the
  rest of the code, they are *KBD* (keyboard) keys, not *SDL* keys.

  If the codebase is ported to future SDL versions or to some other toolkit,
  the intent is to simply change this file without having to modify all
  other classes that use StellaKey.

  @author  Stephen Anthony
*/

// This comes directly from SDL_scancode.h
enum StellaKey  // NOLINT: use 32-bit, even though 16-bit is sufficient
{
    KBDK_UNKNOWN = 0,

    /**
     *  \name Usage page 0x07
     *
     *  These values are from usage page 0x07 (USB keyboard page).
     */
    /* @{ */

    KBDK_A = 4,
    KBDK_B = 5,
    KBDK_C = 6,
    KBDK_D = 7,
    KBDK_E = 8,
    KBDK_F = 9,
    KBDK_G = 10,
    KBDK_H = 11,
    KBDK_I = 12,
    KBDK_J = 13,
    KBDK_K = 14,
    KBDK_L = 15,
    KBDK_M = 16,
    KBDK_N = 17,
    KBDK_O = 18,
    KBDK_P = 19,
    KBDK_Q = 20,
    KBDK_R = 21,
    KBDK_S = 22,
    KBDK_T = 23,
    KBDK_U = 24,
    KBDK_V = 25,
    KBDK_W = 26,
    KBDK_X = 27,
    KBDK_Y = 28,
    KBDK_Z = 29,

    KBDK_1 = 30,
    KBDK_2 = 31,
    KBDK_3 = 32,
    KBDK_4 = 33,
    KBDK_5 = 34,
    KBDK_6 = 35,
    KBDK_7 = 36,
    KBDK_8 = 37,
    KBDK_9 = 38,
    KBDK_0 = 39,

    KBDK_RETURN = 40,
    KBDK_ESCAPE = 41,
    KBDK_BACKSPACE = 42,
    KBDK_TAB = 43,
    KBDK_SPACE = 44,

    KBDK_MINUS = 45,
    KBDK_EQUALS = 46,
    KBDK_LEFTBRACKET = 47,
    KBDK_RIGHTBRACKET = 48,
    KBDK_BACKSLASH = 49, /**< Located at the lower left of the return
                                  *   key on ISO keyboards and at the right end
                                  *   of the QWERTY row on ANSI keyboards.
                                  *   Produces REVERSE SOLIDUS (backslash) and
                                  *   VERTICAL LINE in a US layout, REVERSE
                                  *   SOLIDUS and VERTICAL LINE in a UK Mac
                                  *   layout, NUMBER SIGN and TILDE in a UK
                                  *   Windows layout, DOLLAR SIGN and POUND SIGN
                                  *   in a Swiss German layout, NUMBER SIGN and
                                  *   APOSTROPHE in a German layout, GRAVE
                                  *   ACCENT and POUND SIGN in a French Mac
                                  *   layout, and ASTERISK and MICRO SIGN in a
                                  *   French Windows layout.
                                  */
    KBDK_NONUSHASH = 50, /**< ISO USB keyboards actually use this code
                                  *   instead of 49 for the same key, but all
                                  *   OSes I've seen treat the two codes
                                  *   identically. So, as an implementor, unless
                                  *   your keyboard generates both of those
                                  *   codes and your OS treats them differently,
                                  *   you should generate KBDK_BACKSLASH
                                  *   instead of this code. As a user, you
                                  *   should not rely on this code because SDL
                                  *   will never generate it with most (all?)
                                  *   keyboards.
                                  */
    KBDK_SEMICOLON = 51,
    KBDK_APOSTROPHE = 52,
    KBDK_GRAVE = 53, /**< Located in the top left corner (on both ANSI
                              *   and ISO keyboards). Produces GRAVE ACCENT and
                              *   TILDE in a US Windows layout and in US and UK
                              *   Mac layouts on ANSI keyboards, GRAVE ACCENT
                              *   and NOT SIGN in a UK Windows layout, SECTION
                              *   SIGN and PLUS-MINUS SIGN in US and UK Mac
                              *   layouts on ISO keyboards, SECTION SIGN and
                              *   DEGREE SIGN in a Swiss German layout (Mac:
                              *   only on ISO keyboards), CIRCUMFLEX ACCENT and
                              *   DEGREE SIGN in a German layout (Mac: only on
                              *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
                              *   French Windows layout, COMMERCIAL AT and
                              *   NUMBER SIGN in a French Mac layout on ISO
                              *   keyboards, and LESS-THAN SIGN and GREATER-THAN
                              *   SIGN in a Swiss German, German, or French Mac
                              *   layout on ANSI keyboards.
                              */
    KBDK_COMMA = 54,
    KBDK_PERIOD = 55,
    KBDK_SLASH = 56,

    KBDK_CAPSLOCK = 57,

    KBDK_F1 = 58,
    KBDK_F2 = 59,
    KBDK_F3 = 60,
    KBDK_F4 = 61,
    KBDK_F5 = 62,
    KBDK_F6 = 63,
    KBDK_F7 = 64,
    KBDK_F8 = 65,
    KBDK_F9 = 66,
    KBDK_F10 = 67,
    KBDK_F11 = 68,
    KBDK_F12 = 69,

    KBDK_PRINTSCREEN = 70,
    KBDK_SCROLLLOCK = 71,
    KBDK_PAUSE = 72,
    KBDK_INSERT = 73, /**< insert on PC, help on some Mac keyboards (but
                                   does send code 73, not 117) */
    KBDK_HOME = 74,
    KBDK_PAGEUP = 75,
    KBDK_DELETE = 76,
    KBDK_END = 77,
    KBDK_PAGEDOWN = 78,
    KBDK_RIGHT = 79,
    KBDK_LEFT = 80,
    KBDK_DOWN = 81,
    KBDK_UP = 82,

    KBDK_NUMLOCKCLEAR = 83, /**< num lock on PC, clear on Mac keyboards
                                     */
    KBDK_KP_DIVIDE = 84,
    KBDK_KP_MULTIPLY = 85,
    KBDK_KP_MINUS = 86,
    KBDK_KP_PLUS = 87,
    KBDK_KP_ENTER = 88,
    KBDK_KP_1 = 89,
    KBDK_KP_2 = 90,
    KBDK_KP_3 = 91,
    KBDK_KP_4 = 92,
    KBDK_KP_5 = 93,
    KBDK_KP_6 = 94,
    KBDK_KP_7 = 95,
    KBDK_KP_8 = 96,
    KBDK_KP_9 = 97,
    KBDK_KP_0 = 98,
    KBDK_KP_PERIOD = 99,

    KBDK_NONUSBACKSLASH = 100, /**< This is the additional key that ISO
                                        *   keyboards have over ANSI ones,
                                        *   located between left shift and Y.
                                        *   Produces GRAVE ACCENT and TILDE in a
                                        *   US or UK Mac layout, REVERSE SOLIDUS
                                        *   (backslash) and VERTICAL LINE in a
                                        *   US or UK Windows layout, and
                                        *   LESS-THAN SIGN and GREATER-THAN SIGN
                                        *   in a Swiss German, German, or French
                                        *   layout. */
    KBDK_APPLICATION = 101, /**< windows contextual menu, compose */
    KBDK_POWER = 102, /**< The USB document says this is a status flag,
                               *   not a physical key - but some Mac keyboards
                               *   do have a power key. */
    KBDK_KP_EQUALS = 103,
    KBDK_F13 = 104,
    KBDK_F14 = 105,
    KBDK_F15 = 106,
    KBDK_F16 = 107,
    KBDK_F17 = 108,
    KBDK_F18 = 109,
    KBDK_F19 = 110,
    KBDK_F20 = 111,
    KBDK_F21 = 112,
    KBDK_F22 = 113,
    KBDK_F23 = 114,
    KBDK_F24 = 115,
    KBDK_EXECUTE = 116,
    KBDK_HELP = 117,    /**< AL Integrated Help Center */
    KBDK_MENU = 118,    /**< Menu (show menu) */
    KBDK_SELECT = 119,
    KBDK_STOP = 120,    /**< AC Stop */
    KBDK_AGAIN = 121,   /**< AC Redo/Repeat */
    KBDK_UNDO = 122,    /**< AC Undo */
    KBDK_CUT = 123,     /**< AC Cut */
    KBDK_COPY = 124,    /**< AC Copy */
    KBDK_PASTE = 125,   /**< AC Paste */
    KBDK_FIND = 126,    /**< AC Find */
    KBDK_MUTE = 127,
    KBDK_VOLUMEUP = 128,
    KBDK_VOLUMEDOWN = 129,
/* not sure whether there's a reason to enable these */
/*     KBDK_LOCKINGCAPSLOCK = 130,  */
/*     KBDK_LOCKINGNUMLOCK = 131, */
/*     KBDK_LOCKINGSCROLLLOCK = 132, */
    KBDK_KP_COMMA = 133,
    KBDK_KP_EQUALSAS400 = 134,

    KBDK_INTERNATIONAL1 = 135, /**< used on Asian keyboards, see
                                            footnotes in USB doc */
    KBDK_INTERNATIONAL2 = 136,
    KBDK_INTERNATIONAL3 = 137, /**< Yen */
    KBDK_INTERNATIONAL4 = 138,
    KBDK_INTERNATIONAL5 = 139,
    KBDK_INTERNATIONAL6 = 140,
    KBDK_INTERNATIONAL7 = 141,
    KBDK_INTERNATIONAL8 = 142,
    KBDK_INTERNATIONAL9 = 143,
    KBDK_LANG1 = 144, /**< Hangul/English toggle */
    KBDK_LANG2 = 145, /**< Hanja conversion */
    KBDK_LANG3 = 146, /**< Katakana */
    KBDK_LANG4 = 147, /**< Hiragana */
    KBDK_LANG5 = 148, /**< Zenkaku/Hankaku */
    KBDK_LANG6 = 149, /**< reserved */
    KBDK_LANG7 = 150, /**< reserved */
    KBDK_LANG8 = 151, /**< reserved */
    KBDK_LANG9 = 152, /**< reserved */

    KBDK_ALTERASE = 153,    /**< Erase-Eaze */
    KBDK_SYSREQ = 154,
    KBDK_CANCEL = 155,      /**< AC Cancel */
    KBDK_CLEAR = 156,
    KBDK_PRIOR = 157,
    KBDK_RETURN2 = 158,
    KBDK_SEPARATOR = 159,
    KBDK_OUT = 160,
    KBDK_OPER = 161,
    KBDK_CLEARAGAIN = 162,
    KBDK_CRSEL = 163,
    KBDK_EXSEL = 164,

    KBDK_KP_00 = 176,
    KBDK_KP_000 = 177,
    KBDK_THOUSANDSSEPARATOR = 178,
    KBDK_DECIMALSEPARATOR = 179,
    KBDK_CURRENCYUNIT = 180,
    KBDK_CURRENCYSUBUNIT = 181,
    KBDK_KP_LEFTPAREN = 182,
    KBDK_KP_RIGHTPAREN = 183,
    KBDK_KP_LEFTBRACE = 184,
    KBDK_KP_RIGHTBRACE = 185,
    KBDK_KP_TAB = 186,
    KBDK_KP_BACKSPACE = 187,
    KBDK_KP_A = 188,
    KBDK_KP_B = 189,
    KBDK_KP_C = 190,
    KBDK_KP_D = 191,
    KBDK_KP_E = 192,
    KBDK_KP_F = 193,
    KBDK_KP_XOR = 194,
    KBDK_KP_POWER = 195,
    KBDK_KP_PERCENT = 196,
    KBDK_KP_LESS = 197,
    KBDK_KP_GREATER = 198,
    KBDK_KP_AMPERSAND = 199,
    KBDK_KP_DBLAMPERSAND = 200,
    KBDK_KP_VERTICALBAR = 201,
    KBDK_KP_DBLVERTICALBAR = 202,
    KBDK_KP_COLON = 203,
    KBDK_KP_HASH = 204,
    KBDK_KP_SPACE = 205,
    KBDK_KP_AT = 206,
    KBDK_KP_EXCLAM = 207,
    KBDK_KP_MEMSTORE = 208,
    KBDK_KP_MEMRECALL = 209,
    KBDK_KP_MEMCLEAR = 210,
    KBDK_KP_MEMADD = 211,
    KBDK_KP_MEMSUBTRACT = 212,
    KBDK_KP_MEMMULTIPLY = 213,
    KBDK_KP_MEMDIVIDE = 214,
    KBDK_KP_PLUSMINUS = 215,
    KBDK_KP_CLEAR = 216,
    KBDK_KP_CLEARENTRY = 217,
    KBDK_KP_BINARY = 218,
    KBDK_KP_OCTAL = 219,
    KBDK_KP_DECIMAL = 220,
    KBDK_KP_HEXADECIMAL = 221,

    KBDK_LCTRL = 224,
    KBDK_LSHIFT = 225,
    KBDK_LALT = 226, /**< alt, option */
    KBDK_LGUI = 227, /**< windows, command (apple), meta */
    KBDK_RCTRL = 228,
    KBDK_RSHIFT = 229,
    KBDK_RALT = 230, /**< alt gr, option */
    KBDK_RGUI = 231, /**< windows, command (apple), meta */

    KBDK_MODE = 257,    /**< I'm not sure if this is really not covered
                                 *   by any of the above, but since there's a
                                 *   special SDL_KMOD_MODE for it I'm adding it here
                                 */

    /* @} *//* Usage page 0x07 */

    /**
     *  \name Usage page 0x0C
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     *
     *  There are way more keys in the spec than we can represent in the
     *  current scancode range, so pick the ones that commonly come up in
     *  real world usage.
     */
    /* @{ */

    KBDK_SLEEP = 258,                   /**< Sleep */
    KBDK_WAKE = 259,                    /**< Wake */

    KBDK_CHANNEL_INCREMENT = 260,       /**< Channel Increment */
    KBDK_CHANNEL_DECREMENT = 261,       /**< Channel Decrement */

    KBDK_MEDIA_PLAY = 262,          /**< Play */
    KBDK_MEDIA_PAUSE = 263,         /**< Pause */
    KBDK_MEDIA_RECORD = 264,        /**< Record */
    KBDK_MEDIA_FAST_FORWARD = 265,  /**< Fast Forward */
    KBDK_MEDIA_REWIND = 266,        /**< Rewind */
    KBDK_MEDIA_NEXT_TRACK = 267,    /**< Next Track */
    KBDK_MEDIA_PREVIOUS_TRACK = 268, /**< Previous Track */
    KBDK_MEDIA_STOP = 269,          /**< Stop */
    KBDK_MEDIA_EJECT = 270,         /**< Eject */
    KBDK_MEDIA_PLAY_PAUSE = 271,    /**< Play / Pause */
    KBDK_MEDIA_SELECT = 272,        /* Media Select */

    KBDK_AC_NEW = 273,              /**< AC New */
    KBDK_AC_OPEN = 274,             /**< AC Open */
    KBDK_AC_CLOSE = 275,            /**< AC Close */
    KBDK_AC_EXIT = 276,             /**< AC Exit */
    KBDK_AC_SAVE = 277,             /**< AC Save */
    KBDK_AC_PRINT = 278,            /**< AC Print */
    KBDK_AC_PROPERTIES = 279,       /**< AC Properties */

    KBDK_AC_SEARCH = 280,           /**< AC Search */
    KBDK_AC_HOME = 281,             /**< AC Home */
    KBDK_AC_BACK = 282,             /**< AC Back */
    KBDK_AC_FORWARD = 283,          /**< AC Forward */
    KBDK_AC_STOP = 284,             /**< AC Stop */
    KBDK_AC_REFRESH = 285,          /**< AC Refresh */
    KBDK_AC_BOOKMARKS = 286,        /**< AC Bookmarks */

    /* @} *//* Usage page 0x0C */


    /**
     *  \name Mobile keys
     *
     *  These are values that are often used on mobile phones.
     */
    /* @{ */

    KBDK_SOFTLEFT = 287, /**< Usually situated below the display on phones and
                                      used as a multi-function feature key for selecting
                                      a software defined function shown on the bottom left
                                      of the display. */
    KBDK_SOFTRIGHT = 288, /**< Usually situated below the display on phones and
                                       used as a multi-function feature key for selecting
                                       a software defined function shown on the bottom right
                                       of the display. */
    KBDK_CALL = 289, /**< Used for accepting phone calls. */
    KBDK_ENDCALL = 290, /**< Used for rejecting phone calls. */

    /* @} *//* Mobile keys */

    /* Add any other keys here. */

    KBDK_RESERVED = 400,    /**< 400-500 reserved for dynamic keycodes */

    KBDK_COUNT = 512 /**< not a key, just marks the number of scancodes for array bounds */
};

// This comes directly from SDL_keycode.h
enum StellaMod: uInt16
{
  KBDM_NONE   = 0x0000U, /**< no modifier is applicable. */
  KBDM_LSHIFT = 0x0001U, /**< the left Shift key is down. */
  KBDM_RSHIFT = 0x0002U, /**< the right Shift key is down. */
  KBDM_LEVEL5 = 0x0004U, /**< the Level 5 Shift key is down. */
  KBDM_LCTRL  = 0x0040U, /**< the left Ctrl (Control) key is down. */
  KBDM_RCTRL  = 0x0080U, /**< the right Ctrl (Control) key is down. */
  KBDM_LALT   = 0x0100U, /**< the left Alt key is down. */
  KBDM_RALT   = 0x0200U, /**< the right Alt key is down. */
  KBDM_LGUI   = 0x0400U, /**< the left GUI key (often the Windows key) is down. */
  KBDM_RGUI   = 0x0800U, /**< the right GUI key (often the Windows key) is down. */
  KBDM_NUM    = 0x1000U, /**< the Num Lock key (may be located on an extended keypad) is down. */
  KBDM_CAPS   = 0x2000U, /**< the Caps Lock key is down. */
  KBDM_MODE   = 0x4000U, /**< the !AltGr key is down. */
  KBDM_SCROLL = 0x8000U, /**< the Scroll Lock key is down. */
  KBDM_CTRL   = (KBDM_LCTRL | KBDM_RCTRL),   /**< Any Ctrl key is down. */
  KBDM_SHIFT  = (KBDM_LSHIFT | KBDM_RSHIFT), /**< Any Shift key is down. */
  KBDM_ALT    = (KBDM_LALT | KBDM_RALT),     /**< Any Alt key is down. */
  KBDM_GUI    = (KBDM_LGUI | KBDM_RGUI)      /**< Any GUI key is down. */
};

// Test if specified modifier is pressed
namespace StellaModTest
{
  constexpr bool isAlt(int mod)
  {
#if defined(BSPF_MACOS) || defined(MACOS_KEYS)
    return (mod & KBDM_GUI);
#else
    return (mod & KBDM_ALT);
#endif
  }

  constexpr bool isControl(int mod)
  {
    return (mod & KBDM_CTRL);
  }

  constexpr bool isShift(int mod)
  {
    return (mod & KBDM_SHIFT);
  }
}  // namespace StellaModTest

namespace StellaKeyName
{
  inline string_view forKey(StellaKey key)
  {
  #ifdef SDL_SUPPORT
    return SDL_GetScancodeName(static_cast<SDL_Scancode>(key));
  #else
    return string_view{};
  #endif
  }
}  // namespace StellaKeyName

#endif /* StellaKeys */
