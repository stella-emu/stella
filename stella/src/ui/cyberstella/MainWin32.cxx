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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: MainWin32.cxx,v 1.2 2003-11-14 00:47:35 stephena Exp $
//============================================================================

#define STRICT

#include "pch.hxx"
#include "resource.h"
#include "DirectInput.hxx"

#include "bspf.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "FrameBufferWin32.hxx"
#include "Settings.hxx"
#include "SettingsWin32.hxx"
#include "Sound.hxx"
#include "SoundWin32.hxx"
#include "StellaEvent.hxx"
#include "MainWin32.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MainWin32::MainWin32(const uInt8* image, uInt32 size, const char* filename,
                     Settings& settings, PropertiesSet& properties)
  : theSettings(settings),
    thePropertiesSet(properties),
    theDisplay(NULL),
    theSound(NULL),
    theInput(NULL),
    theMouseX(0),
    thePaddleMode(0),
    myIsInitialized(false)
{
  // Setup the DirectX window
  theDisplay = new FrameBufferWin32();
  if(!theDisplay)
  {
    cleanup();
    return;
  }

  // Create a sound object for playing audio
//  string driver = theSettings.getString("sound");
//  if(driver != "0")
//    theSound = new SoundWin32();
//  else
    theSound = new Sound();
  if(!theSound)
  {
    cleanup();
    return;
  }

//  theSound->setSoundVolume(theSettings.getInt("volume"));

  // Create the 2600 game console
  theConsole = new Console(image, size, filename, theSettings, thePropertiesSet,
                           *theDisplay, *theSound);

  // We can now initialize the sound and directinput classes with
  // the handle to the current window.
  // This must be done after the console is created, since at this
  // point we know that the FrameBuffer has been fully initialized

  // Initialize DirectInput
  theInput = new DirectInput();
  if(!theInput)
  {
    cleanup();
    return;
  }
  theInput->initialize(theDisplay->hwnd());

  myIsInitialized = true;
}

MainWin32::~MainWin32()
{
  cleanup();
}

void MainWin32::cleanup()
{
  ShowCursor(TRUE);

  if(theDisplay)
    delete theDisplay;

  if(theSound)
    delete theSound;
  
  if(theInput)
    delete theInput;

  if(theConsole)
    delete theConsole;

  myIsInitialized = false;
}

DWORD MainWin32::run()
{
  if(!myIsInitialized)
    return 0;

  // Get the initial tick count
  UINT uFrameCount = 0;

  unsigned __int64 uiStartRun;
  QueryPerformanceCounter( (LARGE_INTEGER*)&uiStartRun );

  // Find out how many ticks occur per second
	unsigned __int64 uiCountsPerSecond;
  QueryPerformanceFrequency( (LARGE_INTEGER*)&uiCountsPerSecond );

  const unsigned __int64 uiCountsPerFrame = 
    ( uiCountsPerSecond / 60);// FIXME m_rGlobalData->desiredFrameRate);

  unsigned __int64 uiFrameStart;
  unsigned __int64 uiFrameCurrent;

  // Main message loop
	MSG msg;

  for(;;)
  {
    if(::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ))
    {
      if(msg.message == WM_QUIT)
        break;

      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
    else if (theDisplay->windowActive())
    {
      // idle time -- do stella updates
      ++uFrameCount;

      ::QueryPerformanceCounter( (LARGE_INTEGER*)&uiFrameStart );

      UpdateEvents();
      theDisplay->update();
      theSound->updateSound(*theDisplay->mediaSource());

      // waste time to to meet desired frame rate
      for(;;)
      {
        QueryPerformanceCounter( (LARGE_INTEGER*)&uiFrameCurrent );
        if((uiFrameCurrent - uiFrameStart) >= uiCountsPerFrame)
          break;
//FIXME        else
//          WaitMessage();
      }
    }
  }

  // Main message loop done
/*
  if ( m_rGlobalData->bShowFPS)
	{
		// get number of scanlines in last frame

		uInt32 uScanLines = rMediaSource.scanlines();

		// Get the final tick count

		unsigned __int64 uiEndRun;
        ::QueryPerformanceCounter( (LARGE_INTEGER*)&uiEndRun );

		// Get number of ticks

		DWORD secs = (DWORD)( ( uiEndRun - uiStartRun ) / uiCountsPerSecond );

		DWORD fps = (secs == 0) ? 0 : (uFrameCount / secs);

		TCHAR pszBuf[1024];
		wsprintf( pszBuf, _T("Frames drawn: %ld\nFPS: %ld\nScanlines in last frame: %ld\n"),
			      uFrameCount,
                  fps, 
                  uScanLines );
		MessageBox( NULL, pszBuf, _T("Statistics"), MB_OK );
	}*/

	return msg.wParam;
}

void MainWin32::UpdateEvents()
{
  DIDEVICEOBJECTDATA eventArray[64];
  DWORD numEvents;

  // Check for keyboard events
  numEvents = 64;
  if(theInput->getKeyEvents(eventArray, &numEvents))
  {
    for(unsigned int i = 0; i < numEvents; i++ ) 
    {
      uInt32 key  = eventArray[i].dwOfs;
      uInt8 state = eventArray[i].dwData & 0x80 ? 1 : 0;

      for(uInt32 i = 0; i < sizeof(keyList) / sizeof(Switches); ++i)
      {
        if(keyList[i].nVirtKey == key)
          theConsole->eventHandler().sendKeyEvent(keyList[i].keyCode, state);
      }
    }
  }

  // Check for mouse events
  numEvents = 64;
  if(theInput->getMouseEvents(eventArray, &numEvents))
  {
    for(unsigned int i = 0; i < numEvents; i++ ) 
    {
      Event::Type type = Event::LastType;
      Int32 value;
      switch(eventArray[i].dwOfs)
      {
        // Check for button press and release
        case DIMOFS_BUTTON0:
        case DIMOFS_BUTTON1:
        case DIMOFS_BUTTON2:
        case DIMOFS_BUTTON3:
        case DIMOFS_BUTTON4:
        case DIMOFS_BUTTON5:
        case DIMOFS_BUTTON6:
        case DIMOFS_BUTTON7:
          value = (Int32) eventArray[i].dwData & 0x80 ? 1 : 0;

          if(thePaddleMode == 0)
            type = Event::PaddleZeroFire;
          else if(thePaddleMode == 1)
            type = Event::PaddleOneFire;
          else if(thePaddleMode == 2)
            type = Event::PaddleTwoFire;
          else if(thePaddleMode == 3)
            type = Event::PaddleThreeFire;

          theConsole->eventHandler().sendEvent(type, value);
          break;

        // Check for horizontal movement
        case DIMOFS_X:
          theMouseX = theMouseX + eventArray[i].dwData;

          // Force mouseX between 0 ... 999
          if(theMouseX < 0)
            theMouseX = 0;
          else if(theMouseX > 999)
            theMouseX = 999;

          Int32 value = (999 - theMouseX) * 1000;

          if(thePaddleMode == 0)
            type = Event::PaddleZeroResistance;
          else if(thePaddleMode == 1)
            type = Event::PaddleOneResistance;
          else if(thePaddleMode == 2)
            type = Event::PaddleTwoResistance;
          else if(thePaddleMode == 3)
            type = Event::PaddleThreeResistance;

          theConsole->eventHandler().sendEvent(type, value);
          break;
      }
    }
  }
/*
    //
	// Update joystick
    //

  FIXME - add multiple joysticks 
	if (m_pDirectJoystick->Update() == S_OK)
	{
		rgEventState[Event::JoystickZeroFire] |=
			m_pDirectJoystick->IsButtonPressed(0);

		LONG x;
		LONG y;
		m_pDirectJoystick->GetPos( &x, &y );

		if (x < 0)
        {
			rgEventState[Event::JoystickZeroLeft] = 1;
        }
		else if (x > 0)
        {
			rgEventState[Event::JoystickZeroRight] = 1;
        }
		if (y < 0)
        {
			rgEventState[Event::JoystickZeroUp] = 1;
        }
		else if (y > 0)
        {
			rgEventState[Event::JoystickZeroDown] = 1;
        }
	}

    //
	// Update mouse
    //

	if (m_pDirectMouse->Update() == S_OK)
	{
		// NOTE: Mouse::GetPos returns a value from 0..999

		LONG x;
		m_pDirectMouse->GetPos( &x, NULL );

		// Mouse resistance is measured between 0...1000000

//    	rgEventState[ m_rGlobalData->PaddleResistanceEvent() ] = (999-x)*1000;
		
//		rgEventState[ m_rGlobalData->PaddleFireEvent() ] |= m_pDirectMouse->IsButtonPressed(0);
	}

    //
	// Write new event state
    //

//	for (i = 0; i < nEventCount; ++i)
//	{
//		m_rEvent.set( (Event::Type)i, rgEventState[i] );
//	}
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MainWin32::Switches MainWin32::keyList[StellaEvent::LastKCODE] = { 
  { DIK_F1,           StellaEvent::KCODE_F1           },
  { DIK_F2,           StellaEvent::KCODE_F2           },
  { DIK_F3,           StellaEvent::KCODE_F3           },
  { DIK_F4,           StellaEvent::KCODE_F4           },
  { DIK_F5,           StellaEvent::KCODE_F5           },
  { DIK_F6,           StellaEvent::KCODE_F6           },
  { DIK_F7,           StellaEvent::KCODE_F7           },
  { DIK_F8,           StellaEvent::KCODE_F8           },
  { DIK_F9,           StellaEvent::KCODE_F9           },
  { DIK_F10,          StellaEvent::KCODE_F10          },
  { DIK_F11,          StellaEvent::KCODE_F11          },
  { DIK_F12,          StellaEvent::KCODE_F12          },
  { DIK_F13,          StellaEvent::KCODE_F13          },
  { DIK_F14,          StellaEvent::KCODE_F14          },
  { DIK_F15,          StellaEvent::KCODE_F15          },

  { DIK_UP,           StellaEvent::KCODE_UP           },
  { DIK_DOWN,         StellaEvent::KCODE_DOWN         },
  { DIK_LEFT,         StellaEvent::KCODE_LEFT         },
  { DIK_RIGHT,        StellaEvent::KCODE_RIGHT        },
  { DIK_SPACE,        StellaEvent::KCODE_SPACE        },
  { DIK_LCONTROL,     StellaEvent::KCODE_LCTRL        },
  { DIK_RCONTROL,     StellaEvent::KCODE_RCTRL        },
  { DIK_LMENU,        StellaEvent::KCODE_LALT         },
  { DIK_RMENU,        StellaEvent::KCODE_RALT         },
  { DIK_LWIN,         StellaEvent::KCODE_LWIN         },
  { DIK_RWIN,         StellaEvent::KCODE_RWIN         },
  { DIK_APPS,         StellaEvent::KCODE_MENU         },

  { DIK_A,            StellaEvent::KCODE_a            },
  { DIK_B,            StellaEvent::KCODE_b            },
  { DIK_C,            StellaEvent::KCODE_c            },
  { DIK_D,            StellaEvent::KCODE_d            },
  { DIK_E,            StellaEvent::KCODE_e            },
  { DIK_F,            StellaEvent::KCODE_f            },
  { DIK_G,            StellaEvent::KCODE_g            },
  { DIK_H,            StellaEvent::KCODE_h            },
  { DIK_I,            StellaEvent::KCODE_i            },
  { DIK_J,            StellaEvent::KCODE_j            },
  { DIK_K,            StellaEvent::KCODE_k            },
  { DIK_L,            StellaEvent::KCODE_l            },
  { DIK_M,            StellaEvent::KCODE_m            },
  { DIK_N,            StellaEvent::KCODE_n            },
  { DIK_O,            StellaEvent::KCODE_o            },
  { DIK_P,            StellaEvent::KCODE_p            },
  { DIK_Q,            StellaEvent::KCODE_q            },
  { DIK_R,            StellaEvent::KCODE_r            },
  { DIK_S,            StellaEvent::KCODE_s            },
  { DIK_T,            StellaEvent::KCODE_t            },
  { DIK_U,            StellaEvent::KCODE_u            },
  { DIK_V,            StellaEvent::KCODE_v            },
  { DIK_W,            StellaEvent::KCODE_w            },
  { DIK_X,            StellaEvent::KCODE_x            },
  { DIK_Y,            StellaEvent::KCODE_y            },
  { DIK_Z,            StellaEvent::KCODE_z            },

  { DIK_0,            StellaEvent::KCODE_0            },
  { DIK_1,            StellaEvent::KCODE_1            },
  { DIK_2,            StellaEvent::KCODE_2            },
  { DIK_3,            StellaEvent::KCODE_3            },
  { DIK_4,            StellaEvent::KCODE_4            },
  { DIK_5,            StellaEvent::KCODE_5            },
  { DIK_6,            StellaEvent::KCODE_6            },
  { DIK_7,            StellaEvent::KCODE_7            },
  { DIK_8,            StellaEvent::KCODE_8            },
  { DIK_9,            StellaEvent::KCODE_9            },

  { DIK_NUMPAD0,      StellaEvent::KCODE_KP0          },
  { DIK_NUMPAD1,      StellaEvent::KCODE_KP1          },
  { DIK_NUMPAD2,      StellaEvent::KCODE_KP2          },
  { DIK_NUMPAD3,      StellaEvent::KCODE_KP3          },
  { DIK_NUMPAD4,      StellaEvent::KCODE_KP4          },
  { DIK_NUMPAD5,      StellaEvent::KCODE_KP5          },
  { DIK_NUMPAD6,      StellaEvent::KCODE_KP6          },
  { DIK_NUMPAD7,      StellaEvent::KCODE_KP7          },
  { DIK_NUMPAD8,      StellaEvent::KCODE_KP8          },
  { DIK_NUMPAD9,      StellaEvent::KCODE_KP9          },
  { DIK_DECIMAL,      StellaEvent::KCODE_KP_PERIOD    },
  { DIK_DIVIDE,       StellaEvent::KCODE_KP_DIVIDE    },
  { DIK_MULTIPLY,     StellaEvent::KCODE_KP_MULTIPLY  },
  { DIK_SUBTRACT,     StellaEvent::KCODE_KP_MINUS     },
  { DIK_ADD,          StellaEvent::KCODE_KP_PLUS      },
  { DIK_NUMPADENTER,  StellaEvent::KCODE_KP_ENTER     },
//  { SDLK_KP_EQUALS,   StellaEvent::KCODE_KP_EQUALS  },

  { DIK_BACK,         StellaEvent::KCODE_BACKSPACE    },
  { DIK_TAB,          StellaEvent::KCODE_TAB          },
//    { SDLK_CLEAR,       StellaEvent::KCODE_CLEAR      },
  { DIK_RETURN,       StellaEvent::KCODE_RETURN       },
  { DIK_ESCAPE,       StellaEvent::KCODE_ESCAPE       },
  { DIK_COMMA,        StellaEvent::KCODE_COMMA        },
  { DIK_MINUS,        StellaEvent::KCODE_MINUS        },
  { DIK_PERIOD,       StellaEvent::KCODE_PERIOD       },
  { DIK_SLASH,        StellaEvent::KCODE_SLASH        },
  { DIK_BACKSLASH,    StellaEvent::KCODE_BACKSLASH    },
  { DIK_SEMICOLON,    StellaEvent::KCODE_SEMICOLON    },
  { DIK_EQUALS,       StellaEvent::KCODE_EQUALS       },
  { DIK_APOSTROPHE,   StellaEvent::KCODE_QUOTE        },
  { DIK_GRAVE,        StellaEvent::KCODE_BACKQUOTE    },
  { DIK_LBRACKET,     StellaEvent::KCODE_LEFTBRACKET  },
  { DIK_RBRACKET,     StellaEvent::KCODE_RIGHTBRACKET },

  { DIK_SYSRQ,        StellaEvent::KCODE_PRTSCREEN    },
  { DIK_SCROLL,       StellaEvent::KCODE_SCRLOCK      },
  { DIK_PAUSE,        StellaEvent::KCODE_PAUSE        },
  { DIK_INSERT,       StellaEvent::KCODE_INSERT       },
  { DIK_HOME,         StellaEvent::KCODE_HOME         },
  { DIK_PRIOR,        StellaEvent::KCODE_PAGEUP       },
  { DIK_DELETE,       StellaEvent::KCODE_DELETE       },
  { DIK_END,          StellaEvent::KCODE_END          },
  { DIK_NEXT,         StellaEvent::KCODE_PAGEDOWN     }
};
