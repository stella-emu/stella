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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: StellaEvent.hxx,v 1.6 2003-09-29 18:10:56 stephena Exp $
//============================================================================

#ifndef STELLAEVENT_HXX
#define STELLAEVENT_HXX

/**
  This file defines the global STELLA events that the frontends
  will use to communicate with the Event Handler.

  Only the standard keys are defined here.  Function and
  navigation (HOME, END, etc) keys are special and must be handled
  by the frontends directly.

  @author Stephen Anthony
  @version $Id: StellaEvent.hxx,v 1.6 2003-09-29 18:10:56 stephena Exp $
*/
class StellaEvent
{
  public:
    /**
      Enumeration of keyboard keycodes

      Note that the order of these codes is related to
      UserInterface::ourEventName.  If these are ever changed or rearranged,
      that array must be updated as well.
    */
    enum KeyCode
    {
      KCODE_a, KCODE_b, KCODE_c, KCODE_d, KCODE_e, KCODE_f, KCODE_g, KCODE_h,
      KCODE_i, KCODE_j, KCODE_k, KCODE_l, KCODE_m, KCODE_n, KCODE_o, KCODE_p,
      KCODE_q, KCODE_r, KCODE_s, KCODE_t, KCODE_u, KCODE_v, KCODE_w, KCODE_x,
      KCODE_y, KCODE_z,

      KCODE_0, KCODE_1, KCODE_2, KCODE_3, KCODE_4, KCODE_5, KCODE_6, KCODE_7,
      KCODE_8, KCODE_9,

      KCODE_KP0, KCODE_KP1, KCODE_KP2, KCODE_KP3, KCODE_KP4, KCODE_KP5, KCODE_KP6,
      KCODE_KP7, KCODE_KP8, KCODE_KP9, KCODE_KP_PERIOD, KCODE_KP_DIVIDE,
      KCODE_KP_MULTIPLY, KCODE_KP_MINUS, KCODE_KP_PLUS, KCODE_KP_ENTER,
      KCODE_KP_EQUALS,

      KCODE_BACKSPACE, KCODE_TAB, KCODE_CLEAR, KCODE_RETURN, 
      KCODE_ESCAPE, KCODE_SPACE, KCODE_COMMA, KCODE_MINUS, KCODE_PERIOD,
      KCODE_SLASH, KCODE_BACKSLASH, KCODE_SEMICOLON, KCODE_EQUALS,
      KCODE_QUOTE, KCODE_BACKQUOTE, KCODE_LEFTBRACKET, KCODE_RIGHTBRACKET,

      KCODE_PRTSCREEN, KCODE_SCRLOCK, KCODE_PAUSE,
      KCODE_INSERT,    KCODE_HOME,    KCODE_PAGEUP,
      KCODE_DELETE,    KCODE_END,     KCODE_PAGEDOWN,

      KCODE_LCTRL, KCODE_RCTRL, KCODE_LALT, KCODE_RALT, KCODE_LWIN,
      KCODE_RWIN, KCODE_MENU, KCODE_UP, KCODE_DOWN, KCODE_LEFT, KCODE_RIGHT,

      KCODE_F1, KCODE_F2, KCODE_F3, KCODE_F4, KCODE_F5, KCODE_F6, KCODE_F7,
      KCODE_F8, KCODE_F9, KCODE_F10, KCODE_F11, KCODE_F12, KCODE_F13,
      KCODE_F14, KCODE_F15,

      LastKCODE
    };

    /**
      Enumeration of joystick codes and states
    */
    enum JoyStick
    {
      JSTICK_0, JSTICK_1, JSTICK_2, JSTICK_3,
      LastJSTICK
    };

    enum JoyCode
    {
      JAXIS_UP, JAXIS_DOWN, JAXIS_LEFT, JAXIS_RIGHT,
      JBUTTON_0, JBUTTON_1, JBUTTON_2, JBUTTON_3, JBUTTON_4,
      JBUTTON_5, JBUTTON_6, JBUTTON_7, JBUTTON_8, JBUTTON_9,
      LastJCODE
    };
};

#endif
