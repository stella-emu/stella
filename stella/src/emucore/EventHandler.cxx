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
// Copyright (c) 1995-2004 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventHandler.cxx,v 1.70 2005-06-05 23:46:19 stephena Exp $
//============================================================================

#include <algorithm>
#include <sstream>
#include <SDL.h>

#include "Event.hxx"
#include "EventHandler.hxx"
#include "FSNode.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "FrameBuffer.hxx"
#include "Sound.hxx"
#include "OSystem.hxx"
#include "Menu.hxx"
#include "Launcher.hxx"
#include "Debugger.hxx"
#include "GuiUtils.hxx"
#include "bspf.hxx"

#ifdef SNAPSHOT_SUPPORT
  #include "Snapshot.hxx"
#endif

#ifdef MAC_OSX
extern "C" {
void handleMacOSXKeypress(int key);
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(OSystem* osystem)
    : myOSystem(osystem),
      myState(S_NONE),
      myLSState(0),
      myPauseFlag(false),
      myQuitFlag(false),
      myGrabMouseFlag(false),
      myUseLauncherFlag(false),
      myPaddleMode(0)
{
  // Add this eventhandler object to the OSystem
  myOSystem->attach(this);

  // Create the event object which will be used for this handler
  myEvent = new Event();

  // Erase the KeyEvent arrays
  for(Int32 i = 0; i < SDLK_LAST; ++i)
  {
    myKeyTable[i] = Event::NoType;
    ourSDLMapping[i] = "";
  }

  // Erase the JoyEvent array
  for(Int32 i = 0; i < kNumJoysticks * kNumJoyButtons; ++i)
    myJoyTable[i] = Event::NoType;

  // Erase the Message array 
  for(Int32 i = 0; i < Event::LastType; ++i)
    ourMessageTable[i] = "";

  // Set unchanging messages
  ourMessageTable[Event::ConsoleColor]            = "Color Mode";
  ourMessageTable[Event::ConsoleBlackWhite]       = "BW Mode";
  ourMessageTable[Event::ConsoleLeftDifficultyA]  = "Left Difficulty A";
  ourMessageTable[Event::ConsoleLeftDifficultyB]  = "Left Difficulty B";
  ourMessageTable[Event::ConsoleRightDifficultyA] = "Right Difficulty A";
  ourMessageTable[Event::ConsoleRightDifficultyB] = "Right Difficulty B";

  // Make sure the event/action mappings are correctly set,
  // and fill the ActionList structure with valid values
  setSDLMappings();
  setKeymap();
  setJoymap();
  setActionMappings();

  myGrabMouseFlag = myOSystem->settings().getBool("grabmouse");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::~EventHandler()
{
  if(myEvent)
    delete myEvent;

#ifdef JOYSTICK_SUPPORT
  if(SDL_WasInit(SDL_INIT_JOYSTICK) & SDL_INIT_JOYSTICK)
  {
    for(uInt32 i = 0; i < kNumJoysticks; i++)
    {
      if(SDL_JoystickOpened(i))
        SDL_JoystickClose(ourJoysticks[i].stick);
    }
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event* EventHandler::event()
{
  return myEvent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::reset(State state)
{
  myState = state;
  myLSState = 0;
  myPauseFlag = false;
  myQuitFlag = false;
  myPaddleMode = 0;

  myOSystem->frameBuffer().pause(myPauseFlag);
  myOSystem->sound().mute(myPauseFlag);
  myEvent->clear();

  switch(myState)
  {
    case S_EMULATE:
      break;

    case S_MENU:
      break;

    case S_LAUNCHER:
      myUseLauncherFlag = true;
      break;

    case S_DEBUGGER:
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setupJoysticks()
{
#ifdef JOYSTICK_SUPPORT
  bool showinfo = myOSystem->settings().getBool("showinfo");

  // Keep track of how many Stelladaptors we've found
  uInt8 saCount = 0;

  // First clear the joystick array
  for(uInt32 i = 0; i < kNumJoysticks; i++)
  {
    ourJoysticks[i].stick = (SDL_Joystick*) NULL;
    ourJoysticks[i].type  = JT_NONE;
  }

  // Initialize the joystick subsystem
  if((SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) || (SDL_NumJoysticks() <= 0))
  {
    if(showinfo)
      cout << "No joysticks present, use the keyboard." << endl;
    return;
  }

  // Open up to 6 regular joysticks and 2 Stelladaptor devices
  uInt32 limit = SDL_NumJoysticks() <= kNumJoysticks ?
                 SDL_NumJoysticks() : kNumJoysticks;
  for(uInt32 i = 0; i < limit; i++)
  {
    string name = SDL_JoystickName(i);
    ourJoysticks[i].stick = SDL_JoystickOpen(i);

    // Skip if we couldn't open it for any reason
    if(ourJoysticks[i].stick == NULL)
    {
      ourJoysticks[i].type = JT_NONE;
      continue;
    }

    // Figure out what type of joystick this is
    if(name.find("Stelladaptor", 0) != string::npos)
    {
      saCount++;
      if(saCount > 2)  // Ignore more than 2 Stelladaptors
      {
        ourJoysticks[i].type = JT_NONE;
        continue;
      }
      else if(saCount == 1)
      {
        name = "Left Stelladaptor (Left joystick, Paddles 0 and 1, Left driving controller)";
        ourJoysticks[i].type = JT_STELLADAPTOR_1;
      }
      else if(saCount == 2)
      {
        name = "Right Stelladaptor (Right joystick, Paddles 2 and 3, Right driving controller)";
        ourJoysticks[i].type = JT_STELLADAPTOR_2;
      }

      if(showinfo)
        cout << "Joystick " << i << ": " << name << endl;
    }
    else
    {
      ourJoysticks[i].type = JT_REGULAR;

      if(showinfo)
        cout << "Joystick " << i << ": " << SDL_JoystickName(i)
             << " with " << SDL_JoystickNumButtons(ourJoysticks[i].stick)
             << " buttons." << endl;
    }
    if(showinfo)
      cout << endl;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::poll(uInt32 time)
{
  SDL_Event event;

  // Check for an event
  while(SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      // keyboard events
      case SDL_KEYUP:
      case SDL_KEYDOWN:
      {
        SDLKey key  = event.key.keysym.sym;
        SDLMod mod  = event.key.keysym.mod;
        uInt8 state = event.key.type == SDL_KEYDOWN ? 1 : 0;

        // An attempt to speed up event processing
        // All SDL-specific event actions are accessed by either
        // Control/Cmd or Alt/Shift-Cmd keys.  So we quickly check for those.
#ifndef MAC_OSX
        if(mod & KMOD_ALT && state)
#else
        if((mod & KMOD_META) && (mod & KMOD_SHIFT) && state)
#endif
        {
          // These keys work in all states
          switch(int(key))
          {
            case SDLK_EQUALS:
              myOSystem->frameBuffer().resize(NextSize);
              break;

            case SDLK_MINUS:
              myOSystem->frameBuffer().resize(PreviousSize);
              break;

    #ifndef MAC_OSX
            case SDLK_RETURN:
              myOSystem->frameBuffer().toggleFullscreen();
              break;
    #endif
            case SDLK_f:
              myOSystem->frameBuffer().toggleFilter();
              break;

            case SDLK_g:
              myOSystem->toggleFrameBuffer();
              break;
          }

          // These only work when in emulation mode
          if(myState == S_EMULATE)
          {
            switch(int(key))
            {
              case SDLK_LEFTBRACKET:
                myOSystem->sound().adjustVolume(-1);
                break;

              case SDLK_RIGHTBRACKET:
                myOSystem->sound().adjustVolume(1);
                break;

    #ifdef DEVELOPER_SUPPORT
              case SDLK_END:       // Alt-End increases XStart
                myOSystem->console().changeXStart(1);
                break;

              case SDLK_HOME:      // Alt-Home decreases XStart
                myOSystem->console().changeXStart(0);
                break;

              case SDLK_PAGEUP:    // Alt-PageUp increases YStart
                myOSystem->console().changeYStart(1);
                break;

              case SDLK_PAGEDOWN:  // Alt-PageDown decreases YStart
                myOSystem->console().changeYStart(0);
                break;
      #endif
      #ifdef DEVELOPER_SUPPORT
              case SDLK_z:
                myOSystem->console().toggleP0Bit();
                break;

              case SDLK_x:
                myOSystem->console().toggleP1Bit();
                break;

              case SDLK_c:
                myOSystem->console().toggleM0Bit();
                break;

              case SDLK_v:
                myOSystem->console().toggleM1Bit();
                break;

              case SDLK_b:
                myOSystem->console().toggleBLBit();
                break;

              case SDLK_n:
                myOSystem->console().togglePFBit();
                break;

              case SDLK_PERIOD:
                myOSystem->console().enableBits(false);
                break;

              case SDLK_SLASH:
                myOSystem->console().enableBits(true);
                break;
      #endif
              case SDLK_s:	 // Alt-s merges properties into stella.pro
                myOSystem->console().saveProperties(myOSystem->propertiesOutputFilename(), true);
                break;
            }
          }
        }
#ifndef MAC_OSX
        else if(mod & KMOD_CTRL && state)
#else
        else if((mod & KMOD_META) && !(mod & KMOD_SHIFT) && state)
#endif
        {
          // These keys work in all states
          switch(int(key))
          {
            case SDLK_q:
              handleEvent(Event::Quit, 1);
              break;

  #ifdef MAC_OSX
            case SDLK_h:
            case SDLK_m:
            case SDLK_SLASH:
              handleMacOSXKeypress(int(key));
              break;

            case SDLK_RETURN:
              myOSystem->frameBuffer().toggleFullscreen();
              break;
  #endif
            case SDLK_g:
              // don't change grabmouse in fullscreen mode
              if(!myOSystem->frameBuffer().fullScreen())
              {
                myGrabMouseFlag = !myGrabMouseFlag;
                myOSystem->settings().setBool("grabmouse", myGrabMouseFlag);
                myOSystem->frameBuffer().grabMouse(myGrabMouseFlag);
              }
              break;
          }

          // These only work when in emulation mode
          if(myState == S_EMULATE)
          {
            switch(int(key))
            {
              case SDLK_0:  // Ctrl-0 sets the mouse to paddle 0
                setPaddleMode(0, true);
                break;

              case SDLK_1:  // Ctrl-1 sets the mouse to paddle 1
                setPaddleMode(1, true);
                break;

              case SDLK_2:  // Ctrl-2 sets the mouse to paddle 2
                setPaddleMode(2, true);
                break;

              case SDLK_3:  // Ctrl-3 sets the mouse to paddle 3
                setPaddleMode(3, true);
                  break;

              case SDLK_f:  // Ctrl-f toggles NTSC/PAL mode
                myOSystem->console().toggleFormat();
                break;

              case SDLK_p:  // Ctrl-p toggles different palettes
                myOSystem->console().togglePalette();
                break;

              case SDLK_r:  // Ctrl-r reloads the currently loaded ROM
                myOSystem->createConsole();
                break;

  #ifdef DEVELOPER_SUPPORT
              case SDLK_END:       // Ctrl-End increases Width
                myOSystem->console().changeWidth(1);
                break;

              case SDLK_HOME:      // Ctrl-Home decreases Width
                myOSystem->console().changeWidth(0);
                break;

              case SDLK_PAGEUP:    // Ctrl-PageUp increases Height
                myOSystem->console().changeHeight(1);
                break;

              case SDLK_PAGEDOWN:  // Ctrl-PageDown decreases Height
                myOSystem->console().changeHeight(0);
                break;
  #endif
              case SDLK_s:         // Ctrl-s saves properties to a file
                string newPropertiesFile = myOSystem->baseDir() + BSPF_PATH_SEPARATOR +
                  myOSystem->console().properties().get("Cartridge.Name") + ".pro";
                myOSystem->console().saveProperties(newPropertiesFile);
                break;
            }
          }
        }
        else
          // Otherwise, let the event handler deal with it
          handleKeyEvent(key, mod, state);

        break;  // SDL_KEYUP, SDL_KEYDOWN
      }

      case SDL_MOUSEMOTION:
        handleMouseMotionEvent(event);
        break; // SDL_MOUSEMOTION

      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEBUTTONDOWN:
      {
        uInt8 state = event.button.type == SDL_MOUSEBUTTONDOWN ? 1 : 0;
        handleMouseButtonEvent(event, state);
        break;  // SDL_MOUSEBUTTONUP, SDL_MOUSEBUTTONDOWN
      }

      case SDL_ACTIVEEVENT:
        if((event.active.state & SDL_APPACTIVE) && (event.active.gain == 0))
          if(!myPauseFlag)
            handleEvent(Event::Pause, 1);
        break; // SDL_ACTIVEEVENT

      case SDL_QUIT:
        handleEvent(Event::Quit, 1);
        break;  // SDL_QUIT

      case SDL_VIDEOEXPOSE:
        myOSystem->frameBuffer().refresh();
        break;  // SDL_VIDEOEXPOSE
    }

#ifdef JOYSTICK_SUPPORT
    // Read joystick events and modify event states
    uInt8  stick;
    uInt32 code;
    uInt8  state;
    Uint8 axis;
    Uint8 button;
    Int32 resistance;
    Sint16 value;
    JoyType type;

    if(event.jbutton.which >= kNumJoysticks)
      return;

    stick = event.jbutton.which;
    type  = ourJoysticks[stick].type;

    // Figure put what type of joystick we're dealing with
    // Stelladaptors behave differently, and can't be remapped
    switch(type)
    {
      case JT_NONE:
        break;

      case JT_REGULAR:
        switch(event.type)
        {
          case SDL_JOYBUTTONUP:
          case SDL_JOYBUTTONDOWN:
            if(event.jbutton.button >= kNumJoyButtons-4)
              return;

            code  = event.jbutton.button;
            state = event.jbutton.state == SDL_PRESSED ? 1 : 0;

            handleJoyEvent(stick, code, state);
            break;

          case SDL_JOYAXISMOTION:
            axis = event.jaxis.axis;
            value = event.jaxis.value;

            if(axis == 0)  // x-axis
            {
              handleJoyEvent(stick, kJAxisLeft, (value < -16384) ? 1 : 0);
              handleJoyEvent(stick, kJAxisRight, (value > 16384) ? 1 : 0);
            }
            else if(axis == 1)  // y-axis
            {
              handleJoyEvent(stick, kJAxisUp, (value < -16384) ? 1 : 0);
              handleJoyEvent(stick, kJAxisDown, (value > 16384) ? 1 : 0);
            }
            break;
        }
        break;  // Regular joystick

      case JT_STELLADAPTOR_1:
      case JT_STELLADAPTOR_2:
        switch(event.type)
        {
          case SDL_JOYBUTTONUP:
          case SDL_JOYBUTTONDOWN:
            button = event.jbutton.button;
            state  = event.jbutton.state == SDL_PRESSED ? 1 : 0;

            // Send button events for the joysticks/paddles/driving controllers
            if(button == 0)
            {
              if(type == JT_STELLADAPTOR_1)
              {
                handleEvent(Event::JoystickZeroFire, state);
                handleEvent(Event::DrivingZeroFire, state);
                handleEvent(Event::PaddleZeroFire, state);
              }
              else
              {
                handleEvent(Event::JoystickOneFire, state);
                handleEvent(Event::DrivingOneFire, state);
                handleEvent(Event::PaddleTwoFire, state);
              }
            }
            else if(button == 1)
            {
              if(type == JT_STELLADAPTOR_1)
                handleEvent(Event::PaddleOneFire, state);
              else
                handleEvent(Event::PaddleThreeFire, state);
            }
            break;

          case SDL_JOYAXISMOTION:
            axis = event.jaxis.axis;
            value = event.jaxis.value;

            // Send axis events for the joysticks
            handleEvent(SA_Axis[type-2][axis][0], (value < -16384) ? 1 : 0);
            handleEvent(SA_Axis[type-2][axis][1], (value > 16384) ? 1 : 0);

            // Send axis events for the paddles
            resistance = (Int32) (1000000.0 * (32767 - value) / 65534);
            handleEvent(SA_Axis[type-2][axis][2], resistance);

            // Send events for the driving controllers
            if(axis == 1)
            {
              if(value <= -16384-4096)
                handleEvent(SA_DrivingValue[type-2],2);
              else if(value > 16384+4096)
                handleEvent(SA_DrivingValue[type-2],1);
              else if(value >= 16384-4096)
                handleEvent(SA_DrivingValue[type-2],0);
              else
                handleEvent(SA_DrivingValue[type-2],3);
            }
            break;
        }
        break;  // Stelladaptor joystick

      default:
        break;
    }
#endif
  }

  // Update the current dialog container at regular intervals
  // Used to implement continuous events
  switch(myState)
  {
    case S_MENU:
      myOSystem->menu().updateTime(time);
      break;

    case S_LAUNCHER:
      myOSystem->launcher().updateTime(time);
      break;

    case S_DEBUGGER:
      myOSystem->debugger().updateTime(time);
      break;

    default:
      break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleKeyEvent(SDLKey key, SDLMod mod, uInt8 state)
{
  // Determine which mode we're in, then send the event to the appropriate place
  switch(myState)
  {
    case S_EMULATE:
      if(myKeyTable[key] == Event::MenuMode && state == 1 && !myPauseFlag)
      {
        enterMenuMode();
        return;
      }
      else if(myKeyTable[key] == Event::DebuggerMode && state == 1 && !myPauseFlag)
      {
        enterDebugMode();
        return;
      }
      else
        handleEvent(myKeyTable[key], state);

      break;  // S_EMULATE

    case S_MENU:
      if(myKeyTable[key] == Event::MenuMode && state == 1)
      {
        leaveMenuMode();
        return;
      }
      myOSystem->menu().handleKeyEvent(key, mod, state);
      break;

    case S_LAUNCHER:
      myOSystem->launcher().handleKeyEvent(key, mod, state);
      break;

    case S_DEBUGGER:
      if(myKeyTable[key] == Event::DebuggerMode && state == 1)
      {
        leaveDebugMode();
        return;
      }
      myOSystem->debugger().handleKeyEvent(key, mod, state);
      break;

    case S_NONE:
      return;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseMotionEvent(SDL_Event& event)
{
  // Take window zooming into account
  int x = event.motion.x, y = event.motion.y;
  myOSystem->frameBuffer().translateCoords(&x, &y);

  // Determine which mode we're in, then send the event to the appropriate place
  switch(myState)
  {
    case S_EMULATE:
    {
      int w = myOSystem->frameBuffer().baseWidth();

      // Grabmouse introduces some lag into the mouse movement,
      // so we need to fudge the numbers a bit
      // FIXME - possibly do x *= 1.5 ??

      int resistance = (int)(1000000.0 * (w - x) / w);
      handleEvent(Paddle_Resistance[myPaddleMode], resistance);
      break;
    }

    case S_MENU:
      myOSystem->menu().handleMouseMotionEvent(x, y, 0);
      break;

    case S_LAUNCHER:
      myOSystem->launcher().handleMouseMotionEvent(x, y, 0);
      break;

    case S_DEBUGGER:
      myOSystem->debugger().handleMouseMotionEvent(x, y, 0);
      break;

    case S_NONE:
      return;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseButtonEvent(SDL_Event& event, uInt8 state)
{
  // Determine which mode we're in, then send the event to the appropriate place
  switch(myState)
  {
    case S_EMULATE:
      handleEvent(Paddle_Button[myPaddleMode], state);
      break;

    case S_MENU:
    case S_LAUNCHER:
    case S_DEBUGGER:
    {
      // Take window zooming into account
      Int32 x = event.button.x, y = event.button.y;
//if (state) cerr << "B: x = " << x << ", y = " << y << endl;
      myOSystem->frameBuffer().translateCoords(&x, &y);
//if (state) cerr << "A: x = " << x << ", y = " << y << endl << endl;
      MouseButton button;

      if(state == 1)
      {
        if(event.button.button == SDL_BUTTON_LEFT)
          button = EVENT_LBUTTONDOWN;
        else if(event.button.button == SDL_BUTTON_RIGHT)
          button = EVENT_RBUTTONDOWN;
        else if(event.button.button == SDL_BUTTON_WHEELUP)
          button = EVENT_WHEELUP;
        else if(event.button.button == SDL_BUTTON_WHEELDOWN)
          button = EVENT_WHEELDOWN;
        else
          break;
      }
      else
      {
        if(event.button.button == SDL_BUTTON_LEFT)
          button = EVENT_LBUTTONUP;
        else if(event.button.button == SDL_BUTTON_RIGHT)
          button = EVENT_RBUTTONUP;
        else
          break;
      }

      if(myState == S_MENU)
        myOSystem->menu().handleMouseButtonEvent(button, x, y, state);
      else if(myState == S_LAUNCHER)
        myOSystem->launcher().handleMouseButtonEvent(button, x, y, state);
      else
        myOSystem->debugger().handleMouseButtonEvent(button, x, y, state);
      break;
    }

    default:
      break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleJoyEvent(uInt8 stick, uInt32 code, uInt8 state)
{
  // Determine which mode we're in, then send the event to the appropriate place
  switch(myState)
  {
    case S_EMULATE:
      handleEvent(myJoyTable[stick*kNumJoyButtons + code], state);
      break;

    case S_MENU:
      myOSystem->menu().handleJoyEvent(stick, code, state);
      break;

    case S_LAUNCHER:
      myOSystem->launcher().handleJoyEvent(stick, code, state);
      break;

    case S_DEBUGGER:
      myOSystem->debugger().handleJoyEvent(stick, code, state);
      break;

    case S_NONE:
      return;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleEvent(Event::Type event, Int32 state)
{
  // Ignore unmapped events
  if(event == Event::NoType)
    return;

  // Take care of special events that aren't part of the emulation core
  if(state == 1)
  {
    if(event == Event::SaveState)
    {
      saveState();
      return;
    }
    else if(event == Event::ChangeState)
    {
      changeState();
      return;
    }
    else if(event == Event::LoadState)
    {
      loadState();
      return;
    }
    else if(event == Event::TakeSnapshot)
    {
      takeSnapshot();
      return;
    }
    else if(event == Event::Pause)
    {
      myPauseFlag = !myPauseFlag;
      myOSystem->frameBuffer().pause(myPauseFlag);
      myOSystem->sound().mute(myPauseFlag);
      return;
    }
    else if(event == Event::LauncherMode)
    {
      // ExitGame will only work when we've launched stella using the ROM
      // launcher.  Otherwise, the only way to exit the main loop is to Quit.
      if(myState == S_EMULATE && myUseLauncherFlag)
      {
        myOSystem->settings().saveConfig();
        myOSystem->createLauncher();
        return;
      }
    }
    else if(event == Event::Quit)
    {
      myQuitFlag = true;
      myOSystem->settings().saveConfig();
      return;
    }

    if(ourMessageTable[event] != "")
      myOSystem->frameBuffer().showMessage(ourMessageTable[event]);
  }

  // Otherwise, pass it to the emulation core
  myEvent->set(event, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setActionMappings()
{
  // Fill the ActionList with the current key and joystick mappings
  for(Int32 i = 0; i < 61; ++i)
  {
    Event::Type event = ourActionList[i].event;
    ourActionList[i].key = "None";
    string key = "";
    for(uInt32 j = 0; j < SDLK_LAST; ++j)   // size of myKeyTable
    {
      if(myKeyTable[j] == event)
      {
        if(key == "")
          key = key + ourSDLMapping[j];
        else
          key = key + ", " + ourSDLMapping[j];
      }
    }
    for(uInt32 j = 0; j < kNumJoysticks * kNumJoyButtons; ++j)
    {
      if(myJoyTable[j] == event)
      {
        ostringstream joyevent;
        uInt32 stick  = j / kNumJoyButtons;
        uInt32 button = j % kNumJoyButtons;

        switch(button)
        {
          case kJAxisUp:
            joyevent << "J" << stick << " UP";
            break;

          case kJAxisDown:
            joyevent << "J" << stick << " DOWN";
            break;

          case kJAxisLeft:
            joyevent << "J" << stick << " LEFT";
            break;

          case kJAxisRight:
            joyevent << "J" << stick << " RIGHT";
            break;

          default:
            joyevent << "J" << stick << " B" << button;
            break;
        }
        if(key == "")
          key = key + joyevent.str();
        else
          key = key + ", " + joyevent.str();
      }
    }

    // There are some keys which are hardcoded.  These should be represented too.
    string prepend = "";
    if(event == Event::Quit)
#ifndef MAC_OSX
      prepend = "Ctrl Q";
#else
      prepend = "Cmd Q";
#endif
    // else if ...

    if(key == "")
      key = prepend;
    else if(prepend != "")
      key = prepend + ", " + key;

    if(key != "")
      ourActionList[i].key = key;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setKeymap()
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = myOSystem->settings().getString("keymap");
  replace(list.begin(), list.end(), ':', ' ');

  if(isValidList(list, SDLK_LAST))
  {
    istringstream buf(list);
    string key;

    // Fill the keymap table with events
    for(Int32 i = 0; i < SDLK_LAST; ++i)
    {
      buf >> key;
      myKeyTable[i] = (Event::Type) atoi(key.c_str());
    }
  }
  else
    setDefaultKeymap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setJoymap()
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = myOSystem->settings().getString("joymap");
  replace(list.begin(), list.end(), ':', ' ');

  if(isValidList(list, kNumJoysticks*kNumJoyButtons))
  {
    istringstream buf(list);
    string key;

    // Fill the joymap table with events
    for(Int32 i = 0; i < kNumJoysticks*kNumJoyButtons; ++i)
    {
      buf >> key;
      myJoyTable[i] = (Event::Type) atoi(key.c_str());
    }
  }
  else
    setDefaultJoymap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::addKeyMapping(Event::Type event, uInt16 key)
{
  // These keys cannot be remapped.
  if(key == SDLK_TAB || key == SDLK_ESCAPE)
    return;

  myKeyTable[key] = event;
  saveKeyMapping();

  setActionMappings();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::addJoyMapping(Event::Type event, uInt8 stick, uInt32 code)
{
  myJoyTable[stick * kNumJoyButtons + code] = event;
  saveJoyMapping();

  setActionMappings();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::eraseMapping(Event::Type event)
{
  // Erase the KeyEvent arrays
  for(Int32 i = 0; i < SDLK_LAST; ++i)
    if(myKeyTable[i] == event && i != SDLK_TAB && i != SDLK_ESCAPE)
      myKeyTable[i] = Event::NoType;
  saveKeyMapping();

  // Erase the JoyEvent array
  for(Int32 i = 0; i < kNumJoysticks * kNumJoyButtons; ++i)
    if(myJoyTable[i] == event)
      myJoyTable[i] = Event::NoType;
  saveJoyMapping();

  setActionMappings();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultMapping()
{
  setDefaultKeymap();
  setDefaultJoymap();

  setActionMappings();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultKeymap()
{
  // Erase all mappings
  for(Int32 i = 0; i < SDLK_LAST; ++i)
    myKeyTable[i] = Event::NoType;

  myKeyTable[ SDLK_1 ]         = Event::KeyboardZero1;
  myKeyTable[ SDLK_2 ]         = Event::KeyboardZero2;
  myKeyTable[ SDLK_3 ]         = Event::KeyboardZero3;
  myKeyTable[ SDLK_q ]         = Event::KeyboardZero4;
  myKeyTable[ SDLK_w ]         = Event::KeyboardZero5;
  myKeyTable[ SDLK_e ]         = Event::KeyboardZero6;
  myKeyTable[ SDLK_a ]         = Event::KeyboardZero7;
  myKeyTable[ SDLK_s ]         = Event::KeyboardZero8;
  myKeyTable[ SDLK_d ]         = Event::KeyboardZero9;
  myKeyTable[ SDLK_z ]         = Event::KeyboardZeroStar;
  myKeyTable[ SDLK_x ]         = Event::KeyboardZero0;
  myKeyTable[ SDLK_c ]         = Event::KeyboardZeroPound;

  myKeyTable[ SDLK_8 ]         = Event::KeyboardOne1;
  myKeyTable[ SDLK_9 ]         = Event::KeyboardOne2;
  myKeyTable[ SDLK_0 ]         = Event::KeyboardOne3;
  myKeyTable[ SDLK_i ]         = Event::KeyboardOne4;
  myKeyTable[ SDLK_o ]         = Event::KeyboardOne5;
  myKeyTable[ SDLK_p ]         = Event::KeyboardOne6;
  myKeyTable[ SDLK_k ]         = Event::KeyboardOne7;
  myKeyTable[ SDLK_l ]         = Event::KeyboardOne8;
  myKeyTable[ SDLK_SEMICOLON ] = Event::KeyboardOne9;
  myKeyTable[ SDLK_COMMA ]     = Event::KeyboardOneStar;
  myKeyTable[ SDLK_PERIOD ]    = Event::KeyboardOne0;
  myKeyTable[ SDLK_SLASH ]     = Event::KeyboardOnePound;

  myKeyTable[ SDLK_UP ]        = Event::JoystickZeroUp;
  myKeyTable[ SDLK_DOWN ]      = Event::JoystickZeroDown;
  myKeyTable[ SDLK_LEFT ]      = Event::JoystickZeroLeft;
  myKeyTable[ SDLK_RIGHT ]     = Event::JoystickZeroRight;
  myKeyTable[ SDLK_SPACE ]     = Event::JoystickZeroFire;
  myKeyTable[ SDLK_4 ]         = Event::BoosterGripZeroTrigger;
  myKeyTable[ SDLK_5 ]         = Event::BoosterGripZeroBooster;

  myKeyTable[ SDLK_y ]         = Event::JoystickOneUp;
  myKeyTable[ SDLK_h ]         = Event::JoystickOneDown;
  myKeyTable[ SDLK_g ]         = Event::JoystickOneLeft;
  myKeyTable[ SDLK_j ]         = Event::JoystickOneRight;
  myKeyTable[ SDLK_f ]         = Event::JoystickOneFire;
  myKeyTable[ SDLK_6 ]         = Event::BoosterGripOneTrigger;
  myKeyTable[ SDLK_7 ]         = Event::BoosterGripOneBooster;

  myKeyTable[ SDLK_INSERT ]    = Event::DrivingZeroCounterClockwise;
  myKeyTable[ SDLK_PAGEUP ]    = Event::DrivingZeroClockwise;
  myKeyTable[ SDLK_HOME ]      = Event::DrivingZeroFire;

  myKeyTable[ SDLK_DELETE ]    = Event::DrivingOneCounterClockwise;
  myKeyTable[ SDLK_PAGEDOWN ]  = Event::DrivingOneClockwise;
  myKeyTable[ SDLK_END ]       = Event::DrivingOneFire;

  myKeyTable[ SDLK_F1 ]        = Event::ConsoleSelect;
  myKeyTable[ SDLK_F2 ]        = Event::ConsoleReset;
  myKeyTable[ SDLK_F3 ]        = Event::ConsoleColor;
  myKeyTable[ SDLK_F4 ]        = Event::ConsoleBlackWhite;
  myKeyTable[ SDLK_F5 ]        = Event::ConsoleLeftDifficultyA;
  myKeyTable[ SDLK_F6 ]        = Event::ConsoleLeftDifficultyB;
  myKeyTable[ SDLK_F7 ]        = Event::ConsoleRightDifficultyA;
  myKeyTable[ SDLK_F8 ]        = Event::ConsoleRightDifficultyB;
  myKeyTable[ SDLK_F9 ]        = Event::SaveState;
  myKeyTable[ SDLK_F10 ]       = Event::ChangeState;
  myKeyTable[ SDLK_F11 ]       = Event::LoadState;
  myKeyTable[ SDLK_F12 ]       = Event::TakeSnapshot;
  myKeyTable[ SDLK_PAUSE ]     = Event::Pause;
  myKeyTable[ SDLK_TAB ]       = Event::MenuMode;
  myKeyTable[ SDLK_BACKQUOTE ] = Event::DebuggerMode;
  myKeyTable[ SDLK_ESCAPE ]    = Event::LauncherMode;

  saveKeyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoymap()
{
  uInt32 i;

  // Erase all mappings
  for(i = 0; i < kNumJoysticks * kNumJoyButtons; ++i)
    myJoyTable[i] = Event::NoType;

  // Left joystick
  i = 0 * kNumJoyButtons;
  myJoyTable[i + kJAxisUp]    = Event::JoystickZeroUp;
  myJoyTable[i + kJAxisDown]  = Event::JoystickZeroDown;
  myJoyTable[i + kJAxisLeft]  = Event::JoystickZeroLeft;
  myJoyTable[i + kJAxisRight] = Event::JoystickZeroRight;
  myJoyTable[i + 0]           = Event::JoystickZeroFire;

  // Right joystick
  i = 1 * kNumJoyButtons;
  myJoyTable[i + kJAxisUp]    = Event::JoystickOneUp;
  myJoyTable[i + kJAxisDown]  = Event::JoystickOneDown;
  myJoyTable[i + kJAxisLeft]  = Event::JoystickOneLeft;
  myJoyTable[i + kJAxisRight] = Event::JoystickOneRight;
  myJoyTable[i + 0] 		  = Event::JoystickOneFire;

  saveJoyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveKeyMapping()
{
  // Iterate through the keymap table and create a colon-separated list
  ostringstream keybuf;
  for(uInt32 i = 0; i < SDLK_LAST; ++i)
    keybuf << myKeyTable[i] << ":";

  myOSystem->settings().setString("keymap", keybuf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveJoyMapping()
{
  // Iterate through the joymap table and create a colon-separated list
  ostringstream joybuf;
  for(Int32 i = 0; i < kNumJoysticks * kNumJoyButtons; ++i)
    joybuf << myJoyTable[i] << ":";

  myOSystem->settings().setString("joymap", joybuf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::isValidList(string list, uInt32 length)
{
  // Rudimentary check to see if the list contains 'length' keys
  istringstream buf(list);
  string key;
  uInt32 i = 0;

  while(buf >> key)
    i++;

  return (i == length);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveState()
{
  // Do a state save using the System
  string md5 = myOSystem->console().properties().get("Cartridge.MD5");
  ostringstream buf;
  buf << myOSystem->stateDir() << BSPF_PATH_SEPARATOR << md5 << ".st" << myLSState;

  int result = myOSystem->console().system().saveState(buf.str(), md5);

  // Print appropriate message
  buf.str("");
  if(result == 1)
    buf << "State " << myLSState << " saved";
  else if(result == 2)
    buf << "Error saving state " << myLSState;
  else if(result == 3)
    buf << "Invalid state " << myLSState << " file";

  myOSystem->frameBuffer().showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::changeState()
{
  if(myLSState == 9)
    myLSState = 0;
  else
    ++myLSState;

  // Print appropriate message
  ostringstream buf;
  buf << "Changed to slot " << myLSState;

  myOSystem->frameBuffer().showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::loadState()
{
  // Do a state save using the System
  string md5 = myOSystem->console().properties().get("Cartridge.MD5");
  ostringstream buf;
  buf << myOSystem->stateDir() << BSPF_PATH_SEPARATOR << md5 << ".st" << myLSState;

  int result = myOSystem->console().system().loadState(buf.str(), md5);

  // Print appropriate message
  buf.str("");
  if(result == 1)
    buf << "State " << myLSState << " loaded";
  else if(result == 2)
    buf << "Error loading state " << myLSState;
  else if(result == 3)
    buf << "Invalid state " << myLSState << " file";

  myOSystem->frameBuffer().showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::takeSnapshot()
{
#ifdef SNAPSHOT_SUPPORT
  // Figure out the correct snapshot name
  string filename;
  string sspath = myOSystem->settings().getString("ssdir");
  string ssname = myOSystem->settings().getString("ssname");

  if(ssname == "romname")
    sspath = sspath + BSPF_PATH_SEPARATOR +
             myOSystem->console().properties().get("Cartridge.Name");
  else if(ssname == "md5sum")
    sspath = sspath + BSPF_PATH_SEPARATOR +
             myOSystem->console().properties().get("Cartridge.MD5");

  // Check whether we want multiple snapshots created
  if(!myOSystem->settings().getBool("sssingle"))
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    filename = sspath + ".png";
    if(FilesystemNode::fileExists(filename))
    {
      ostringstream buf;
      for(uInt32 i = 1; ;++i)
      {
        buf.str("");
        buf << sspath << "_" << i << ".png";
        if(!FilesystemNode::fileExists(buf.str()))
          break;
      }
      filename = buf.str();
    }
  }
  else
    filename = sspath + ".png";

  // Now create a Snapshot object and save the PNG
  myOSystem->frameBuffer().refresh(true);
  Snapshot snapshot(myOSystem->frameBuffer());
  string result = snapshot.savePNG(filename);
  myOSystem->frameBuffer().showMessage(result);
#else
  myOSystem->frameBuffer().showMessage("Snapshots unsupported");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setPaddleMode(uInt32 num, bool showmessage)
{
  myPaddleMode = num;

  if(showmessage)
  {
    ostringstream buf;
    buf << "Mouse is paddle " << num;
    myOSystem->frameBuffer().showMessage(buf.str());
  }

  myOSystem->settings().setInt("paddle", myPaddleMode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::enterMenuMode()
{
  myState = S_MENU;
  myOSystem->menu().reStack();
  myOSystem->frameBuffer().refresh();
  myOSystem->frameBuffer().setCursorState();
  myOSystem->sound().mute(true);
  myEvent->clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveMenuMode()
{
  myState = S_EMULATE;
  myOSystem->frameBuffer().refresh();
  myOSystem->frameBuffer().setCursorState();
  myOSystem->sound().mute(false);
  myEvent->clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::enterDebugMode()
{
  myState = S_DEBUGGER;
  myOSystem->createFrameBuffer();
  myOSystem->debugger().reStack();
  myOSystem->frameBuffer().refresh();
  myOSystem->frameBuffer().setCursorState();
  myEvent->clear();

  if(!myPauseFlag)  // Pause when entering debugger mode
    handleEvent(Event::Pause, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveDebugMode()
{
  myState = S_EMULATE;
  myOSystem->createFrameBuffer();
  myOSystem->frameBuffer().refresh();
  myOSystem->frameBuffer().setCursorState();
  myEvent->clear();

  if(myPauseFlag)  // Un-Pause when leaving debugger mode
    handleEvent(Event::Pause, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setSDLMappings()
{
  ourSDLMapping[ SDLK_BACKSPACE ]    = "BACKSPACE";
  ourSDLMapping[ SDLK_TAB ]          = "TAB";
  ourSDLMapping[ SDLK_CLEAR ]        = "CLEAR";
  ourSDLMapping[ SDLK_RETURN ]       = "RETURN";
  ourSDLMapping[ SDLK_PAUSE ]        = "PAUSE";
  ourSDLMapping[ SDLK_ESCAPE ]       = "ESCAPE";
  ourSDLMapping[ SDLK_SPACE ]        = "SPACE";
  ourSDLMapping[ SDLK_EXCLAIM ]      = "EXCLAIM";
  ourSDLMapping[ SDLK_QUOTEDBL ]     = "QUOTEDBL";
  ourSDLMapping[ SDLK_HASH ]         = "HASH";
  ourSDLMapping[ SDLK_DOLLAR ]       = "DOLLAR";
  ourSDLMapping[ SDLK_AMPERSAND ]    = "AMPERSAND";
  ourSDLMapping[ SDLK_QUOTE ]        = "QUOTE";
  ourSDLMapping[ SDLK_LEFTPAREN ]    = "LEFTPAREN";
  ourSDLMapping[ SDLK_RIGHTPAREN ]   = "RIGHTPAREN";
  ourSDLMapping[ SDLK_ASTERISK ]     = "ASTERISK";
  ourSDLMapping[ SDLK_PLUS ]         = "PLUS";
  ourSDLMapping[ SDLK_COMMA ]        = "COMMA";
  ourSDLMapping[ SDLK_MINUS ]        = "MINUS";
  ourSDLMapping[ SDLK_PERIOD ]       = "PERIOD";
  ourSDLMapping[ SDLK_SLASH ]        = "SLASH";
  ourSDLMapping[ SDLK_0 ]            = "0";
  ourSDLMapping[ SDLK_1 ]            = "1";
  ourSDLMapping[ SDLK_2 ]            = "2";
  ourSDLMapping[ SDLK_3 ]            = "3";
  ourSDLMapping[ SDLK_4 ]            = "4";
  ourSDLMapping[ SDLK_5 ]            = "5";
  ourSDLMapping[ SDLK_6 ]            = "6";
  ourSDLMapping[ SDLK_7 ]            = "7";
  ourSDLMapping[ SDLK_8 ]            = "8";
  ourSDLMapping[ SDLK_9 ]            = "9";
  ourSDLMapping[ SDLK_COLON ]        = "COLON";
  ourSDLMapping[ SDLK_SEMICOLON ]    = "SEMICOLON";
  ourSDLMapping[ SDLK_LESS ]         = "LESS";
  ourSDLMapping[ SDLK_EQUALS ]       = "EQUALS";
  ourSDLMapping[ SDLK_GREATER ]      = "GREATER";
  ourSDLMapping[ SDLK_QUESTION ]     = "QUESTION";
  ourSDLMapping[ SDLK_AT ]           = "AT";
  ourSDLMapping[ SDLK_LEFTBRACKET ]  = "LEFTBRACKET";
  ourSDLMapping[ SDLK_BACKSLASH ]    = "BACKSLASH";
  ourSDLMapping[ SDLK_RIGHTBRACKET ] = "RIGHTBRACKET";
  ourSDLMapping[ SDLK_CARET ]        = "CARET";
  ourSDLMapping[ SDLK_UNDERSCORE ]   = "UNDERSCORE";
  ourSDLMapping[ SDLK_BACKQUOTE ]    = "BACKQUOTE";
  ourSDLMapping[ SDLK_a ]            = "A";
  ourSDLMapping[ SDLK_b ]            = "B";
  ourSDLMapping[ SDLK_c ]            = "C";
  ourSDLMapping[ SDLK_d ]            = "D";
  ourSDLMapping[ SDLK_e ]            = "E";
  ourSDLMapping[ SDLK_f ]            = "F";
  ourSDLMapping[ SDLK_g ]            = "G";
  ourSDLMapping[ SDLK_h ]            = "H";
  ourSDLMapping[ SDLK_i ]            = "I";
  ourSDLMapping[ SDLK_j ]            = "J";
  ourSDLMapping[ SDLK_k ]            = "K";
  ourSDLMapping[ SDLK_l ]            = "L";
  ourSDLMapping[ SDLK_m ]            = "M";
  ourSDLMapping[ SDLK_n ]            = "N";
  ourSDLMapping[ SDLK_o ]            = "O";
  ourSDLMapping[ SDLK_p ]            = "P";
  ourSDLMapping[ SDLK_q ]            = "Q";
  ourSDLMapping[ SDLK_r ]            = "R";
  ourSDLMapping[ SDLK_s ]            = "S";
  ourSDLMapping[ SDLK_t ]            = "T";
  ourSDLMapping[ SDLK_u ]            = "U";
  ourSDLMapping[ SDLK_v ]            = "V";
  ourSDLMapping[ SDLK_w ]            = "W";
  ourSDLMapping[ SDLK_x ]            = "X";
  ourSDLMapping[ SDLK_y ]            = "Y";
  ourSDLMapping[ SDLK_z ]            = "Z";
  ourSDLMapping[ SDLK_DELETE ]       = "DELETE";
  ourSDLMapping[ SDLK_WORLD_0 ]      = "WORLD_0";
  ourSDLMapping[ SDLK_WORLD_1 ]      = "WORLD_1";
  ourSDLMapping[ SDLK_WORLD_2 ]      = "WORLD_2";
  ourSDLMapping[ SDLK_WORLD_3 ]      = "WORLD_3";
  ourSDLMapping[ SDLK_WORLD_4 ]      = "WORLD_4";
  ourSDLMapping[ SDLK_WORLD_5 ]      = "WORLD_5";
  ourSDLMapping[ SDLK_WORLD_6 ]      = "WORLD_6";
  ourSDLMapping[ SDLK_WORLD_7 ]      = "WORLD_7";
  ourSDLMapping[ SDLK_WORLD_8 ]      = "WORLD_8";
  ourSDLMapping[ SDLK_WORLD_9 ]      = "WORLD_9";
  ourSDLMapping[ SDLK_WORLD_10 ]     = "WORLD_10";
  ourSDLMapping[ SDLK_WORLD_11 ]     = "WORLD_11";
  ourSDLMapping[ SDLK_WORLD_12 ]     = "WORLD_12";
  ourSDLMapping[ SDLK_WORLD_13 ]     = "WORLD_13";
  ourSDLMapping[ SDLK_WORLD_14 ]     = "WORLD_14";
  ourSDLMapping[ SDLK_WORLD_15 ]     = "WORLD_15";
  ourSDLMapping[ SDLK_WORLD_16 ]     = "WORLD_16";
  ourSDLMapping[ SDLK_WORLD_17 ]     = "WORLD_17";
  ourSDLMapping[ SDLK_WORLD_18 ]     = "WORLD_18";
  ourSDLMapping[ SDLK_WORLD_19 ]     = "WORLD_19";
  ourSDLMapping[ SDLK_WORLD_20 ]     = "WORLD_20";
  ourSDLMapping[ SDLK_WORLD_21 ]     = "WORLD_21";
  ourSDLMapping[ SDLK_WORLD_22 ]     = "WORLD_22";
  ourSDLMapping[ SDLK_WORLD_23 ]     = "WORLD_23";
  ourSDLMapping[ SDLK_WORLD_24 ]     = "WORLD_24";
  ourSDLMapping[ SDLK_WORLD_25 ]     = "WORLD_25";
  ourSDLMapping[ SDLK_WORLD_26 ]     = "WORLD_26";
  ourSDLMapping[ SDLK_WORLD_27 ]     = "WORLD_27";
  ourSDLMapping[ SDLK_WORLD_28 ]     = "WORLD_28";
  ourSDLMapping[ SDLK_WORLD_29 ]     = "WORLD_29";
  ourSDLMapping[ SDLK_WORLD_30 ]     = "WORLD_30";
  ourSDLMapping[ SDLK_WORLD_31 ]     = "WORLD_31";
  ourSDLMapping[ SDLK_WORLD_32 ]     = "WORLD_32";
  ourSDLMapping[ SDLK_WORLD_33 ]     = "WORLD_33";
  ourSDLMapping[ SDLK_WORLD_34 ]     = "WORLD_34";
  ourSDLMapping[ SDLK_WORLD_35 ]     = "WORLD_35";
  ourSDLMapping[ SDLK_WORLD_36 ]     = "WORLD_36";
  ourSDLMapping[ SDLK_WORLD_37 ]     = "WORLD_37";
  ourSDLMapping[ SDLK_WORLD_38 ]     = "WORLD_38";
  ourSDLMapping[ SDLK_WORLD_39 ]     = "WORLD_39";
  ourSDLMapping[ SDLK_WORLD_40 ]     = "WORLD_40";
  ourSDLMapping[ SDLK_WORLD_41 ]     = "WORLD_41";
  ourSDLMapping[ SDLK_WORLD_42 ]     = "WORLD_42";
  ourSDLMapping[ SDLK_WORLD_43 ]     = "WORLD_43";
  ourSDLMapping[ SDLK_WORLD_44 ]     = "WORLD_44";
  ourSDLMapping[ SDLK_WORLD_45 ]     = "WORLD_45";
  ourSDLMapping[ SDLK_WORLD_46 ]     = "WORLD_46";
  ourSDLMapping[ SDLK_WORLD_47 ]     = "WORLD_47";
  ourSDLMapping[ SDLK_WORLD_48 ]     = "WORLD_48";
  ourSDLMapping[ SDLK_WORLD_49 ]     = "WORLD_49";
  ourSDLMapping[ SDLK_WORLD_50 ]     = "WORLD_50";
  ourSDLMapping[ SDLK_WORLD_51 ]     = "WORLD_51";
  ourSDLMapping[ SDLK_WORLD_52 ]     = "WORLD_52";
  ourSDLMapping[ SDLK_WORLD_53 ]     = "WORLD_53";
  ourSDLMapping[ SDLK_WORLD_54 ]     = "WORLD_54";
  ourSDLMapping[ SDLK_WORLD_55 ]     = "WORLD_55";
  ourSDLMapping[ SDLK_WORLD_56 ]     = "WORLD_56";
  ourSDLMapping[ SDLK_WORLD_57 ]     = "WORLD_57";
  ourSDLMapping[ SDLK_WORLD_58 ]     = "WORLD_58";
  ourSDLMapping[ SDLK_WORLD_59 ]     = "WORLD_59";
  ourSDLMapping[ SDLK_WORLD_60 ]     = "WORLD_60";
  ourSDLMapping[ SDLK_WORLD_61 ]     = "WORLD_61";
  ourSDLMapping[ SDLK_WORLD_62 ]     = "WORLD_62";
  ourSDLMapping[ SDLK_WORLD_63 ]     = "WORLD_63";
  ourSDLMapping[ SDLK_WORLD_64 ]     = "WORLD_64";
  ourSDLMapping[ SDLK_WORLD_65 ]     = "WORLD_65";
  ourSDLMapping[ SDLK_WORLD_66 ]     = "WORLD_66";
  ourSDLMapping[ SDLK_WORLD_67 ]     = "WORLD_67";
  ourSDLMapping[ SDLK_WORLD_68 ]     = "WORLD_68";
  ourSDLMapping[ SDLK_WORLD_69 ]     = "WORLD_69";
  ourSDLMapping[ SDLK_WORLD_70 ]     = "WORLD_70";
  ourSDLMapping[ SDLK_WORLD_71 ]     = "WORLD_71";
  ourSDLMapping[ SDLK_WORLD_72 ]     = "WORLD_72";
  ourSDLMapping[ SDLK_WORLD_73 ]     = "WORLD_73";
  ourSDLMapping[ SDLK_WORLD_74 ]     = "WORLD_74";
  ourSDLMapping[ SDLK_WORLD_75 ]     = "WORLD_75";
  ourSDLMapping[ SDLK_WORLD_76 ]     = "WORLD_76";
  ourSDLMapping[ SDLK_WORLD_77 ]     = "WORLD_77";
  ourSDLMapping[ SDLK_WORLD_78 ]     = "WORLD_78";
  ourSDLMapping[ SDLK_WORLD_79 ]     = "WORLD_79";
  ourSDLMapping[ SDLK_WORLD_80 ]     = "WORLD_80";
  ourSDLMapping[ SDLK_WORLD_81 ]     = "WORLD_81";
  ourSDLMapping[ SDLK_WORLD_82 ]     = "WORLD_82";
  ourSDLMapping[ SDLK_WORLD_83 ]     = "WORLD_83";
  ourSDLMapping[ SDLK_WORLD_84 ]     = "WORLD_84";
  ourSDLMapping[ SDLK_WORLD_85 ]     = "WORLD_85";
  ourSDLMapping[ SDLK_WORLD_86 ]     = "WORLD_86";
  ourSDLMapping[ SDLK_WORLD_87 ]     = "WORLD_87";
  ourSDLMapping[ SDLK_WORLD_88 ]     = "WORLD_88";
  ourSDLMapping[ SDLK_WORLD_89 ]     = "WORLD_89";
  ourSDLMapping[ SDLK_WORLD_90 ]     = "WORLD_90";
  ourSDLMapping[ SDLK_WORLD_91 ]     = "WORLD_91";
  ourSDLMapping[ SDLK_WORLD_92 ]     = "WORLD_92";
  ourSDLMapping[ SDLK_WORLD_93 ]     = "WORLD_93";
  ourSDLMapping[ SDLK_WORLD_94 ]     = "WORLD_94";
  ourSDLMapping[ SDLK_WORLD_95 ]     = "WORLD_95";
  ourSDLMapping[ SDLK_KP0 ]          = "KP0";
  ourSDLMapping[ SDLK_KP1 ]          = "KP1";
  ourSDLMapping[ SDLK_KP2 ]          = "KP2";
  ourSDLMapping[ SDLK_KP3 ]          = "KP3";
  ourSDLMapping[ SDLK_KP4 ]          = "KP4";
  ourSDLMapping[ SDLK_KP5 ]          = "KP5";
  ourSDLMapping[ SDLK_KP6 ]          = "KP6";
  ourSDLMapping[ SDLK_KP7 ]          = "KP7";
  ourSDLMapping[ SDLK_KP8 ]          = "KP8";
  ourSDLMapping[ SDLK_KP9 ]          = "KP9";
  ourSDLMapping[ SDLK_KP_PERIOD ]    = "KP_PERIOD";
  ourSDLMapping[ SDLK_KP_DIVIDE ]    = "KP_DIVIDE";
  ourSDLMapping[ SDLK_KP_MULTIPLY ]  = "KP_MULTIPLY";
  ourSDLMapping[ SDLK_KP_MINUS ]     = "KP_MINUS";
  ourSDLMapping[ SDLK_KP_PLUS ]      = "KP_PLUS";
  ourSDLMapping[ SDLK_KP_ENTER ]     = "KP_ENTER";
  ourSDLMapping[ SDLK_KP_EQUALS ]    = "KP_EQUALS";
  ourSDLMapping[ SDLK_UP ]           = "UP";
  ourSDLMapping[ SDLK_DOWN ]         = "DOWN";
  ourSDLMapping[ SDLK_RIGHT ]        = "RIGHT";
  ourSDLMapping[ SDLK_LEFT ]         = "LEFT";
  ourSDLMapping[ SDLK_INSERT ]       = "INSERT";
  ourSDLMapping[ SDLK_HOME ]         = "HOME";
  ourSDLMapping[ SDLK_END ]          = "END";
  ourSDLMapping[ SDLK_PAGEUP ]       = "PAGEUP";
  ourSDLMapping[ SDLK_PAGEDOWN ]     = "PAGEDOWN";
  ourSDLMapping[ SDLK_F1 ]           = "F1";
  ourSDLMapping[ SDLK_F2 ]           = "F2";
  ourSDLMapping[ SDLK_F3 ]           = "F3";
  ourSDLMapping[ SDLK_F4 ]           = "F4";
  ourSDLMapping[ SDLK_F5 ]           = "F5";
  ourSDLMapping[ SDLK_F6 ]           = "F6";
  ourSDLMapping[ SDLK_F7 ]           = "F7";
  ourSDLMapping[ SDLK_F8 ]           = "F8";
  ourSDLMapping[ SDLK_F9 ]           = "F9";
  ourSDLMapping[ SDLK_F10 ]          = "F10";
  ourSDLMapping[ SDLK_F11 ]          = "F11";
  ourSDLMapping[ SDLK_F12 ]          = "F12";
  ourSDLMapping[ SDLK_F13 ]          = "F13";
  ourSDLMapping[ SDLK_F14 ]          = "F14";
  ourSDLMapping[ SDLK_F15 ]          = "F15";
  ourSDLMapping[ SDLK_NUMLOCK ]      = "NUMLOCK";
  ourSDLMapping[ SDLK_CAPSLOCK ]     = "CAPSLOCK";
  ourSDLMapping[ SDLK_SCROLLOCK ]    = "SCROLLOCK";
  ourSDLMapping[ SDLK_RSHIFT ]       = "RSHIFT";
  ourSDLMapping[ SDLK_LSHIFT ]       = "LSHIFT";
  ourSDLMapping[ SDLK_RCTRL ]        = "RCTRL";
  ourSDLMapping[ SDLK_LCTRL ]        = "LCTRL";
  ourSDLMapping[ SDLK_RALT ]         = "RALT";
  ourSDLMapping[ SDLK_LALT ]         = "LALT";
  ourSDLMapping[ SDLK_RMETA ]        = "RMETA";
  ourSDLMapping[ SDLK_LMETA ]        = "LMETA";
  ourSDLMapping[ SDLK_LSUPER ]       = "LSUPER";
  ourSDLMapping[ SDLK_RSUPER ]       = "RSUPER";
  ourSDLMapping[ SDLK_MODE ]         = "MODE";
  ourSDLMapping[ SDLK_COMPOSE ]      = "COMPOSE";
  ourSDLMapping[ SDLK_HELP ]         = "HELP";
  ourSDLMapping[ SDLK_PRINT ]        = "PRINT";
  ourSDLMapping[ SDLK_SYSREQ ]       = "SYSREQ";
  ourSDLMapping[ SDLK_BREAK ]        = "BREAK";
  ourSDLMapping[ SDLK_MENU ]         = "MENU";
  ourSDLMapping[ SDLK_POWER ]        = "POWER";
  ourSDLMapping[ SDLK_EURO ]         = "EURO";
  ourSDLMapping[ SDLK_UNDO ]         = "UNDO";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionList EventHandler::ourActionList[61] = {
  { Event::ConsoleSelect,               "Select",                          "" },
  { Event::ConsoleReset,                "Reset",                           "" },
  { Event::ConsoleColor,                "Color TV",                        "" },
  { Event::ConsoleBlackWhite,           "Black & White TV",                "" },
  { Event::ConsoleLeftDifficultyA,      "P1 Difficulty A",                 "" },
  { Event::ConsoleLeftDifficultyB,      "P1 Difficulty B",                 "" },
  { Event::ConsoleRightDifficultyA,     "P2 Difficulty A",                 "" },
  { Event::ConsoleRightDifficultyB,     "P2 Difficulty B",                 "" },
  { Event::SaveState,                   "Save State",                      "" },
  { Event::ChangeState,                 "Change State",                    "" },
  { Event::LoadState,                   "Load State",                      "" },
  { Event::TakeSnapshot,                "Snapshot",                        "" },
  { Event::Pause,                       "Pause",                           "" },
  { Event::MenuMode,                    "Toggle menu/options mode",        "" },
  { Event::DebuggerMode,                "Toggle debugger mode",            "" },
  { Event::LauncherMode,                "Enter ROM launcher",              "" },
  { Event::Quit,                        "Quit",                            "" },

  { Event::JoystickZeroUp,              "P1 Joystick Up",                  "" },
  { Event::JoystickZeroDown,            "P1 Joystick Down",                "" },
  { Event::JoystickZeroLeft,            "P1 Joystick Left",                "" },
  { Event::JoystickZeroRight,           "P1 Joystick Right",               "" },
  { Event::JoystickZeroFire,            "P1 Joystick Fire",                "" },

  { Event::JoystickOneUp,               "P2 Joystick Up",                  "" },
  { Event::JoystickOneDown,             "P2 Joystick Down",                "" },
  { Event::JoystickOneLeft,             "P2 Joystick Left",                "" },
  { Event::JoystickOneRight,            "P2 Joystick Right",               "" },
  { Event::JoystickOneFire,             "P2 Joystick Fire",                "" },

  { Event::BoosterGripZeroTrigger,      "P1 Booster-Grip Trigger",         "" },
  { Event::BoosterGripZeroBooster,      "P1 Booster-Grip Booster",         "" },

  { Event::BoosterGripOneTrigger,       "P2 Booster-Grip Trigger",         "" },
  { Event::BoosterGripOneBooster,       "P2 Booster-Grip Booster",         "" },

  { Event::DrivingZeroCounterClockwise, "P1 Driving Controller Left",      "" },
  { Event::DrivingZeroClockwise,        "P1 Driving Controller Right",     "" },
  { Event::DrivingZeroFire,             "P1 Driving Controller Fire",      "" },

  { Event::DrivingOneCounterClockwise,  "P2 Driving Controller Left",      "" },
  { Event::DrivingOneClockwise,         "P2 Driving Controller Right",     "" },
  { Event::DrivingOneFire,              "P2 Driving Controller Fire",      "" },

  { Event::KeyboardZero1,               "P1 GamePad 1",                    "" },
  { Event::KeyboardZero2,               "P1 GamePad 2",                    "" },
  { Event::KeyboardZero3,               "P1 GamePad 3",                    "" },
  { Event::KeyboardZero4,               "P1 GamePad 4",                    "" },
  { Event::KeyboardZero5,               "P1 GamePad 5",                    "" },
  { Event::KeyboardZero6,               "P1 GamePad 6",                    "" },
  { Event::KeyboardZero7,               "P1 GamePad 7",                    "" },
  { Event::KeyboardZero8,               "P1 GamePad 8",                    "" },
  { Event::KeyboardZero9,               "P1 GamePad 9",                    "" },
  { Event::KeyboardZeroStar,            "P1 GamePad *",                    "" },
  { Event::KeyboardZero0,               "P1 GamePad 0",                    "" },
  { Event::KeyboardZeroPound,           "P1 GamePad #",                    "" },

  { Event::KeyboardOne1,                "P2 GamePad 1",                    "" },
  { Event::KeyboardOne2,                "P2 GamePad 2",                    "" },
  { Event::KeyboardOne3,                "P2 GamePad 3",                    "" },
  { Event::KeyboardOne4,                "P2 GamePad 4",                    "" },
  { Event::KeyboardOne5,                "P2 GamePad 5",                    "" },
  { Event::KeyboardOne6,                "P2 GamePad 6",                    "" },
  { Event::KeyboardOne7,                "P2 GamePad 7",                    "" },
  { Event::KeyboardOne8,                "P2 GamePad 8",                    "" },
  { Event::KeyboardOne9,                "P2 GamePad 9",                    "" },
  { Event::KeyboardOneStar,             "P2 GamePad *",                    "" },
  { Event::KeyboardOne0,                "P2 GamePad 0",                    "" },
  { Event::KeyboardOnePound,            "P2 GamePad #",                    "" }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::Type EventHandler::Paddle_Resistance[4] = {
  Event::PaddleZeroResistance, Event::PaddleOneResistance,
  Event::PaddleTwoResistance,  Event::PaddleThreeResistance
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::Type EventHandler::Paddle_Button[4] = {
  Event::PaddleZeroFire, Event::PaddleOneFire,
  Event::PaddleTwoFire,  Event::PaddleThreeFire
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::Type EventHandler::SA_Axis[2][2][3] = {
  { {Event::JoystickZeroLeft, Event::JoystickZeroRight, Event::PaddleZeroResistance},
    {Event::JoystickZeroUp,   Event::JoystickZeroDown,  Event::PaddleOneResistance }  },
  { {Event::JoystickOneLeft,  Event::JoystickOneRight,  Event::PaddleTwoResistance},
    {Event::JoystickOneUp,    Event::JoystickOneDown,   Event::PaddleThreeResistance} }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::Type EventHandler::SA_DrivingValue[2] = {
  Event::DrivingZeroValue, Event::DrivingOneValue
};
