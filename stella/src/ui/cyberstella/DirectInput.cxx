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
// $Id: DirectInput.cxx,v 1.6 2003-11-24 23:56:10 stephena Exp $
//============================================================================

#include "pch.hxx"
#include "resource.h"

#include "DirectInput.hxx"

DirectInput::DirectInput(bool disablejoystick)
  : myHWND(NULL),
    mylpdi(NULL),
    myKeyboard(NULL),
    myMouse(NULL),
    myJoystickCount(0),
    myDisableJoystick(disablejoystick)
{
  for(uInt32 i = 0; i < 8; i++)
    myJoystick[i] = NULL;
}

DirectInput::~DirectInput()
{
	cleanup();
}

bool DirectInput::initialize(HWND hwnd)
{
  // FIXME - this should move to the constructor
  if(FAILED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
                               IID_IDirectInput8, (void**)&mylpdi, NULL)))
    return false;

  // We use buffered mode whenever possible, since it is more
  // efficient than constantly getting a full state snapshot
  // and analyzing it for state changes

  // Initialize the keyboard
  if(FAILED(mylpdi->CreateDevice(GUID_SysKeyboard, &myKeyboard, NULL)))
    return false;
  if(FAILED(myKeyboard->SetDataFormat(&c_dfDIKeyboard)))
    return false;
  if(FAILED(myKeyboard->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE)))
    return false;

  DIPROPDWORD k_dipdw;
  k_dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
  k_dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  k_dipdw.diph.dwObj        = 0;
  k_dipdw.diph.dwHow        = DIPH_DEVICE;
  k_dipdw.dwData            = 64;
  if(FAILED(myKeyboard->SetProperty(DIPROP_BUFFERSIZE, &k_dipdw.diph)))
    return false;

  if(FAILED(myKeyboard->Acquire()))
    return false;

  // Initialize the mouse
  if(FAILED(mylpdi->CreateDevice(GUID_SysMouse, &myMouse, NULL)))
    return false;
  if(FAILED(myMouse->SetDataFormat(&c_dfDIMouse2)))
    return false;
  if(FAILED(myMouse->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE)))
    return false; // DISCL_FOREGROUND | DISCL_EXCLUSIVE

  DIPROPDWORD m_dipdw;
  m_dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
  m_dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  m_dipdw.diph.dwObj        = 0;
  m_dipdw.diph.dwHow        = DIPH_DEVICE;
  m_dipdw.dwData            = 64;
  if(FAILED(myMouse->SetProperty(DIPROP_BUFFERSIZE, &m_dipdw.diph)))
    return false;

  if(FAILED(myMouse->Acquire()))
    return false;

  // Don't go any further if using joysticks has been disabled
  if(myDisableJoystick)
    return true;

  // Initialize all joysticks
  // Since a joystick isn't absolutely required, we won't return
  // false if there are none found
  if(FAILED(mylpdi->EnumDevices(DI8DEVCLASS_GAMECTRL,
                                EnumJoysticksCallback,
                                this, DIEDFL_ATTACHEDONLY)))
    return true;

  for(uInt32 i = 0; i < myJoystickCount; i++)
  {
    LPDIRECTINPUTDEVICE8 joystick = myJoystick[i];

    if(FAILED(joystick->SetDataFormat(&c_dfDIJoystick2 )))
      return true;
    if(FAILED(joystick->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)))
      return true;

    // Set the size of the buffer for buffered data
    DIPROPDWORD j_dipdw;
    j_dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
    j_dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    j_dipdw.diph.dwObj        = 0;
    j_dipdw.diph.dwHow        = DIPH_DEVICE;
    j_dipdw.dwData            = 64;
    joystick->SetProperty(DIPROP_BUFFERSIZE, &j_dipdw.diph);

    // Set X-axis range to (-1000 ... +1000)
    DIPROPRANGE dipr;
    dipr.diph.dwSize       = sizeof(dipr);
    dipr.diph.dwHeaderSize = sizeof(dipr.diph);
    dipr.diph.dwHow        = DIPH_BYOFFSET;
    dipr.lMin              = -1000;
    dipr.lMax              = +1000;
    dipr.diph.dwObj = DIJOFS_X;
    joystick->SetProperty(DIPROP_RANGE, &dipr.diph);

    // And again for Y-axis range
    dipr.diph.dwSize       = sizeof(dipr);
    dipr.diph.dwHeaderSize = sizeof(dipr.diph);
    dipr.diph.dwHow        = DIPH_BYOFFSET;
    dipr.lMin              = -1000;
    dipr.lMax              = +1000;
    dipr.diph.dwObj = DIJOFS_Y;
    joystick->SetProperty(DIPROP_RANGE, &dipr.diph);

    // Set dead zone to 50%
    DIPROPDWORD dipdw;
    dipdw.diph.dwSize       = sizeof(dipdw);
    dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
    dipdw.diph.dwHow        = DIPH_BYOFFSET;
    dipdw.dwData            = 5000;
    dipdw.diph.dwObj        = DIJOFS_X;
    joystick->SetProperty(DIPROP_DEADZONE, &dipdw.diph);

    dipdw.diph.dwSize       = sizeof(dipdw);
    dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
    dipdw.diph.dwHow        = DIPH_BYOFFSET;
    dipdw.dwData            = 5000;
    dipdw.diph.dwObj        = DIJOFS_Y;
    joystick->SetProperty(DIPROP_DEADZONE, &dipdw.diph);

    joystick->Acquire();
  }

  return true;
}

bool DirectInput::getKeyEvents(DIDEVICEOBJECTDATA* keyEvents,
                               DWORD* numKeyEvents)
{
  HRESULT hr;

  // Make sure the keyboard has been initialized
  if(myKeyboard == NULL)
    return false;

  // Check for keyboard events
  hr = myKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), 
                                 keyEvents, numKeyEvents, 0 );

  if(hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
  {
    hr = myKeyboard->Acquire();
    if(hr == DIERR_OTHERAPPHASPRIO)
      return false;

    hr = myKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), 
                                   keyEvents, numKeyEvents, 0 );
  }

  return true;
}

bool DirectInput::getMouseEvents(DIDEVICEOBJECTDATA* mouseEvents,
                                 DWORD* numMouseEvents)
{
  HRESULT hr;

  // Make sure the mouse has been initialized
  if(myMouse == NULL)
    return false;

  // Check for mouse events
  hr = myMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), 
                              mouseEvents, numMouseEvents, 0 );

  if(hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
  {
    hr = myMouse->Acquire();
    if(hr == DIERR_OTHERAPPHASPRIO)
      return false;

    hr = myMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), 
                                mouseEvents, numMouseEvents, 0 );
  }

  return true;
}

bool DirectInput::getJoystickEvents(uInt32 stick, DIDEVICEOBJECTDATA* joyEvents,
                                    DWORD* numJoyEvents)
{
  LPDIRECTINPUTDEVICE8 joystick;

  // Make sure the joystick exists and has been initialized
  if(stick >= 0 && stick <= 8)
  {
    joystick = myJoystick[stick];
    if(joystick == NULL)
      return false;
  }
  else
    return false;

  // Check for joystick events
  joystick->Poll();
  HRESULT hr = joystick->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), 
                                       joyEvents, numJoyEvents, 0 );

  if(hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
  {
    hr = joystick->Acquire();
    if(hr == DIERR_OTHERAPPHASPRIO)
      return false;

    joystick->Poll();
    hr = joystick->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), 
                                 joyEvents, numJoyEvents, 0 );
  }

  return true;
}

void DirectInput::cleanup()
{
  for(uInt32 i = 0; i < myJoystickCount; i++)
  {
    if(myJoystick[i])
    {
      myJoystick[i]->Unacquire();
      myJoystick[i]->Release();
      myJoystick[i] = NULL;
    }
  }

  if(myMouse)
  {
    myMouse->Unacquire();
    myMouse->Release();
    myMouse = NULL;
  }

  if(myKeyboard)
  {
    myKeyboard->Unacquire();
    myKeyboard->Release();
    myKeyboard = NULL;
  }

  if(mylpdi)
  {
    mylpdi->Release();
    mylpdi = NULL;
  }
}

BOOL CALLBACK DirectInput::EnumJoysticksCallback(
                const DIDEVICEINSTANCE* inst, 
               	LPVOID pvRef)
{
  DirectInput* pThis = (DirectInput*) pvRef;
  if(!pThis)
    return DIENUM_STOP;

  // If we can't store any more joysticks, then stop enumeration.
  // The limit is set to 8, since the Stella eventhandler core
  // can use up to 8 joysticks.
  if(pThis->myJoystickCount > 8)
    return DIENUM_STOP;

  // Obtain an interface to the enumerated joystick.
  HRESULT hr = pThis->mylpdi->CreateDevice(inst->guidInstance,
      (LPDIRECTINPUTDEVICE8*) &pThis->myJoystick[pThis->myJoystickCount], NULL );

  // Indicate that we've found one more joystick
  if(!FAILED(hr))
    pThis->myJoystickCount++;

  // And continue enumeration for more joysticks
  return DIENUM_CONTINUE;
}
