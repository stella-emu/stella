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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventHandler.cxx,v 1.118 2005-11-21 13:47:34 stephena Exp $
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
#include "DialogContainer.hxx"
#include "Menu.hxx"
#include "CommandMenu.hxx"
#include "Launcher.hxx"
#include "GuiUtils.hxx"
#include "bspf.hxx"

#ifdef DEVELOPER_SUPPORT
  #include "Debugger.hxx"
#endif

#ifdef SNAPSHOT_SUPPORT
  #include "Snapshot.hxx"
#endif

#ifdef CHEATCODE_SUPPORT
  #include "CheatManager.hxx"
#endif

#ifdef MAC_OSX
  extern "C" {
    void handleMacOSXKeypress(int key);
  }
#endif

#define JOY_DEADZONE 3200

#ifdef PSP
  #define JOYMOUSE_LEFT_BUTTON 2
#else
  #define JOYMOUSE_LEFT_BUTTON 0
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(OSystem* osystem)
  : myOSystem(osystem),
    myState(S_NONE),
    myOverlay(NULL),
    myLSState(0),
    myPauseFlag(false),
    myQuitFlag(false),
    myGrabMouseFlag(false),
    myUseLauncherFlag(false),
    myEmulateMouseFlag(false),
    myPaddleMode(0),
    myMouseMove(3)
{
  uInt32 i;

  // Add this eventhandler object to the OSystem
  myOSystem->attach(this);

  // Create the event object which will be used for this handler
  myEvent = new Event();

  // Erase the KeyEvent arrays
  for(i = 0; i < SDLK_LAST; ++i)
  {
    myKeyTable[i] = Event::NoType;
    ourSDLMapping[i] = "";
  }

  // Erase the JoyEvent array
  for(i = 0; i < kNumJoysticks * kNumJoyButtons; ++i)
    myJoyTable[i] = Event::NoType;

  // Erase the Message array
  for(i = 0; i < Event::LastType; ++i)
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
  myEmulateMouseFlag = myOSystem->settings().getBool("joymouse");

  setPaddleMode(myOSystem->settings().getInt("paddle"), false);

  myFryingFlag = false;

  memset(&myJoyMouse, 0, sizeof(myJoyMouse));
  myJoyMouse.delay_time = 25;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::~EventHandler()
{
  ourJoystickNames.clear();

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
  setEventState(state);

  myLSState = 0;
  myPauseFlag = false;
  myQuitFlag = false;

  myOSystem->frameBuffer().pause(myPauseFlag);
  myOSystem->sound().mute(myPauseFlag);
  myEvent->clear();

  if(myState == S_LAUNCHER)
    myUseLauncherFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::refreshDisplay()
{
  // These are reset each time the display changes size
  myJoyMouse.x_max = myOSystem->frameBuffer().imageWidth();
  myJoyMouse.y_max = myOSystem->frameBuffer().imageHeight();
  myMouseMove      = myOSystem->frameBuffer().zoomLevel() * 3;

  switch(myState)
  {
    case S_EMULATE:
      myOSystem->frameBuffer().refresh();
      break;

    case S_MENU:
    case S_CMDMENU:
      myOSystem->frameBuffer().refresh();
    case S_LAUNCHER:
    case S_DEBUGGER:
      myOverlay->refresh();
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
  if(showinfo)
    cerr << "Joystick devices found:" << endl;
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
      ourJoysticks[i].name = "None";
      continue;
    }

    // Figure out what type of joystick this is
    if(name.find("Stelladaptor", 0) != string::npos)
    {
      saCount++;
      if(saCount > 2)  // Ignore more than 2 Stelladaptors
        continue;
      else if(saCount == 1)  // Type will be set by mapStelladaptors()
        ourJoysticks[i].name = "Stelladaptor 1";
      else if(saCount == 2)
        ourJoysticks[i].name = "Stelladaptor 2";

      if(showinfo)
        cout << "  " << i << ": " << ourJoysticks[i].name << endl;
    }
    else
    {
      ourJoysticks[i].type = JT_REGULAR;
      ourJoysticks[i].name = SDL_JoystickName(i);

      if(showinfo)
        cout << "  " << i << ": " << ourJoysticks[i].name
             << " with " << SDL_JoystickNumButtons(ourJoysticks[i].stick)
             << " buttons" << endl;
    }
  }
  if(showinfo)
    cout << endl;

  // Map the stelladaptors we've found according to the specified ports
  const string& sa1 = myOSystem->settings().getString("sa1");
  const string& sa2 = myOSystem->settings().getString("sa2");
  mapStelladaptors(sa1, sa2);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::mapStelladaptors(const string& sa1, const string& sa2)
{
#ifdef JOYSTICK_SUPPORT
  bool showinfo = myOSystem->settings().getBool("showinfo");

  for(int i = 0; i < kNumJoysticks; i++)
  {
    if(ourJoysticks[i].name == "Stelladaptor 1")
    {
      if(sa1 == "left")
      {
        ourJoysticks[i].type = JT_STELLADAPTOR_LEFT;
        if(showinfo)
          cout << "  Stelladaptor 1 emulates left joystick port\n";
      }
      else if(sa1 == "right")
      {
        ourJoysticks[i].type = JT_STELLADAPTOR_RIGHT;
        if(showinfo)
          cout << "  Stelladaptor 1 emulates right joystick port\n";
      }
    }
    else if(ourJoysticks[i].name == "Stelladaptor 2")
    {
      if(sa2 == "left")
      {
        ourJoysticks[i].type = JT_STELLADAPTOR_LEFT;
        if(showinfo)
          cout << "  Stelladaptor 2 emulates left joystick port\n";
      }
      else if(sa2 == "right")
      {
        ourJoysticks[i].type = JT_STELLADAPTOR_RIGHT;
        if(showinfo)
          cout << "  Stelladaptor 2 emulates right joystick port\n";
      }
    }
  }
  if(showinfo)
    cout << endl;

  myOSystem->settings().setString("sa1", sa1);
  myOSystem->settings().setString("sa2", sa2);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::poll(uInt32 time)
{
  SDL_Event event;

  // Handle joystick to mouse emulation
  if(myEmulateMouseFlag)
    handleJoyMouse(time);

  // Check for an event
  while(SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      // keyboard events
      case SDL_KEYUP:
      case SDL_KEYDOWN:
      {
        int unicode = event.key.keysym.unicode;
        SDLKey key  = event.key.keysym.sym;
        SDLMod mod  = event.key.keysym.mod;
        uInt8 state = event.key.type == SDL_KEYDOWN ? 1 : 0;

        // An attempt to speed up event processing
        // All SDL-specific event actions are accessed by either
        // Control/Cmd or Alt/Shift-Cmd keys.  So we quickly check for those.
        if(kbdAlt(mod) && state)
        {
          // These keys work in all states
          switch(int(key))
          {
    #ifndef MAC_OSX
            case SDLK_EQUALS:
              myOSystem->frameBuffer().resize(NextSize);
              break;

            case SDLK_MINUS:
              myOSystem->frameBuffer().resize(PreviousSize);
              break;

            case SDLK_RETURN:
              myOSystem->frameBuffer().toggleFullscreen();
              break;
    #endif
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

              case SDLK_s:  // Alt-s merges properties into user properties (user.pro)
                saveProperties();
                break;
            }
          }
        }
        else if(kbdControl(mod) && state)
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

            case SDLK_EQUALS:
              myOSystem->frameBuffer().resize(NextSize);
              break;

            case SDLK_MINUS:
              myOSystem->frameBuffer().resize(PreviousSize);
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
              case SDLK_c:
                enterMenuMode(S_MENU);
                myOSystem->menu().enterCheatMode();
                break;

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

              case SDLK_s:         // Ctrl-s saves properties to a file
                string newPropertiesFile = myOSystem->baseDir() + BSPF_PATH_SEPARATOR +
                  myOSystem->console().properties().get("Cartridge.Name") + ".pro";
                myOSystem->console().saveProperties(newPropertiesFile);
                break;
            }
          }
        }

        // Handle keys which switch eventhandler state
        // Arrange the logic to take advantage of short-circuit evaluation
        if(!(kbdControl(mod) || kbdShift(mod) || kbdAlt(mod)) &&
            state && eventStateChange(myKeyTable[key]))
          return;

        // Otherwise, let the event handler deal with it
        if(myState == S_EMULATE)
          handleEvent(myKeyTable[key], state);
        else if(myOverlay != NULL)
        {
          // Make sure the unicode field is valid
          if (key == SDLK_BACKSPACE || key == SDLK_DELETE ||
             (key >= SDLK_UP && key <= SDLK_PAGEDOWN))
            unicode = key;

          myOverlay->handleKeyEvent(unicode, key, mod, state);
        }

        break;  // SDL_KEYUP, SDL_KEYDOWN
      }

      case SDL_MOUSEMOTION:
        handleMouseMotionEvent(event);
        break;  // SDL_MOUSEMOTION

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
        refreshDisplay();
        break;  // SDL_VIDEOEXPOSE

#ifdef JOYSTICK_SUPPORT
      case SDL_JOYBUTTONUP:
      case SDL_JOYBUTTONDOWN:
      {
        if(event.jbutton.which >= kNumJoysticks)
          break;

        // Stelladaptors handle buttons differently than regular joysticks
        int type = ourJoysticks[event.jbutton.which].type;
        switch(type)
        {
          case JT_REGULAR:
          {
            if(event.jbutton.button >= kNumJoyButtons-4)
              return;

            int stick = event.jbutton.which;
            int code  = event.jbutton.button;
            int state = event.jbutton.state == SDL_PRESSED ? 1 : 0;

            handleJoyEvent(stick, code, state);
            break;  // Regular joystick button
          }

          case JT_STELLADAPTOR_LEFT:
          case JT_STELLADAPTOR_RIGHT:
          {
            int button = event.jbutton.button;
            int state  = event.jbutton.state == SDL_PRESSED ? 1 : 0;

            // Since we can't detect what controller is attached to a
            // Stelladaptor, we only send events based on controller
            // type in ROM properties
            switch((int)myController[type-2])
            {
              // Send button events for the joysticks
              case Controller::Joystick:
                myEvent->set(SA_Button[type-2][button][0], state);
                break;

              // Send axis events for the paddles
              case Controller::Paddles:
                myEvent->set(SA_Button[type-2][button][1], state);
                break;

              // Send events for the driving controllers
              case Controller::Driving:
                myEvent->set(SA_Button[type-2][button][2], state);
                break;
            }
            break;  // Stelladaptor button
          }
        }
        break;  // SDL_JOYBUTTONUP, SDL_JOYBUTTONDOWN
      }  

      case SDL_JOYAXISMOTION:
      {
        if(event.jbutton.which >= kNumJoysticks)
          break;

        // Stelladaptors handle axis differently than regular joysticks
        int type = ourJoysticks[event.jbutton.which].type;
        switch(type)
        {
          case JT_REGULAR:
          {
            int stick = event.jbutton.which;
            int axis  = event.jaxis.axis;
            int value = event.jaxis.value;

            // Handle emulation of mouse using the joystick
            if(myState == S_EMULATE)
            {
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
            }
            else
              handleMouseWarp(stick, axis, value);
            break;  // Regular joystick axis
          }

          case JT_STELLADAPTOR_LEFT:
          case JT_STELLADAPTOR_RIGHT:
          {
            int axis  = event.jaxis.axis;
            int value = event.jaxis.value;

            // Since we can't detect what controller is attached to a
            // Stelladaptor, we only send events based on controller
            // type in ROM properties
            switch((int)myController[type-2])
            {
              // Send axis events for the joysticks
              case Controller::Joystick:
                myEvent->set(SA_Axis[type-2][axis][0], (value < -16384) ? 1 : 0);
                myEvent->set(SA_Axis[type-2][axis][1], (value > 16384) ? 1 : 0);
                break;

              // Send axis events for the paddles
              case Controller::Paddles:
              {
                int resistance = (Int32) (1000000.0 * (32767 - value) / 65534);
                myEvent->set(SA_Axis[type-2][axis][2], resistance);
                break;
              }

              // Send events for the driving controllers
              case Controller::Driving:
                if(axis == 1)
                {
                  if(value <= -16384-4096)
                    myEvent->set(SA_DrivingValue[type-2],2);
                  else if(value > 16384+4096)
                    myEvent->set(SA_DrivingValue[type-2],1);
                  else if(value >= 16384-4096)
                    myEvent->set(SA_DrivingValue[type-2],0);
                  else
                    myEvent->set(SA_DrivingValue[type-2],3);
                }
            }
            break;  // Stelladaptor axis
          }
        }
        break;  // SDL_JOYAXISMOTION
      }
#endif  // JOYSTICK_SUPPORT
    }
  }

  // Update the current dialog container at regular intervals
  // Used to implement continuous events
  if(myOverlay)
    myOverlay->updateTime(time);

#ifdef CHEATCODE_SUPPORT
  const CheatList& cheats = myOSystem->cheat().perFrame();
  for(unsigned int i = 0; i < cheats.size(); i++)
    cheats[i]->evaluate();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseMotionEvent(SDL_Event& event)
{
  // Take window zooming into account
  int x = event.motion.x, y = event.motion.y;
  myJoyMouse.x = x;
  myJoyMouse.y = y;

  myOSystem->frameBuffer().translateCoords(&x, &y);

  // Determine which mode we're in, then send the event to the appropriate place
  switch(myState)
  {
    case S_EMULATE:
    {
      int w = myOSystem->frameBuffer().baseWidth();

      // Grabmouse introduces some lag into the mouse movement,
      // so we need to fudge the numbers a bit
      if(myGrabMouseFlag) x = MIN(w, (int) (x * 1.5));

      int resistance = (int)(1000000.0 * (w - x) / w);
      handleEvent(Paddle_Resistance[myPaddleMode], resistance);
      break;
    }

    case S_MENU:
    case S_CMDMENU:
    case S_LAUNCHER:
    case S_DEBUGGER:
      myOverlay->handleMouseMotionEvent(x, y, 0);
      break;

    default:
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
    case S_CMDMENU:
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

      myOverlay->handleMouseButtonEvent(button, x, y, state);
      break;
    }

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleJoyMouse(uInt32 time)
{
  bool mouseAccel = false;  // TODO - make this a commandline option

  if (time >= myJoyMouse.last_time + myJoyMouse.delay_time)
  {
    myJoyMouse.last_time = time;
    if (myJoyMouse.x_down_count == 1)
    {
      myJoyMouse.x_down_time = time;
      myJoyMouse.x_down_count = 2;
    }
    if (myJoyMouse.y_down_count == 1)
    {
      myJoyMouse.y_down_time = time;
      myJoyMouse.y_down_count = 2;
    }

    if (myJoyMouse.x_vel || myJoyMouse.y_vel)
    {
      if (myJoyMouse.x_down_count)
      {
        if (mouseAccel && time > myJoyMouse.x_down_time + myJoyMouse.delay_time * 12)
        {
          if (myJoyMouse.x_vel > 0)
            myJoyMouse.x_vel++;
          else
            myJoyMouse.x_vel--;
        }
        else if (time > myJoyMouse.x_down_time + myJoyMouse.delay_time * 8)
        {
          if (myJoyMouse.x_vel > 0)
            myJoyMouse.x_vel = myMouseMove;
          else
            myJoyMouse.x_vel = -myMouseMove;
        }
      }
      if (myJoyMouse.y_down_count)
      {
        if (mouseAccel && time > myJoyMouse.y_down_time + myJoyMouse.delay_time * 12)
        {
          if (myJoyMouse.y_vel > 0)
            myJoyMouse.y_vel++;
          else
            myJoyMouse.y_vel--;
        }
        else if (time > myJoyMouse.y_down_time + myJoyMouse.delay_time * 8)
        {
          if (myJoyMouse.y_vel > 0)
            myJoyMouse.y_vel = myMouseMove;
          else
            myJoyMouse.y_vel = -myMouseMove;
        }
      }

      myJoyMouse.x += myJoyMouse.x_vel;
      myJoyMouse.y += myJoyMouse.y_vel;

      if (myJoyMouse.x < 0)
      {
        myJoyMouse.x = 0;
        myJoyMouse.x_vel = -1;
        myJoyMouse.x_down_count = 1;
      }
      else if (myJoyMouse.x > myJoyMouse.x_max)
      {
        myJoyMouse.x = myJoyMouse.x_max;
        myJoyMouse.x_vel = 1;
        myJoyMouse.x_down_count = 1;
      }

      if (myJoyMouse.y < 0)
      {
        myJoyMouse.y = 0;
        myJoyMouse.y_vel = -1;
        myJoyMouse.y_down_count = 1;
      }
      else if (myJoyMouse.y > myJoyMouse.y_max)
      {
        myJoyMouse.y = myJoyMouse.y_max;
        myJoyMouse.y_vel = 1;
        myJoyMouse.y_down_count = 1;
      }

      SDL_WarpMouse(myJoyMouse.x, myJoyMouse.y);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseWarp(uInt8 stick, uInt8 axis, Int16 value)
{
  if(value > JOY_DEADZONE)
    value -= JOY_DEADZONE;
  else if(value < -JOY_DEADZONE )
    value += JOY_DEADZONE;
  else
    value = 0;

  if(axis == 0)  // X axis
  {
    if (value != 0)
    {
      myJoyMouse.x_vel = (value > 0) ? 1 : -1;
      myJoyMouse.x_down_count = 1;
    }
    else
    {
      myJoyMouse.x_vel = 0;
      myJoyMouse.x_down_count = 0;
    }
  }
  else if(axis == 1)  // Y axis
  {
    value = -value;

    if (value != 0)
    {
      myJoyMouse.y_vel = (-value > 0) ? 1 : -1;
      myJoyMouse.y_down_count = 1;
    }
    else
    {
      myJoyMouse.y_vel = 0;
      myJoyMouse.y_down_count = 0;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleJoyEvent(uInt8 stick, uInt32 code, uInt8 state)
{
  Event::Type event = myJoyTable[stick*kNumJoyButtons + code];

  // Joystick button zero acts as left mouse button and cannot be remapped
  if(myState != S_EMULATE && code == JOYMOUSE_LEFT_BUTTON &&
     myEmulateMouseFlag)
  {
    // This button acts as mouse button zero, and can never be remapped
    SDL_MouseButtonEvent mouseEvent;
    mouseEvent.type   = state ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    mouseEvent.button = SDL_BUTTON_LEFT;
    mouseEvent.state  = state ? SDL_PRESSED : SDL_RELEASED;
    mouseEvent.x      = myJoyMouse.x;
    mouseEvent.y      = myJoyMouse.y;

    handleMouseButtonEvent((SDL_Event&)mouseEvent, state);
    return;
  }
  else if(state && eventStateChange(event))
    return;

  // Determine which mode we're in, then send the event to the appropriate place
  if(myState == S_EMULATE)
    handleEvent(event, state);
  else if(myOverlay != NULL)
    myOverlay->handleJoyEvent(stick, code, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleEvent(Event::Type event, Int32 state)
{
  // Take care of special events that aren't part of the emulation core
  // or need to be preprocessed before passing them on
  switch((int)event)
  {
    ////////////////////////////////////////////////////////////////////////
    // Make sure that simultaneous events that are impossible on a real
    // Atari 2600 cannot happen.  Maybe this should be in the Event class??
    case Event::JoystickZeroUp:
      if(state) myEvent->set(Event::JoystickZeroDown, 0);
      break;

    case Event::JoystickZeroDown:
      if(state) myEvent->set(Event::JoystickZeroUp, 0);
      break;

    case Event::JoystickZeroLeft:
      if(state) myEvent->set(Event::JoystickZeroRight, 0);
      break;

    case Event::JoystickZeroRight:
      if(state) myEvent->set(Event::JoystickZeroLeft, 0);
      break;

    case Event::JoystickOneUp:
      if(state) myEvent->set(Event::JoystickOneDown, 0);
      break;

    case Event::JoystickOneDown:
      if(state) myEvent->set(Event::JoystickOneUp, 0);
      break;

    case Event::JoystickOneLeft:
      if(state) myEvent->set(Event::JoystickOneRight, 0);
      break;

    case Event::JoystickOneRight:
      if(state) myEvent->set(Event::JoystickOneLeft, 0);
      break;
    ////////////////////////////////////////////////////////////////////////

    case Event::NoType:  // Ignore unmapped events
      return;

    case Event::Fry:
      if(!myPauseFlag)
        myFryingFlag = bool(state);
      return;

    case Event::SaveState:
      if(state && !myPauseFlag) saveState();
      return;

    case Event::ChangeState:
      if(state && !myPauseFlag) changeState();
      return;

    case Event::LoadState:
      if(state && !myPauseFlag) loadState();
      return;

    case Event::TakeSnapshot:
      if(state && !myPauseFlag) takeSnapshot();
      return;

    case Event::Pause:
      if(state)
      {
        myPauseFlag = !myPauseFlag;
        myOSystem->frameBuffer().pause(myPauseFlag);
        myOSystem->sound().mute(myPauseFlag);
      }
      return;

    case Event::LauncherMode:
      // ExitGame will only work when we've launched stella using the ROM
      // launcher.  Otherwise, the only way to exit the main loop is to Quit.
      if(myState == S_EMULATE && myUseLauncherFlag && state)
      {
        myOSystem->settings().saveConfig();
        myOSystem->createLauncher();
      }
      return;

    case Event::Quit:
      if(state) myQuitFlag = true;
      myOSystem->settings().saveConfig();
      return;
  }

  if(state && ourMessageTable[event] != "")
    myOSystem->frameBuffer().showMessage(ourMessageTable[event]);

  // Otherwise, pass it to the emulation core
  myEvent->set(event, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::eventStateChange(Event::Type type)
{
  bool handled = true;

  switch(type)
  {
    case Event::MenuMode:
      if(!myPauseFlag)
      {
        if(myState == S_EMULATE)
          enterMenuMode(S_MENU);
        else if(myState == S_MENU)
          leaveMenuMode();
        else
          handled = false;
      }
      else
        handled = false;
      break;

    case Event::CmdMenuMode:
      if(!myPauseFlag)
      {
        if(myState == S_EMULATE)
          enterMenuMode(S_CMDMENU);
        else if(myState == S_CMDMENU)
          leaveMenuMode();
        else
          handled = false;
      }
      else
        handled = false;
      break;

    case Event::DebuggerMode:
      if(myState == S_EMULATE)
        enterDebugMode();
      else if(myState == S_DEBUGGER)
        leaveDebugMode();
      else
        handled = false;
      break;

    default:
      handled = false;
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setActionMappings()
{
  // Fill the ActionList with the current key and joystick mappings
  for(Int32 i = 0; i < kActionListSize; ++i)
  {
    uInt32 j;

    Event::Type event = ourActionList[i].event;
    ourActionList[i].key = "None";
    string key = "";
    for(j = 0; j < SDLK_LAST; ++j)   // size of myKeyTable
    {
      if(myKeyTable[j] == event)
      {
        if(key == "")
          key = key + ourSDLMapping[j];
        else
          key = key + ", " + ourSDLMapping[j];
      }
    }
    for(j = 0; j < kNumJoysticks * kNumJoyButtons; ++j)
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
  string list = myOSystem->settings().getString("keymap");
  IntArray map;

  if(isValidList(list, map, SDLK_LAST))
  {
    // Fill the keymap table with events
    for(Int32 i = 0; i < SDLK_LAST; ++i)
      myKeyTable[i] = (Event::Type) map[i];
  }
  else
    setDefaultKeymap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setJoymap()
{
  string list = myOSystem->settings().getString("joymap");
  IntArray map;

  if(isValidList(list, map, kNumJoysticks*kNumJoyButtons))
  {
    // Fill the joymap table with events
    for(Int32 i = 0; i < kNumJoysticks*kNumJoyButtons; ++i)
      myJoyTable[i] = (Event::Type) map[i];
  }
  else
    setDefaultJoymap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::addKeyMapping(Event::Type event, uInt16 key)
{
  // These keys cannot be remapped.
  if(key == SDLK_TAB)
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
  uInt32 i;

  // Erase the KeyEvent arrays
  for(i = 0; i < SDLK_LAST; ++i)
    if(myKeyTable[i] == event && i != SDLK_TAB)
      myKeyTable[i] = Event::NoType;
  saveKeyMapping();

  // Erase the JoyEvent array
  for(i = 0; i < kNumJoysticks * kNumJoyButtons; ++i)
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
  myKeyTable[ SDLK_LCTRL ]     = Event::JoystickZeroFire;
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
  myKeyTable[ SDLK_BACKSPACE ] = Event::Fry;
  myKeyTable[ SDLK_TAB ]       = Event::MenuMode;
  myKeyTable[ SDLK_BACKSLASH ] = Event::CmdMenuMode;
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

#ifdef PSP
  myJoyTable[i + 0]  = Event::TakeSnapshot;      // Triangle
  myJoyTable[i + 1]  = Event::LoadState;         // Circle
  myJoyTable[i + 2]  = Event::JoystickZeroFire;  // Cross
  myJoyTable[i + 3]  = Event::SaveState;         // Square
  myJoyTable[i + 4]  = Event::MenuMode;          // Left trigger
  myJoyTable[i + 5]  = Event::CmdMenuMode;       // Right trigger
  myJoyTable[i + 6]  = Event::JoystickZeroDown;  // Down
  myJoyTable[i + 7]  = Event::JoystickZeroLeft;  // Left
  myJoyTable[i + 8]  = Event::JoystickZeroUp;    // Up
  myJoyTable[i + 9]  = Event::JoystickZeroRight; // Right
  myJoyTable[i + 10] = Event::ConsoleSelect;     // Select
  myJoyTable[i + 11] = Event::ConsoleReset;      // Start
  myJoyTable[i + 12] = Event::NoType;            // Home
  myJoyTable[i + 13] = Event::NoType;            // Hold
#endif

  // Right joystick
  i = 1 * kNumJoyButtons;
  myJoyTable[i + kJAxisUp]    = Event::JoystickOneUp;
  myJoyTable[i + kJAxisDown]  = Event::JoystickOneDown;
  myJoyTable[i + kJAxisLeft]  = Event::JoystickOneLeft;
  myJoyTable[i + kJAxisRight] = Event::JoystickOneRight;
  myJoyTable[i + 0]     = Event::JoystickOneFire;

  saveJoyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveKeyMapping()
{
  // Iterate through the keymap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream keybuf;
  keybuf << Event::LastType << ":";
  for(uInt32 i = 0; i < SDLK_LAST; ++i)
    keybuf << myKeyTable[i] << ":";

  myOSystem->settings().setString("keymap", keybuf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveJoyMapping()
{
  // Iterate through the joymap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream joybuf;
  joybuf << Event::LastType << ":";
  for(Int32 i = 0; i < kNumJoysticks * kNumJoyButtons; ++i)
    joybuf << myJoyTable[i] << ":";

  myOSystem->settings().setString("joymap", joybuf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::isValidList(string& list, IntArray& map, uInt32 length)
{
  string key;
  Event::Type event;

  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  replace(list.begin(), list.end(), ':', ' ');
  istringstream buf(list);

  // Get event count, which should be the first int in the list
  buf >> key;
  event = (Event::Type) atoi(key.c_str());
  if(event == Event::LastType)
    while(buf >> key)
      map.push_back(atoi(key.c_str()));

  if(event == Event::LastType && map.size() == length)
    return true;
  else
    return false;
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
void EventHandler::saveState(int state)
{
  myLSState = state;
  saveState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::changeState()
{
  myLSState = (myLSState + 1) % 10;

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
void EventHandler::loadState(int state)
{
  myLSState = state;
  loadState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::takeSnapshot()
{
#ifdef PSP_DEBUG
  fprintf(stdout,"EventHandler::takeSnapshot\n");
#endif

#ifdef SNAPSHOT_SUPPORT
  // Figure out the correct snapshot name
  string filename;
  string sspath = myOSystem->settings().getString("ssdir");
  string ssname = myOSystem->settings().getString("ssname");

  if(sspath.length() > 0)
    if(sspath.substr(sspath.length()-1) != BSPF_PATH_SEPARATOR)
      sspath += BSPF_PATH_SEPARATOR;

  if(ssname == "romname")
    sspath += myOSystem->console().properties().get("Cartridge.Name");
  else if(ssname == "md5sum")
    sspath += myOSystem->console().properties().get("Cartridge.MD5");

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
#ifdef PSP_DEBUG
        fprintf(stdout,"EventHandler::takeSnapshot '%s'\n",buf.str().c_str());
#endif
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
  string result  = snapshot.savePNG(filename);
  myOSystem->frameBuffer().showMessage(result);
#else
  myOSystem->frameBuffer().showMessage("Snapshots unsupported");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setPaddleMode(uInt32 num, bool showmessage)
{
  if(num < 0 || num > 3)
    return;

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
void EventHandler::enterMenuMode(State state)
{
  setEventState(state);
  myOverlay->reStack();

  refreshDisplay();

  myOSystem->frameBuffer().setCursorState();
  myOSystem->sound().mute(true);
  myEvent->clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveMenuMode()
{
  setEventState(S_EMULATE);

  refreshDisplay();

  myOSystem->frameBuffer().setCursorState();
  myOSystem->sound().mute(false);
  myEvent->clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::enterDebugMode()
{
#ifdef DEVELOPER_SUPPORT
  if(myState == S_DEBUGGER)
    return false;

  setEventState(S_DEBUGGER);
  myOSystem->createFrameBuffer();
  myOverlay->reStack();
  myOSystem->frameBuffer().setCursorState();
  myEvent->clear();

  if(!myPauseFlag)  // Pause when entering debugger mode
    handleEvent(Event::Pause, 1);

  // Make sure debugger starts in a consistent state
  myOSystem->debugger().setStartState();

  // Make sure screen is always refreshed when entering debug mode
  // (sometimes entering on a breakpoint doesn't draw contents)
  refreshDisplay();
#else
  myOSystem->frameBuffer().showMessage("Developer/debugger unsupported");
#endif

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveDebugMode()
{
#ifdef DEVELOPER_SUPPORT
  // paranoia: this should never happen:
  if(myState != S_DEBUGGER)
    return;

  // Make sure debugger quits in a consistent state
  myOSystem->debugger().setQuitState();

  setEventState(S_EMULATE);
  myOSystem->createFrameBuffer();
  refreshDisplay();
  myOSystem->frameBuffer().setCursorState();
  myEvent->clear();

  if(myPauseFlag)  // Un-Pause when leaving debugger mode
    handleEvent(Event::Pause, 1);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setEventState(State state)
{
  myState = state;
  switch(myState)
  {
    case S_EMULATE:
      myOverlay = NULL;

      // Controller types only make sense in Emulate mode
      myController[0] = myOSystem->console().controller(Controller::Left).type();
      myController[1] = myOSystem->console().controller(Controller::Right).type();
      break;

    case S_MENU:
      myOverlay = &myOSystem->menu();
      break;

    case S_CMDMENU:
      myOverlay = &myOSystem->commandMenu();
      break;

    case S_LAUNCHER:
      myOverlay = &myOSystem->launcher();
      break;

#ifdef DEVELOPER_SUPPORT
    case S_DEBUGGER:
      myOverlay = &myOSystem->debugger();
      break;
#endif

    default:
      myOverlay = NULL;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveProperties()
{
  myOSystem->console().saveProperties(myOSystem->userProperties(), true);
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
  ourSDLMapping[ SDLK_EXCLAIM ]      = "!";
  ourSDLMapping[ SDLK_QUOTEDBL ]     = "\"";
  ourSDLMapping[ SDLK_HASH ]         = "#";
  ourSDLMapping[ SDLK_DOLLAR ]       = "$";
  ourSDLMapping[ SDLK_AMPERSAND ]    = "&";
  ourSDLMapping[ SDLK_QUOTE ]        = "\'";
  ourSDLMapping[ SDLK_LEFTPAREN ]    = "(";
  ourSDLMapping[ SDLK_RIGHTPAREN ]   = ")";
  ourSDLMapping[ SDLK_ASTERISK ]     = "*";
  ourSDLMapping[ SDLK_PLUS ]         = "+";
  ourSDLMapping[ SDLK_COMMA ]        = "COMMA";
  ourSDLMapping[ SDLK_MINUS ]        = "-";
  ourSDLMapping[ SDLK_PERIOD ]       = ".";
  ourSDLMapping[ SDLK_SLASH ]        = "/";
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
  ourSDLMapping[ SDLK_COLON ]        = ":";
  ourSDLMapping[ SDLK_SEMICOLON ]    = ";";
  ourSDLMapping[ SDLK_LESS ]         = "<";
  ourSDLMapping[ SDLK_EQUALS ]       = "=";
  ourSDLMapping[ SDLK_GREATER ]      = ">";
  ourSDLMapping[ SDLK_QUESTION ]     = "?";
  ourSDLMapping[ SDLK_AT ]           = "@";
  ourSDLMapping[ SDLK_LEFTBRACKET ]  = "[";
  ourSDLMapping[ SDLK_BACKSLASH ]    = "\\";
  ourSDLMapping[ SDLK_RIGHTBRACKET ] = "]";
  ourSDLMapping[ SDLK_CARET ]        = "^";
  ourSDLMapping[ SDLK_UNDERSCORE ]   = "_";
  ourSDLMapping[ SDLK_BACKQUOTE ]    = "`";
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
  ourSDLMapping[ SDLK_KP_PERIOD ]    = "KP .";
  ourSDLMapping[ SDLK_KP_DIVIDE ]    = "KP /";
  ourSDLMapping[ SDLK_KP_MULTIPLY ]  = "KP *";
  ourSDLMapping[ SDLK_KP_MINUS ]     = "KP -";
  ourSDLMapping[ SDLK_KP_PLUS ]      = "KP +";
  ourSDLMapping[ SDLK_KP_ENTER ]     = "KP ENTER";
  ourSDLMapping[ SDLK_KP_EQUALS ]    = "KP =";
  ourSDLMapping[ SDLK_UP ]           = "UP";
  ourSDLMapping[ SDLK_DOWN ]         = "DOWN";
  ourSDLMapping[ SDLK_RIGHT ]        = "RIGHT";
  ourSDLMapping[ SDLK_LEFT ]         = "LEFT";
  ourSDLMapping[ SDLK_INSERT ]       = "INS";
  ourSDLMapping[ SDLK_HOME ]         = "HOME";
  ourSDLMapping[ SDLK_END ]          = "END";
  ourSDLMapping[ SDLK_PAGEUP ]       = "PGUP";
  ourSDLMapping[ SDLK_PAGEDOWN ]     = "PGDN";
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
// FIXME - this must be handled better in the future ///
#ifndef _WIN32_WCE
ActionList EventHandler::ourActionList[kActionListSize] = {
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
  { Event::Fry,                         "Fry cartridge",                   "" },
  { Event::MenuMode,                    "Toggle options menu mode",        "" },
  { Event::CmdMenuMode,                 "Toggle command menu mode",        "" },
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
#else
ActionList EventHandler::ourActionList[kActionListSize];
#endif

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
    {Event::JoystickZeroUp,   Event::JoystickZeroDown,  Event::PaddleOneResistance}   },
  { {Event::JoystickOneLeft,  Event::JoystickOneRight,  Event::PaddleTwoResistance},
    {Event::JoystickOneUp,    Event::JoystickOneDown,   Event::PaddleThreeResistance} }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::Type EventHandler::SA_Button[2][2][3] = {
  { {Event::JoystickZeroFire, Event::PaddleZeroFire,  Event::DrivingZeroFire },
    {Event::NoType,           Event::PaddleOneFire,   Event::NoType}            },
  { {Event::JoystickOneFire,  Event::PaddleTwoFire,   Event::DrivingOneFire },
    {Event::NoType,           Event::PaddleThreeFire, Event::NoType}            }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Event::Type EventHandler::SA_DrivingValue[2] = {
  Event::DrivingZeroValue, Event::DrivingOneValue
};
