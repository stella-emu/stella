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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: DirectInput.hxx,v 1.7 2003-11-24 23:56:10 stephena Exp $
//============================================================================

#ifndef DIRECT_INPUT_HXX
#define DIRECT_INPUT_HXX

#define DIRECTINPUT_VERSION 0x0800

#include "bspf.hxx"
#include "dinput.h"

class DirectInput
{
  public:
    DirectInput(bool usejoystick);
    ~DirectInput();

    bool getKeyEvents(DIDEVICEOBJECTDATA* keyEvents, DWORD* numKeyEvents);
    bool getMouseEvents(DIDEVICEOBJECTDATA* mouseEvents, DWORD* numMouseEvents);
    bool getJoystickEvents(uInt32 stick, DIDEVICEOBJECTDATA* joyEvents,
                           DWORD* numJoyEvents);

    bool initialize(HWND hwnd);

    void update();

    uInt32 numJoysticks() { return myJoystickCount; }

  private:
    void cleanup();

    static BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* inst,
                                               LPVOID pvRef);

    HWND myHWND;

    LPDIRECTINPUT8       mylpdi;
    LPDIRECTINPUTDEVICE8 myKeyboard;
    LPDIRECTINPUTDEVICE8 myMouse;
    LPDIRECTINPUTDEVICE8 myJoystick[8];
    uInt32 myJoystickCount;
    bool myDisableJoystick;
};

#endif
