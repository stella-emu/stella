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
// $Id: DirectInput.hxx,v 1.5 2003-11-14 00:47:35 stephena Exp $
//============================================================================

#ifndef DIRECT_INPUT_HXX
#define DIRECT_INPUT_HXX

#include "bspf.hxx"
#include "dinput.h"

class DirectInput
{
  public:
    DirectInput();
    ~DirectInput();

    bool getKeyEvents(DIDEVICEOBJECTDATA* keyEvents, DWORD* numKeyEvents);
    bool getMouseEvents(DIDEVICEOBJECTDATA* mouseEvents, DWORD* numMouseEvents);

    bool initialize(HWND hwnd);

    void update();

  private:
    void cleanup();

    static BOOL CALLBACK EnumDevicesProc(const DIDEVICEINSTANCE* lpddi, LPVOID pvRef );

    HWND myHWND;

    LPDIRECTINPUT8       mylpdi;
    LPDIRECTINPUTDEVICE8 myKeyboard;
    LPDIRECTINPUTDEVICE8 myMouse;
    LPDIRECTINPUTDEVICE8 myLeftJoystick;
    LPDIRECTINPUTDEVICE8 myRightJoystick;
};

#endif
