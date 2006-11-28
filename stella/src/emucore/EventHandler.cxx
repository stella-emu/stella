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
// $Id: EventHandler.cxx,v 1.174 2006-11-28 21:48:56 stephena Exp $
//============================================================================

#include <sstream>
#include <SDL.h>

#include "Event.hxx"
#include "EventHandler.hxx"
#include "EventStreamer.hxx"
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
#include "Deserializer.hxx"
#include "Serializer.hxx"
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(OSystem* osystem)
  : myOSystem(osystem),
    myEvent(NULL),
    myEventStreamer(NULL),
    myOverlay(NULL),
    myState(S_NONE),
    myLSState(0),
    myPauseFlag(false),
    myQuitFlag(false),
    myGrabMouseFlag(false),
    myUseLauncherFlag(false),
    myFryingFlag(false),
    myPaddleMode(0),
    myPaddleThreshold(0)
{
  int i, j, k, m;

  // Create the streamer used for accessing eventstreams/recordings
  myEventStreamer = new EventStreamer(myOSystem);

  // Create the event object which will be used for this handler
  myEvent = new Event(myEventStreamer);

  // Erase the key mapping array
  for(i = 0; i < SDLK_LAST; ++i)
  {
    ourSDLMapping[i] = "";
    for(m = 0; m < kNumModes; ++m)
      myKeyTable[i][m] = Event::NoType;
  }

#ifdef JOYSTICK_SUPPORT
  // Erase the joystick button mapping array
  for(m = 0; m < kNumModes; ++m)
    for(i = 0; i < kNumJoysticks; ++i)
      for(j = 0; j < kNumJoyButtons; ++j)
        myJoyTable[i][j][m] = Event::NoType;

  // Erase the joystick axis mapping array
  for(m = 0; m < kNumModes; ++m)
    for(i = 0; i < kNumJoysticks; ++i)
      for(j = 0; j < kNumJoyAxis; ++j)
        myJoyAxisTable[i][j][0][m] = myJoyAxisTable[i][j][1][m] = Event::NoType;

  // Erase the joystick hat mapping array
  for(m = 0; m < kNumModes; ++m)
    for(i = 0; i < kNumJoysticks; ++i)
      for(j = 0; j < kNumJoyHats; ++j)
        for(k = 0; k < 4; ++k)
          myJoyHatTable[i][j][k][m] = Event::NoType;
#endif

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::~EventHandler()
{
  delete myEvent;
  delete myEventStreamer;

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
void EventHandler::initialize()
{
  // Make sure the event/action mappings are correctly set,
  // and fill the ActionList structure with valid values
  setSDLMappings();
  setKeymap();
  setJoymap();
  setJoyAxisMap();
  setJoyHatMap();
  setActionMappings(kEmulationMode);
  setActionMappings(kMenuMode);

  myGrabMouseFlag = myOSystem->settings().getBool("grabmouse");
  setPaddleThreshold(myOSystem->settings().getInt("pthresh"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::reset(State state)
{
  setEventState(state);

  myLSState = 0;
  myPauseFlag = false;
  myQuitFlag = false;

  pause(false);
  myEvent->clear();

  if(myState == S_LAUNCHER)
    myUseLauncherFlag = true;

  // Start paddle emulation in a known state
  for(int i = 0; i < 4; ++i)
  {
    memset(&myPaddle[i], 0, sizeof(JoyMouse));
    myEvent->set(Paddle_Resistance[i], 1000000);
  }
  setPaddleSpeed(0, myOSystem->settings().getInt("p0speed"));
  setPaddleSpeed(1, myOSystem->settings().getInt("p1speed"));
  setPaddleSpeed(2, myOSystem->settings().getInt("p2speed"));
  setPaddleSpeed(3, myOSystem->settings().getInt("p3speed"));

//  myEventStreamer->reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::refreshDisplay(bool forceUpdate)
{
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
      return;
      break;
  }

  if(forceUpdate)
    myOSystem->frameBuffer().update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::pause(bool status)
{
  myPauseFlag = status;

  if(&myOSystem->frameBuffer())
    myOSystem->frameBuffer().pause(myPauseFlag);
  if(&myOSystem->sound())
    myOSystem->sound().mute(myPauseFlag);

  // Inform the OSystem of the change in pause
  myOSystem->pauseChanged(myPauseFlag);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setupJoysticks()
{
  bool showinfo = myOSystem->settings().getBool("showinfo");

#ifdef JOYSTICK_SUPPORT
  // Keep track of how many Stelladaptors we've found
  int saCount = 0;

  // First clear the joystick array, closing previously opened sticks
  for(int i = 0; i < kNumJoysticks; i++)
  {
    if(ourJoysticks[i].stick && SDL_JoystickOpened(i))
      SDL_JoystickClose(ourJoysticks[i].stick);

    ourJoysticks[i].stick = (SDL_Joystick*) NULL;
    ourJoysticks[i].type  = JT_NONE;
  }

  // (Re)-Initialize the joystick subsystem
  if(showinfo)
    cout << "Joystick devices found:" << endl;
  if((SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) || (SDL_NumJoysticks() <= 0))
  {
    if(showinfo)
      cout << "No joysticks present." << endl;
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
        cout << "  " << i << ": " << ourJoysticks[i].name << " with "
             << SDL_JoystickNumAxes(ourJoysticks[i].stick) << " axes, "
             << SDL_JoystickNumHats(ourJoysticks[i].stick) << " hats, "
             << SDL_JoystickNumBalls(ourJoysticks[i].stick) << " balls, "
             << SDL_JoystickNumButtons(ourJoysticks[i].stick) << " buttons"
             << endl;
    }
  }
  if(showinfo)
    cout << endl;

  // Map the stelladaptors we've found according to the specified ports
  const string& sa1 = myOSystem->settings().getString("sa1");
  const string& sa2 = myOSystem->settings().getString("sa2");
  mapStelladaptors(sa1, sa2);
#else
  if(showinfo)
    cout << "No joysticks present." << endl;
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
  // Check if we have an event from the eventstreamer
  // TODO - should we lock out input from the user while getting synthetic events?
//  int type, value;
//  while(myEventStreamer->pollEvent(type, value))
//    myEvent->set((Event::Type)type, value);

  // Synthesize events for platform-specific hardware
  myOSystem->pollEvent();

  // Check for an event
  SDL_Event event;
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
              myOSystem->frameBuffer().scale(+1);
              break;

            case SDLK_MINUS:
              myOSystem->frameBuffer().scale(-1);
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
                myOSystem->sound().adjustVolume(+1);
                break;

              case SDLK_END:       // Alt-End increases XStart
                myOSystem->console().changeXStart(+1);
                break;

              case SDLK_HOME:      // Alt-Home decreases XStart
                myOSystem->console().changeXStart(-1);
                break;

              case SDLK_PAGEUP:    // Alt-PageUp increases YStart
                myOSystem->console().changeYStart(+1);
                break;

              case SDLK_PAGEDOWN:  // Alt-PageDown decreases YStart
                myOSystem->console().changeYStart(-1);
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

              case SDLK_p:  // Alt-p toggles phosphor effect
                myOSystem->console().togglePhosphor();
                break;

#if 0
// FIXME - these will be removed when a UI is added for event recording
              case SDLK_e:  // Alt-e starts/stops event recording
                if(myEventStreamer->isRecording())
                {
                  if(myEventStreamer->stopRecording())
                    myOSystem->frameBuffer().showMessage("Recording stopped");
                  else
                    myOSystem->frameBuffer().showMessage("Stop recording error");
                }
                else
                {
                  if(myEventStreamer->startRecording())
                    myOSystem->frameBuffer().showMessage("Recording started");
                  else
                    myOSystem->frameBuffer().showMessage("Start recording error");
                }
                return;
                break;

              case SDLK_l:  // Alt-l loads a recording
                if(myEventStreamer->loadRecording())
                  myOSystem->frameBuffer().showMessage("Playing recording");
                else
                  myOSystem->frameBuffer().showMessage("Playing recording error");
                return;
                break;
////////////////////////////////////////////////////////////////////////
#endif
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
              myOSystem->frameBuffer().scale(+1);
              break;

            case SDLK_MINUS:
              myOSystem->frameBuffer().scale(-1);
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

              case SDLK_END:       // Ctrl-End increases Width
                myOSystem->console().changeWidth(+1);
                break;

              case SDLK_HOME:      // Ctrl-Home decreases Width
                myOSystem->console().changeWidth(-1);
                break;

              case SDLK_PAGEUP:    // Ctrl-PageUp increases Height
                myOSystem->console().changeHeight(+1);
                break;

              case SDLK_PAGEDOWN:  // Ctrl-PageDown decreases Height
                myOSystem->console().changeHeight(-1);
                break;

              case SDLK_s:         // Ctrl-s saves properties to a file
                string newPropertiesFile = myOSystem->baseDir() + BSPF_PATH_SEPARATOR +
                  myOSystem->console().properties().get(Cartridge_Name) + ".pro";
                myOSystem->console().saveProperties(newPropertiesFile);
                break;
            }
          }
        }

        // Handle keys which switch eventhandler state
        // Arrange the logic to take advantage of short-circuit evaluation
        if(!(kbdControl(mod) || kbdShift(mod) || kbdAlt(mod)) &&
            state && eventStateChange(myKeyTable[key][kEmulationMode]))
          return;

        // Otherwise, let the event handler deal with it
        if(myState == S_EMULATE)
          handleEvent(myKeyTable[key][kEmulationMode], state);
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
            if(event.jbutton.button >= kNumJoyButtons)
              return;

            int stick  = event.jbutton.which;
            int button = event.jbutton.button;
            int state  = event.jbutton.state == SDL_PRESSED ? 1 : 0;

            // Filter out buttons handled by OSystem
            if(!myOSystem->joyButtonHandled(button))
              handleJoyEvent(stick, button, state);

            break;  // Regular button
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
        if(event.jaxis.which >= kNumJoysticks)
          break;

        // Stelladaptors handle axis differently than regular joysticks
        int type = ourJoysticks[event.jbutton.which].type;
        switch(type)
        {
          case JT_REGULAR:
          {
            int stick = event.jaxis.which;
            int axis  = event.jaxis.axis;
            int value = event.jaxis.value;

            if(myState == S_EMULATE)
              handleJoyAxisEvent(stick, axis, value);
            else if(myOverlay != NULL)
              myOverlay->handleJoyAxisEvent(stick, axis, value);
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
                // Determine which paddle we're emulating and see if
                // we're getting rapid movement (aka jittering)
                if(isJitter(((type-2) << 1) + axis, value))
                  break;

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

      case SDL_JOYHATMOTION:
      {
        int stick = event.jhat.which;
        int hat   = event.jhat.hat;
        int value = event.jhat.value;

        if(stick >= kNumJoysticks || hat >= kNumJoyHats)
          break;

        // Preprocess all hat events, converting to Stella JoyHat type
        // Generate two equivalent hat events representing combined direction
        // when we get a diagonal hat event
        if(value == SDL_HAT_CENTERED)
          handleJoyHatEvent(stick, hat, kJHatCentered);
        else
        {
          if(value & SDL_HAT_UP)
            handleJoyHatEvent(stick, hat, kJHatUp);
          if(value & SDL_HAT_RIGHT)
            handleJoyHatEvent(stick, hat, kJHatRight); 
          if(value & SDL_HAT_DOWN)
            handleJoyHatEvent(stick, hat, kJHatDown);
          if(value & SDL_HAT_LEFT)
            handleJoyHatEvent(stick, hat, kJHatLeft);
        }
        break;  // SDL_JOYHATMOTION
      }
#endif  // JOYSTICK_SUPPORT
    }
  }

  // Handle paddle emulation using joystick or key events
  for(int i = 0; i < 4; ++i)
  {
    if(myPaddle[i].active)
    {
      myPaddle[i].x += myPaddle[i].x_amt;
      if(myPaddle[i].x < 0)
      {
        myPaddle[i].x = 0;  continue;
      }
      else if(myPaddle[i].x > 1000000)
      {
        myPaddle[i].x = 1000000;  continue;
      }
      else
      {
        int resistance = (int)(1000000.0 * (1000000.0 - myPaddle[i].x) / 1000000.0);
        myEvent->set(Paddle_Resistance[i], resistance);
      }
    }
  }

  // Update the current dialog container at regular intervals
  // Used to implement continuous events
  if(myState != S_EMULATE && myOverlay)
    myOverlay->updateTime(time);

#ifdef CHEATCODE_SUPPORT
  const CheatList& cheats = myOSystem->cheat().perFrame();
  for(unsigned int i = 0; i < cheats.size(); i++)
    cheats[i]->evaluate();
#endif

  // Tell the eventstreamer that another frame has finished
  // This is used for event recording
//  myEventStreamer->nextFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseMotionEvent(SDL_Event& event)
{
  // Take window zooming into account
  int x = event.motion.x, y = event.motion.y;
  myOSystem->frameBuffer().translateCoords(&x, &y);

  // Determine which mode we're in, then send the event to the appropriate place
  if(myState == S_EMULATE)
  {
    int w = myOSystem->frameBuffer().baseWidth();

    // Grabmouse introduces some lag into the mouse movement,
    // so we need to fudge the numbers a bit
    if(myGrabMouseFlag) x = MIN(w, (int) (x * 1.5));

    int resistance = (int)(1000000.0 * (w - x) / w);
    myEvent->set(Paddle_Resistance[myPaddleMode], resistance);
  }
  else if(myOverlay)
    myOverlay->handleMouseMotionEvent(x, y, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleMouseButtonEvent(SDL_Event& event, int state)
{
  // Determine which mode we're in, then send the event to the appropriate place
  if(myState == S_EMULATE)
    myEvent->set(Paddle_Button[myPaddleMode], state);
  else if(myOverlay)
  {
    // Take window zooming into account
    Int32 x = event.button.x, y = event.button.y;
//if (state) cerr << "B: x = " << x << ", y = " << y << endl;
    myOSystem->frameBuffer().translateCoords(&x, &y);
//if (state) cerr << "A: x = " << x << ", y = " << y << endl << endl;
    MouseButton button;

    switch(event.button.button)
    {
      case SDL_BUTTON_LEFT:
        button = state ? EVENT_LBUTTONDOWN : EVENT_LBUTTONUP;
        break;
      case SDL_BUTTON_RIGHT:
        button = state ? EVENT_RBUTTONDOWN : EVENT_RBUTTONUP;
        break;
      case SDL_BUTTON_WHEELDOWN:
        button = EVENT_WHEELDOWN;
        break;
      case SDL_BUTTON_WHEELUP:
        button = EVENT_WHEELUP;
        break;
      default:
        return;
    }
    myOverlay->handleMouseButtonEvent(button, x, y, state);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleJoyEvent(int stick, int button, int state)
{
#ifdef JOYSTICK_SUPPORT
  if(state && eventStateChange(myJoyTable[stick][button][kEmulationMode]))
    return;

  // Determine which mode we're in, then send the event to the appropriate place
  if(myState == S_EMULATE)
    handleEvent(myJoyTable[stick][button][kEmulationMode], state);
  else if(myOverlay != NULL)
    myOverlay->handleJoyEvent(stick, button, state);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleJoyAxisEvent(int stick, int axis, int value)
{
#ifdef JOYSTICK_SUPPORT
  // Every axis event has two associated values, negative and positive
  Event::Type eventAxisNeg = myJoyAxisTable[stick][axis][0][kEmulationMode];
  Event::Type eventAxisPos = myJoyAxisTable[stick][axis][1][kEmulationMode];

  // Check for analog events, which are handled differently
  switch((int)eventAxisNeg)
  {
    case Event::PaddleZeroAnalog:
      myEvent->set(Event::PaddleZeroResistance,
                    (int)(1000000.0 * (32767 - value) / 65534));
      break;
    case Event::PaddleOneAnalog:
      myEvent->set(Event::PaddleOneResistance,
                    (int)(1000000.0 * (32767 - value) / 65534));
      break;
    case Event::PaddleTwoAnalog:
      myEvent->set(Event::PaddleTwoResistance,
                    (int)(1000000.0 * (32767 - value) / 65534));
      break;
    case Event::PaddleThreeAnalog:
      myEvent->set(Event::PaddleThreeResistance,
                    (int)(1000000.0 * (32767 - value) / 65534));
      break;
    default:
      // Otherwise, we know the event is digital
      if(value > -JOY_DEADZONE && value < JOY_DEADZONE)
      {
        // Turn off both events, since we don't know exactly which one
        // was previously activated.
        handleEvent(eventAxisNeg, 0);
        handleEvent(eventAxisPos, 0);
      }
      else
        handleEvent(value < 0 ? eventAxisNeg : eventAxisPos, 1);
      break;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleJoyHatEvent(int stick, int hat, int value)
{
#ifdef JOYSTICK_SUPPORT
  if(myState == S_EMULATE)
  {
    if(value == kJHatCentered)
    {
      // Turn off all associated events, since we don't know exactly
      // which one was previously activated.
      handleEvent(myJoyHatTable[stick][hat][0][kEmulationMode], 0);
      handleEvent(myJoyHatTable[stick][hat][1][kEmulationMode], 0);
      handleEvent(myJoyHatTable[stick][hat][2][kEmulationMode], 0);
      handleEvent(myJoyHatTable[stick][hat][3][kEmulationMode], 0);
    }
    else
      handleEvent(myJoyHatTable[stick][hat][value][kEmulationMode], 1);
  }
  else if(myOverlay != NULL)
    myOverlay->handleJoyHatEvent(stick, hat, value);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleEvent(Event::Type event, int state)
{
  // Take care of special events that aren't part of the emulation core
  // or need to be preprocessed before passing them on
  switch((int)event)
  {
    ////////////////////////////////////////////////////////////////////////
    // Preprocess joystick events, converting them to events from whichever
    // controller is plugged into the corresponding virtual port.
    // This duplicates z26 behaviour, whereby the joystick can be used
    // for almost all types of input.
    // Yes, this is messy, but it's also as fast as possible ...
    case Event::JoystickZeroUp:
      if(state) myEvent->set(Event::JoystickZeroDown, 0);
      break;
    case Event::JoystickZeroDown:
      if(state) myEvent->set(Event::JoystickZeroUp, 0);
      break;
    case Event::JoystickZeroLeft:
      switch((int)myController[0])
      {
        case Controller::Joystick:
        case Controller::BoosterGrip:
          if(state) myEvent->set(Event::JoystickZeroRight, 0);
          myEvent->set(Event::JoystickZeroLeft, state);
          return;
        case Controller::Paddles:
          handleEvent(Event::PaddleZeroDecrease, state);
          return;
        case Controller::Driving:
          myEvent->set(Event::DrivingZeroCounterClockwise, state);
          return;
      }
      break;
    case Event::JoystickZeroRight:
      switch((int)myController[0])
      {
        case Controller::Joystick:
        case Controller::BoosterGrip:
          if(state) myEvent->set(Event::JoystickZeroLeft, 0);
          myEvent->set(Event::JoystickZeroRight, state);
          return;
        case Controller::Paddles:
          handleEvent(Event::PaddleZeroIncrease, state);
          return;
        case Controller::Driving:
          myEvent->set(Event::DrivingZeroClockwise, state);
          return;
      }
      break;
    case Event::JoystickZeroFire:
      switch((int)myController[0])
      {
        case Controller::Joystick:
          myEvent->set(Event::JoystickZeroFire, state);
          return;
        case Controller::Paddles:
          myEvent->set(Event::PaddleZeroFire, state);
          return;
        case Controller::Driving:
          myEvent->set(Event::DrivingZeroFire, state);
          return;
      }
      break;
    case Event::JoystickOneUp:
      if(state) myEvent->set(Event::JoystickOneDown, 0);
      break;
    case Event::JoystickOneDown:
      if(state) myEvent->set(Event::JoystickOneUp, 0);
      break;
    case Event::JoystickOneLeft:
      switch((int)myController[1])
      {
        case Controller::Joystick:
        case Controller::BoosterGrip:
          if(state) myEvent->set(Event::JoystickOneRight, 0);
          myEvent->set(Event::JoystickOneLeft, state);
          return;
        case Controller::Paddles:
          handleEvent(Event::PaddleOneDecrease, state);
          return;
        case Controller::Driving:
          myEvent->set(Event::DrivingOneCounterClockwise, state);
          return;
      }
      break;
    case Event::JoystickOneRight:
      switch((int)myController[1])
      {
        case Controller::Joystick:
        case Controller::BoosterGrip:
          if(state) myEvent->set(Event::JoystickOneLeft, 0);
          myEvent->set(Event::JoystickOneRight, state);
          return;
        case Controller::Paddles:
          handleEvent(Event::PaddleZeroIncrease, state);
          return;
        case Controller::Driving:
          myEvent->set(Event::DrivingOneClockwise, state);
          return;
      }
      break;
    case Event::JoystickOneFire:
      switch((int)myController[1])
      {
        case Controller::Joystick:
          myEvent->set(Event::JoystickOneFire, state);
          return;
        case Controller::Paddles:
          myEvent->set(Event::PaddleOneFire, state);
          return;
        case Controller::Driving:
          myEvent->set(Event::DrivingOneFire, state);
          return;
      }
      break;
    ////////////////////////////////////////////////////////////////////////

    case Event::PaddleZeroDecrease:
      myPaddle[0].active = (bool) state;
      myPaddle[0].x_amt  = -myPaddle[0].amt;
      return;
    case Event::PaddleZeroIncrease:
      myPaddle[0].active = (bool) state;
      myPaddle[0].x_amt  = myPaddle[0].amt;
      return;
    case Event::PaddleOneDecrease:
      myPaddle[1].active = (bool) state;
      myPaddle[1].x_amt  = -myPaddle[1].amt;
      return;
    case Event::PaddleOneIncrease:
      myPaddle[1].active = (bool) state;
      myPaddle[1].x_amt  = myPaddle[1].amt;
      return;
    case Event::PaddleTwoDecrease:
      myPaddle[2].active = (bool) state;
      myPaddle[2].x_amt  = -myPaddle[2].amt;
      return;
    case Event::PaddleTwoIncrease:
      myPaddle[2].active = (bool) state;
      myPaddle[2].x_amt  = myPaddle[2].amt;
      return;
    case Event::PaddleThreeDecrease:
      myPaddle[3].active = (bool) state;
      myPaddle[3].x_amt  = -myPaddle[3].amt;
      return;
    case Event::PaddleThreeIncrease:
      myPaddle[3].active = (bool) state;
      myPaddle[3].x_amt  = myPaddle[3].amt;
      return;

    case Event::NoType:  // Ignore unmapped events
      return;

    case Event::Fry:
      if(!myPauseFlag)
        myFryingFlag = bool(state);
      return;

    case Event::VolumeDecrease:
    case Event::VolumeIncrease:
      if(state && !myPauseFlag)
        myOSystem->sound().adjustVolume(event == Event::VolumeIncrease ? 1 : -1);
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
        pause(!myPauseFlag);
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
void EventHandler::createMouseMotionEvent(int x, int y)
{
  SDL_WarpMouse(x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::createMouseButtonEvent(int x, int y, int state)
{
  // Synthesize an left mouse button press/release event
  SDL_MouseButtonEvent mouseEvent;
  mouseEvent.type   = state ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
  mouseEvent.button = SDL_BUTTON_LEFT;
  mouseEvent.state  = state ? SDL_PRESSED : SDL_RELEASED;
  mouseEvent.x      = x;
  mouseEvent.y      = y;

  handleMouseButtonEvent((SDL_Event&)mouseEvent, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setActionMappings(EventMode mode)
{
  int listsize = 0;
  ActionList* list = NULL;

  switch(mode)
  {
    case kEmulationMode:
      listsize = kEmulActionListSize;
      list     = ourEmulActionList;
      break;
    case kMenuMode:
      listsize = kMenuActionListSize;
      list     = ourMenuActionList;
      break;
    default:
      return;
      break;
  }

  int i, j, stick, button, axis, hat, dir;
  ostringstream buf;

  // Fill the ActionList with the current key and joystick mappings
  for(i = 0; i < listsize; ++i)
  {
    Event::Type event = list[i].event;
    if(list[i].key)
      free(list[i].key);
    list[i].key = strdup("None");
    string key = "";
    for(j = 0; j < SDLK_LAST; ++j)   // key mapping
    {
      if(myKeyTable[j][mode] == event)
      {
        if(key == "")
          key = key + ourSDLMapping[j];
        else
          key = key + ", " + ourSDLMapping[j];
      }
    }
#ifdef JOYSTICK_SUPPORT
    // Joystick button mapping/labeling
    for(stick = 0; stick < kNumJoysticks; ++stick)
    {
      for(button = 0; button < kNumJoyButtons; ++button)
      {
        if(myJoyTable[stick][button][mode] == event)
        {
          buf.str("");
          buf << "J" << stick << " B" << button;
          if(key == "")
            key = key + buf.str();
          else
            key = key + ", " + buf.str();
        }
      }
    }
    // Joystick axis mapping/labeling
    for(stick = 0; stick < kNumJoysticks; ++stick)
    {
      for(axis = 0; axis < kNumJoyAxis; ++axis)
      {
        for(dir = 0; dir < 2; ++dir)
        {
          if(myJoyAxisTable[stick][axis][dir][mode] == event)
          {
            buf.str("");
            buf << "J" << stick << " axis " << axis;
            if(eventIsAnalog(event))
            {
              dir = 2;  // Immediately exit the inner loop after this iteration
              buf << " abs";
            }
            else if(dir == 0)
              buf << " neg";
            else
              buf << " pos";

            if(key == "")
              key = key + buf.str();
            else
              key = key + ", " + buf.str();
          }
        }
      }
    }
    // Joystick hat mapping/labeling
    for(stick = 0; stick < kNumJoysticks; ++stick)
    {
      for(hat = 0; hat < kNumJoyHats; ++hat)
      {
        for(dir = 0; dir < 4; ++dir)
        {
          if(myJoyHatTable[stick][hat][dir][mode] == event)
          {
            buf.str("");
            buf << "J" << stick << " hat " << hat;
            switch(dir)
            {
              case kJHatUp:    buf << " up"; break;
              case kJHatDown:  buf << " down"; break;
              case kJHatLeft:  buf << " left"; break;
              case kJHatRight: buf << " right"; break;
            }
            if(key == "")
              key = key + buf.str();
            else
              key = key + ", " + buf.str();
          }
        }
      }
    }
#endif

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
    {
      if(list[i].key)
        free(list[i].key);
      list[i].key = strdup(key.c_str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setKeymap()
{
  string list = myOSystem->settings().getString("keymap");
  IntArray map;

  if(isValidList(list, map, SDLK_LAST * kNumModes))
  {
    // Fill the keymap table with events
    IntArray::const_iterator event = map.begin();
    for(int mode = 0; mode < kNumModes; ++mode)
      for(int i = 0; i < SDLK_LAST; ++i)
        myKeyTable[i][mode] = (Event::Type) *event++;
  }
  else
  {
    setDefaultKeymap(kEmulationMode);
    setDefaultKeymap(kMenuMode);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setJoymap()
{
#ifdef JOYSTICK_SUPPORT
  string list = myOSystem->settings().getString("joymap");
  IntArray map;

  if(isValidList(list, map, kNumJoysticks*kNumJoyButtons*kNumModes))
  {
    // Fill the joymap table with events
    IntArray::const_iterator event = map.begin();
    for(int mode = 0; mode < kNumModes; ++mode)
      for(int i = 0; i < kNumJoysticks; ++i)
        for(int j = 0; j < kNumJoyButtons; ++j)
          myJoyTable[i][j][mode] = (Event::Type) *event++;
  }
  else
  {
    setDefaultJoymap(kEmulationMode);
    setDefaultJoymap(kMenuMode);
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setJoyAxisMap()
{
#ifdef JOYSTICK_SUPPORT
  string list = myOSystem->settings().getString("joyaxismap");
  IntArray map;

  if(isValidList(list, map, kNumJoysticks*kNumJoyAxis*2*kNumModes))
  {
    // Fill the joyaxismap table with events
    IntArray::const_iterator event = map.begin();
    for(int mode = 0; mode < kNumModes; ++mode)
      for(int i = 0; i < kNumJoysticks; ++i)
        for(int j = 0; j < kNumJoyAxis; ++j)
          for(int k = 0; k < 2; ++k)
            myJoyAxisTable[i][j][k][mode] = (Event::Type) *event++;
  }
  else
  {
    setDefaultJoyAxisMap(kEmulationMode);
    setDefaultJoyAxisMap(kMenuMode);
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setJoyHatMap()
{
#ifdef JOYSTICK_SUPPORT
  string list = myOSystem->settings().getString("joyhatmap");
  IntArray map;

  if(isValidList(list, map, kNumJoysticks*kNumJoyHats*4*kNumModes))
  {
    // Fill the joyhatmap table with events
    IntArray::const_iterator event = map.begin();
    for(int mode = 0; mode < kNumModes; ++mode)
      for(int i = 0; i < kNumJoysticks; ++i)
        for(int j = 0; j < kNumJoyHats; ++j)
          for(int k = 0; k < 4; ++k)
            myJoyHatTable[i][j][k][mode] = (Event::Type) *event++;
  }
  else
  {
    setDefaultJoyHatMap(kEmulationMode);
    setDefaultJoyHatMap(kMenuMode);
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addKeyMapping(Event::Type event, EventMode mode, int key)
{
  // These keys cannot be remapped.
  if(key == SDLK_TAB || eventIsAnalog(event))
    return false;
  else
  {
    myKeyTable[key][mode] = event;
    saveKeyMapping();

    setActionMappings(mode);
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoyMapping(Event::Type event, EventMode mode,
                                        int stick, int button)
{
  if(stick >= 0 && stick < kNumJoysticks &&
     button >= 0 && button < kNumJoyButtons &&
     event >= 0 && event < Event::LastType)
    myJoyTable[stick][button][mode] = event;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyMapping(Event::Type event, EventMode mode,
                                 int stick, int button)
{
  if(!eventIsAnalog(event))
  {
    setDefaultJoyMapping(event, mode, stick, button);

    saveJoyMapping();
    setActionMappings(mode);
    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoyAxisMapping(Event::Type event, EventMode mode,
                                            int stick, int axis, int value)
{
  if(stick >= 0 && stick < kNumJoysticks &&
     axis >= 0 && axis < kNumJoyAxis &&
     event >= 0 && event < Event::LastType)
  {
    // This confusing code is because each axis has two associated values,
    // but analog events only affect one of the axis.
    if(eventIsAnalog(event))
      myJoyAxisTable[stick][axis][0][mode] =
        myJoyAxisTable[stick][axis][1][mode] = event;
    else
    {
      // Otherwise, turn off the analog event(s) for this axis
      if(eventIsAnalog(myJoyAxisTable[stick][axis][0][mode]))
        myJoyAxisTable[stick][axis][0][mode] = Event::NoType;
      if(eventIsAnalog(myJoyAxisTable[stick][axis][1][mode]))
        myJoyAxisTable[stick][axis][1][mode] = Event::NoType;
    
      myJoyAxisTable[stick][axis][(value > 0)][mode] = event;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyAxisMapping(Event::Type event, EventMode mode,
                                     int stick, int axis, int value)
{
  setDefaultJoyAxisMapping(event, mode, stick, axis, value);

  saveJoyAxisMapping();
  setActionMappings(mode);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoyHatMapping(Event::Type event, EventMode mode,
                                           int stick, int hat, int value)
{
  if(stick >= 0 && stick < kNumJoysticks &&
     hat >= 0 && hat < kNumJoyHats &&
     event >= 0 && event < Event::LastType)
  {
    switch(value)
    {
      case kJHatUp:
      case kJHatDown:
      case kJHatLeft:
      case kJHatRight:
        myJoyHatTable[stick][hat][value][mode] = event;
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyHatMapping(Event::Type event, EventMode mode,
                                    int stick, int hat, int value)
{
  if(!eventIsAnalog(event))
  {
    setDefaultJoyHatMapping(event, mode, stick, hat, value);

    saveJoyHatMapping();
    setActionMappings(mode);
    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::eraseMapping(Event::Type event, EventMode mode)
{
  int i, j, k;

  // Erase the KeyEvent arrays
  for(i = 0; i < SDLK_LAST; ++i)
    if(myKeyTable[i][mode] == event && i != SDLK_TAB)
      myKeyTable[i][mode] = Event::NoType;
  saveKeyMapping();

#ifdef JOYSTICK_SUPPORT
  // Erase the JoyEvent array
  for(i = 0; i < kNumJoysticks; ++i)
    for(j = 0; j < kNumJoyButtons; ++j)
      if(myJoyTable[i][j][mode] == event)
        myJoyTable[i][j][mode] = Event::NoType;
  saveJoyMapping();

  // Erase the JoyAxisEvent array
  for(i = 0; i < kNumJoysticks; ++i)
    for(j = 0; j < kNumJoyAxis; ++j)
      for(k = 0; k < 2; ++k)
        if(myJoyAxisTable[i][j][k][mode] == event)
          myJoyAxisTable[i][j][k][mode] = Event::NoType;
  saveJoyAxisMapping();

  // Erase the JoyHatEvent array
  for(i = 0; i < kNumJoysticks; ++i)
    for(j = 0; j < kNumJoyHats; ++j)
      for(k = 0; k < 4; ++k)
        if(myJoyHatTable[i][j][k][mode] == event)
          myJoyHatTable[i][j][k][mode] = Event::NoType;
  saveJoyHatMapping();
#endif

  setActionMappings(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultMapping(EventMode mode)
{
  setDefaultKeymap(mode);
  setDefaultJoymap(mode);
  setDefaultJoyAxisMap(mode);
  setDefaultJoyHatMap(mode);

  setActionMappings(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultKeymap(EventMode mode)
{
  switch(mode)
  {
    case kEmulationMode:
      // Erase all mappings
      for(int i = 0; i < SDLK_LAST; ++i)
        myKeyTable[i][mode] = Event::NoType;

      myKeyTable[ SDLK_1 ][mode]         = Event::KeyboardZero1;
      myKeyTable[ SDLK_2 ][mode]         = Event::KeyboardZero2;
      myKeyTable[ SDLK_3 ][mode]         = Event::KeyboardZero3;
      myKeyTable[ SDLK_q ][mode]         = Event::KeyboardZero4;
      myKeyTable[ SDLK_w ][mode]         = Event::KeyboardZero5;
      myKeyTable[ SDLK_e ][mode]         = Event::KeyboardZero6;
      myKeyTable[ SDLK_a ][mode]         = Event::KeyboardZero7;
      myKeyTable[ SDLK_s ][mode]         = Event::KeyboardZero8;
      myKeyTable[ SDLK_d ][mode]         = Event::KeyboardZero9;
      myKeyTable[ SDLK_z ][mode]         = Event::KeyboardZeroStar;
      myKeyTable[ SDLK_x ][mode]         = Event::KeyboardZero0;
      myKeyTable[ SDLK_c ][mode]         = Event::KeyboardZeroPound;

      myKeyTable[ SDLK_8 ][mode]         = Event::KeyboardOne1;
      myKeyTable[ SDLK_9 ][mode]         = Event::KeyboardOne2;
      myKeyTable[ SDLK_0 ][mode]         = Event::KeyboardOne3;
      myKeyTable[ SDLK_i ][mode]         = Event::KeyboardOne4;
      myKeyTable[ SDLK_o ][mode]         = Event::KeyboardOne5;
      myKeyTable[ SDLK_p ][mode]         = Event::KeyboardOne6;
      myKeyTable[ SDLK_k ][mode]         = Event::KeyboardOne7;
      myKeyTable[ SDLK_l ][mode]         = Event::KeyboardOne8;
      myKeyTable[ SDLK_SEMICOLON ][mode] = Event::KeyboardOne9;
      myKeyTable[ SDLK_COMMA ][mode]     = Event::KeyboardOneStar;
      myKeyTable[ SDLK_PERIOD ][mode]    = Event::KeyboardOne0;
      myKeyTable[ SDLK_SLASH ][mode]     = Event::KeyboardOnePound;

      myKeyTable[ SDLK_UP ][mode]        = Event::JoystickZeroUp;
      myKeyTable[ SDLK_DOWN ][mode]      = Event::JoystickZeroDown;
      myKeyTable[ SDLK_LEFT ][mode]      = Event::JoystickZeroLeft;
      myKeyTable[ SDLK_RIGHT ][mode]     = Event::JoystickZeroRight;
      myKeyTable[ SDLK_SPACE ][mode]     = Event::JoystickZeroFire;
      myKeyTable[ SDLK_LCTRL ][mode]     = Event::JoystickZeroFire;
      myKeyTable[ SDLK_4 ][mode]         = Event::BoosterGripZeroTrigger;
      myKeyTable[ SDLK_5 ][mode]         = Event::BoosterGripZeroBooster;

      myKeyTable[ SDLK_y ][mode]         = Event::JoystickOneUp;
      myKeyTable[ SDLK_h ][mode]         = Event::JoystickOneDown;
      myKeyTable[ SDLK_g ][mode]         = Event::JoystickOneLeft;
      myKeyTable[ SDLK_j ][mode]         = Event::JoystickOneRight;
      myKeyTable[ SDLK_f ][mode]         = Event::JoystickOneFire;
      myKeyTable[ SDLK_6 ][mode]         = Event::BoosterGripOneTrigger;
      myKeyTable[ SDLK_7 ][mode]         = Event::BoosterGripOneBooster;

      myKeyTable[ SDLK_INSERT ][mode]    = Event::DrivingZeroCounterClockwise;
      myKeyTable[ SDLK_PAGEUP ][mode]    = Event::DrivingZeroClockwise;
      myKeyTable[ SDLK_HOME ][mode]      = Event::DrivingZeroFire;

      myKeyTable[ SDLK_DELETE ][mode]    = Event::DrivingOneCounterClockwise;
      myKeyTable[ SDLK_PAGEDOWN ][mode]  = Event::DrivingOneClockwise;
      myKeyTable[ SDLK_END ][mode]       = Event::DrivingOneFire;

      myKeyTable[ SDLK_F1 ][mode]        = Event::ConsoleSelect;
      myKeyTable[ SDLK_F2 ][mode]        = Event::ConsoleReset;
      myKeyTable[ SDLK_F3 ][mode]        = Event::ConsoleColor;
      myKeyTable[ SDLK_F4 ][mode]        = Event::ConsoleBlackWhite;
      myKeyTable[ SDLK_F5 ][mode]        = Event::ConsoleLeftDifficultyA;
      myKeyTable[ SDLK_F6 ][mode]        = Event::ConsoleLeftDifficultyB;
      myKeyTable[ SDLK_F7 ][mode]        = Event::ConsoleRightDifficultyA;
      myKeyTable[ SDLK_F8 ][mode]        = Event::ConsoleRightDifficultyB;
      myKeyTable[ SDLK_F9 ][mode]        = Event::SaveState;
      myKeyTable[ SDLK_F10 ][mode]       = Event::ChangeState;
      myKeyTable[ SDLK_F11 ][mode]       = Event::LoadState;
      myKeyTable[ SDLK_F12 ][mode]       = Event::TakeSnapshot;
      myKeyTable[ SDLK_PAUSE ][mode]     = Event::Pause;
      myKeyTable[ SDLK_BACKSPACE ][mode] = Event::Fry;
      myKeyTable[ SDLK_TAB ][mode]       = Event::MenuMode;
      myKeyTable[ SDLK_BACKSLASH ][mode] = Event::CmdMenuMode;
      myKeyTable[ SDLK_BACKQUOTE ][mode] = Event::DebuggerMode;
      myKeyTable[ SDLK_ESCAPE ][mode]    = Event::LauncherMode;
      break;

    case kMenuMode:
      myKeyTable[ SDLK_UP ][mode]        = Event::UIUp;
      myKeyTable[ SDLK_DOWN ][mode]      = Event::UIDown;
      myKeyTable[ SDLK_LEFT ][mode]      = Event::UILeft;
      myKeyTable[ SDLK_RIGHT ][mode]     = Event::UIRight;

      myKeyTable[ SDLK_HOME ][mode]      = Event::UIHome;
      myKeyTable[ SDLK_END ][mode]       = Event::UIEnd;
      myKeyTable[ SDLK_PAGEUP ][mode]    = Event::UIPgUp;
      myKeyTable[ SDLK_PAGEDOWN ][mode]  = Event::UIPgDown;

      myKeyTable[ SDLK_RETURN ][mode]    = Event::UISelect;
      myKeyTable[ SDLK_ESCAPE ][mode]    = Event::UICancel;
      break;

    default:
      return;
      break;
  }

  saveKeyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoymap(EventMode mode)
{
  // Erase all mappings
  for(int i = 0; i < kNumJoysticks; ++i)
    for(int j = 0; j < kNumJoyButtons; ++j)
      myJoyTable[i][j][mode] = Event::NoType;

  myOSystem->setDefaultJoymap();
  saveJoyMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoyAxisMap(EventMode mode)
{
  // Erase all mappings
  for(int i = 0; i < kNumJoysticks; ++i)
    for(int j = 0; j < kNumJoyAxis; ++j)
      for(int k = 0; k < 2; ++k)
        myJoyAxisTable[i][j][k][mode] = Event::NoType;

  myOSystem->setDefaultJoyAxisMap();
  saveJoyAxisMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoyHatMap(EventMode mode)
{
  // Erase all mappings
  for(int i = 0; i < kNumJoysticks; ++i)
    for(int j = 0; j < kNumJoyHats; ++j)
      for(int k = 0; k < 4; ++k)
        myJoyHatTable[i][j][k][mode] = Event::NoType;

  myOSystem->setDefaultJoyHatMap();
  saveJoyHatMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveKeyMapping()
{
  // Iterate through the keymap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream keybuf;
  keybuf << Event::LastType << ":";
  for(int mode = 0; mode < kNumModes; ++mode)
    for(int i = 0; i < SDLK_LAST; ++i)
      keybuf << myKeyTable[i][mode] << ":";

  myOSystem->settings().setString("keymap", keybuf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveJoyMapping()
{
  // Iterate through the joymap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream buf;
  buf << Event::LastType << ":";
  for(int mode = 0; mode < kNumModes; ++mode)
    for(int i = 0; i < kNumJoysticks; ++i)
      for(int j = 0; j < kNumJoyButtons; ++j)
        buf << myJoyTable[i][j][mode] << ":";

  myOSystem->settings().setString("joymap", buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveJoyAxisMapping()
{
  // Iterate through the joyaxismap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream buf;
  buf << Event::LastType << ":";
  for(int mode = 0; mode < kNumModes; ++mode)
    for(int i = 0; i < kNumJoysticks; ++i)
      for(int j = 0; j < kNumJoyAxis; ++j)
        for(int k = 0; k < 2; ++k)
          buf << myJoyAxisTable[i][j][k][mode] << ":";

  myOSystem->settings().setString("joyaxismap", buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveJoyHatMapping()
{
  // Iterate through the joyhatmap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream buf;
  buf << Event::LastType << ":";
  for(int mode = 0; mode < kNumModes; ++mode)
    for(int i = 0; i < kNumJoysticks; ++i)
      for(int j = 0; j < kNumJoyHats; ++j)
        for(int k = 0; k < 4; ++k)
          buf << myJoyHatTable[i][j][k][mode] << ":";

  myOSystem->settings().setString("joyhatmap", buf.str());
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

  return (event == Event::LastType && map.size() == length);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool EventHandler::eventIsAnalog(Event::Type event)
{
  switch((int)event)
  {
    case Event::PaddleZeroAnalog:
    case Event::PaddleOneAnalog:
    case Event::PaddleTwoAnalog:
    case Event::PaddleThreeAnalog:
      return true;
    default:
      return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StringList EventHandler::getActionList(EventMode mode)
{
  StringList l;

  switch(mode)
  {
    case kEmulationMode:
      for(int i = 0; i < kEmulActionListSize; ++i)
        l.push_back(EventHandler::ourEmulActionList[i].action);
      break;
    case kMenuMode:
      for(int i = 0; i < kMenuActionListSize; ++i)
        l.push_back(EventHandler::ourMenuActionList[i].action);
      break;
    default:
      break;
  }

  return l;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type EventHandler::eventAtIndex(int idx, EventMode mode)
{
  switch(mode)
  {
    case kEmulationMode:
      if(idx < 0 || idx >= kEmulActionListSize)
        return Event::NoType;
      else
        return ourEmulActionList[idx].event;
      break;
    case kMenuMode:
      if(idx < 0 || idx >= kMenuActionListSize)
        return Event::NoType;
      else
        return ourMenuActionList[idx].event;
      break;
    default:
      return Event::NoType;
      break;
  }
}  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::actionAtIndex(int idx, EventMode mode)
{
  switch(mode)
  {
    case kEmulationMode:
      if(idx < 0 || idx >= kEmulActionListSize)
        return EmptyString;
      else
        return ourEmulActionList[idx].action;
      break;
    case kMenuMode:
      if(idx < 0 || idx >= kMenuActionListSize)
        return EmptyString;
      else
        return ourMenuActionList[idx].action;
      break;
    default:
      return EmptyString;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::keyAtIndex(int idx, EventMode mode)
{
  switch(mode)
  {
    case kEmulationMode:
      if(idx < 0 || idx >= kEmulActionListSize)
        return EmptyString;
      else
        return ourEmulActionList[idx].key;
      break;
    case kMenuMode:
      if(idx < 0 || idx >= kMenuActionListSize)
        return EmptyString;
      else
        return ourMenuActionList[idx].key;
      break;
    default:
      return EmptyString;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool EventHandler::isJitter(int paddle, int value)
{
  bool jitter = false;
  bool leftMotion = myPaddle[paddle].joy_val - myPaddle[paddle].old_joy_val > 0;
  int distance = value - myPaddle[paddle].joy_val;

  // Filter out jitter by not allowing rapid direction changes
  if(distance > 0 && !leftMotion)     // movement switched from left to right
    jitter = distance < myPaddleThreshold;
  else if(distance < 0 && leftMotion) // movement switched from right to left
    jitter = distance > -myPaddleThreshold;

  myPaddle[paddle].old_joy_val = myPaddle[paddle].joy_val;
  myPaddle[paddle].joy_val = value;

  return jitter;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveState()
{
  const string& name = myOSystem->console().properties().get(Cartridge_Name);
  const string& md5  = myOSystem->console().properties().get(Cartridge_MD5);

  ostringstream buf;
  buf << myOSystem->stateDir() << BSPF_PATH_SEPARATOR
      << name << ".st" << myLSState;

  // Make sure the file can be opened for writing
  Serializer out;
  if(!out.open(buf.str()))
  {
    myOSystem->frameBuffer().showMessage("Error saving state file");
    return;
  }

  // Do a state save using the System
  buf.str("");
  if(myOSystem->console().system().saveState(md5, out))
  {
    buf << "State " << myLSState << " saved";
    if(myOSystem->settings().getBool("autoslot"))
    {
      changeState(false);  // don't show a message for state change
      buf << ", switching to slot " << myLSState;
    }
  }
  else
    buf << "Error saving state " << myLSState;

  out.close();
  myOSystem->frameBuffer().showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveState(int state)
{
  myLSState = state;
  saveState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::changeState(bool show)
{
  myLSState = (myLSState + 1) % 10;

  // Print appropriate message
  if(show)
  {
    ostringstream buf;
    buf << "Changed to slot " << myLSState;
    myOSystem->frameBuffer().showMessage(buf.str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::loadState()
{
  const string& name = myOSystem->console().properties().get(Cartridge_Name);
  const string& md5  = myOSystem->console().properties().get(Cartridge_MD5);

  ostringstream buf;
  buf << myOSystem->stateDir() << BSPF_PATH_SEPARATOR
      << name << ".st" << myLSState;

  // Make sure the file can be opened for reading
  Deserializer in;
  if(!in.open(buf.str()))
  {
    buf.str("");
    buf << "Error loading state " << myLSState;
    myOSystem->frameBuffer().showMessage(buf.str());
    return;
  }

  // Do a state load using the System
  buf.str("");
  if(myOSystem->console().system().loadState(md5, in))
    buf << "State " << myLSState << " loaded";
  else
    buf << "Invalid state " << myLSState << " file";

  in.close();
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
#ifdef SNAPSHOT_SUPPORT
  // Figure out the correct snapshot name
  string filename;
  string sspath = myOSystem->settings().getString("ssdir");

  if(sspath.length() > 0)
    if(sspath.substr(sspath.length()-1) != BSPF_PATH_SEPARATOR)
      sspath += BSPF_PATH_SEPARATOR;
  sspath += myOSystem->console().properties().get(Cartridge_Name);

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

  // Now create a PNG snapshot
  Snapshot::savePNG(myOSystem->frameBuffer(),
                    myOSystem->console().properties(), filename);
#else
  myOSystem->frameBuffer().showMessage("Snapshots unsupported");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setPaddleMode(int num, bool showmessage)
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setPaddleSpeed(int num, int speed)
{
  if(num < 0 || num > 3 || speed < 0 || speed > 100)
    return;

  myPaddle[num].amt = (int) (20000 + speed/100.0 * 50000);
  ostringstream buf;
  buf << "p" << num+1 << "speed";
  myOSystem->settings().setInt(buf.str(), speed);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setPaddleThreshold(int thresh)
{
  myPaddleThreshold = thresh;
  myOSystem->settings().setInt("pthresh", thresh);
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
  myOSystem->console().setPalette(myOSystem->settings().getString("palette"));
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

  // Inform the OSystem about the new state
  myOSystem->stateChanged(myState);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveProperties()
{
  myOSystem->console().saveProperties(myOSystem->propertiesFile(), true);
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
ActionList EventHandler::ourEmulActionList[kEmulActionListSize] = {
  { Event::ConsoleSelect,               "Select",                          0 },
  { Event::ConsoleReset,                "Reset",                           0 },
  { Event::ConsoleColor,                "Color TV",                        0 },
  { Event::ConsoleBlackWhite,           "Black & White TV",                0 },
  { Event::ConsoleLeftDifficultyA,      "P1 Difficulty A",                 0 },
  { Event::ConsoleLeftDifficultyB,      "P1 Difficulty B",                 0 },
  { Event::ConsoleRightDifficultyA,     "P2 Difficulty A",                 0 },
  { Event::ConsoleRightDifficultyB,     "P2 Difficulty B",                 0 },
  { Event::SaveState,                   "Save State",                      0 },
  { Event::ChangeState,                 "Change State",                    0 },
  { Event::LoadState,                   "Load State",                      0 },
  { Event::TakeSnapshot,                "Snapshot",                        0 },
  { Event::Pause,                       "Pause",                           0 },
  { Event::Fry,                         "Fry cartridge",                   0 },
  { Event::VolumeDecrease,              "Decrease volume",                 0 },
  { Event::VolumeIncrease,              "Increase volume",                 0 },
  { Event::MenuMode,                    "Toggle options menu mode",        0 },
  { Event::CmdMenuMode,                 "Toggle command menu mode",        0 },
  { Event::DebuggerMode,                "Toggle debugger mode",            0 },
  { Event::LauncherMode,                "Enter ROM launcher",              0 },
  { Event::Quit,                        "Quit",                            0 },

  { Event::JoystickZeroUp,              "P1 Joystick Up",                  0 },
  { Event::JoystickZeroDown,            "P1 Joystick Down",                0 },
  { Event::JoystickZeroLeft,            "P1 Joystick Left",                0 },
  { Event::JoystickZeroRight,           "P1 Joystick Right",               0 },
  { Event::JoystickZeroFire,            "P1 Joystick Fire",                0 },

  { Event::JoystickOneUp,               "P2 Joystick Up",                  0 },
  { Event::JoystickOneDown,             "P2 Joystick Down",                0 },
  { Event::JoystickOneLeft,             "P2 Joystick Left",                0 },
  { Event::JoystickOneRight,            "P2 Joystick Right",               0 },
  { Event::JoystickOneFire,             "P2 Joystick Fire",                0 },

  { Event::PaddleZeroAnalog,            "Paddle 1 Analog",                 0 },
  { Event::PaddleZeroDecrease,          "Paddle 1 Decrease",               0 },
  { Event::PaddleZeroIncrease,          "Paddle 1 Increase",               0 },
  { Event::PaddleZeroFire,              "Paddle 1 Fire",                   0 },

  { Event::PaddleOneAnalog,             "Paddle 2 Analog",                 0 },
  { Event::PaddleOneDecrease,           "Paddle 2 Decrease",               0 },
  { Event::PaddleOneIncrease,           "Paddle 2 Increase",               0 },
  { Event::PaddleOneFire,               "Paddle 2 Fire",                   0 },

  { Event::PaddleTwoAnalog,             "Paddle 3 Analog",                 0 },
  { Event::PaddleTwoDecrease,           "Paddle 3 Decrease",               0 },
  { Event::PaddleTwoIncrease,           "Paddle 3 Increase",               0 },
  { Event::PaddleTwoFire,               "Paddle 3 Fire",                   0 },

  { Event::PaddleThreeAnalog,           "Paddle 4 Analog",                 0 },
  { Event::PaddleThreeDecrease,         "Paddle 4 Decrease",               0 },
  { Event::PaddleThreeIncrease,         "Paddle 4 Increase",               0 },
  { Event::PaddleThreeFire,             "Paddle 4 Fire",                   0 },

  { Event::BoosterGripZeroTrigger,      "P1 Booster-Grip Trigger",         0 },
  { Event::BoosterGripZeroBooster,      "P1 Booster-Grip Booster",         0 },

  { Event::BoosterGripOneTrigger,       "P2 Booster-Grip Trigger",         0 },
  { Event::BoosterGripOneBooster,       "P2 Booster-Grip Booster",         0 },

  { Event::DrivingZeroCounterClockwise, "P1 Driving Controller Left",      0 },
  { Event::DrivingZeroClockwise,        "P1 Driving Controller Right",     0 },
  { Event::DrivingZeroFire,             "P1 Driving Controller Fire",      0 },

  { Event::DrivingOneCounterClockwise,  "P2 Driving Controller Left",      0 },
  { Event::DrivingOneClockwise,         "P2 Driving Controller Right",     0 },
  { Event::DrivingOneFire,              "P2 Driving Controller Fire",      0 },

  { Event::KeyboardZero1,               "P1 Keyboard 1",                   0 },
  { Event::KeyboardZero2,               "P1 Keyboard 2",                   0 },
  { Event::KeyboardZero3,               "P1 Keyboard 3",                   0 },
  { Event::KeyboardZero4,               "P1 Keyboard 4",                   0 },
  { Event::KeyboardZero5,               "P1 Keyboard 5",                   0 },
  { Event::KeyboardZero6,               "P1 Keyboard 6",                   0 },
  { Event::KeyboardZero7,               "P1 Keyboard 7",                   0 },
  { Event::KeyboardZero8,               "P1 Keyboard 8",                   0 },
  { Event::KeyboardZero9,               "P1 Keyboard 9",                   0 },
  { Event::KeyboardZeroStar,            "P1 Keyboard *",                   0 },
  { Event::KeyboardZero0,               "P1 Keyboard 0",                   0 },
  { Event::KeyboardZeroPound,           "P1 Keyboard #",                   0 },

  { Event::KeyboardOne1,                "P2 Keyboard 1",                   0 },
  { Event::KeyboardOne2,                "P2 Keyboard 2",                   0 },
  { Event::KeyboardOne3,                "P2 Keyboard 3",                   0 },
  { Event::KeyboardOne4,                "P2 Keyboard 4",                   0 },
  { Event::KeyboardOne5,                "P2 Keyboard 5",                   0 },
  { Event::KeyboardOne6,                "P2 Keyboard 6",                   0 },
  { Event::KeyboardOne7,                "P2 Keyboard 7",                   0 },
  { Event::KeyboardOne8,                "P2 Keyboard 8",                   0 },
  { Event::KeyboardOne9,                "P2 Keyboard 9",                   0 },
  { Event::KeyboardOneStar,             "P2 Keyboard *",                   0 },
  { Event::KeyboardOne0,                "P2 Keyboard 0",                   0 },
  { Event::KeyboardOnePound,            "P2 Keyboard #",                   0 }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionList EventHandler::ourMenuActionList[kMenuActionListSize] = {
  { Event::UIUp,        "Move Up",              0 },
  { Event::UIDown,      "Move Down",            0 },
  { Event::UILeft,      "Move Left",            0 },
  { Event::UIRight,     "Move Right",           0 },

  { Event::UIHome,      "Home",                 0 },
  { Event::UIEnd,       "End",                  0 },
  { Event::UIPgUp,      "Page Up",              0 },
  { Event::UIPgDown,    "Page Down",            0 },

  { Event::UIOK,        "OK",                   0 },
  { Event::UICancel,    "Cancel",               0 },
  { Event::UISelect,    "Select item",          0 },

  { Event::UINavPrev,   "Previous object",      0 },
  { Event::UINavNext,   "Next object",          0 },
  { Event::UITabPrev,   "Previous tabgroup",    0 },
  { Event::UITabNext,   "Next tabgroup",        0 }
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
