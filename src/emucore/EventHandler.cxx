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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <sstream>
#include <set>
#include <map>
#include <SDL.h>

#include "bspf.hxx"

#include "CommandMenu.hxx"
#include "Console.hxx"
#include "DialogContainer.hxx"
#include "Event.hxx"
#include "FrameBuffer.hxx"
#include "FSNode.hxx"
#include "Launcher.hxx"
#include "Menu.hxx"
#include "OSystem.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "PropsSet.hxx"
#include "ListWidget.hxx"
#include "ScrollBarWidget.hxx"
#include "Settings.hxx"
#include "Snapshot.hxx"
#include "Sound.hxx"
#include "StateManager.hxx"
#include "Switches.hxx"
#include "MouseControl.hxx"

#include "EventHandler.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "Cheat.hxx"
  #include "CheatManager.hxx"
#endif
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(OSystem* osystem)
  : myOSystem(osystem),
    myOverlay(NULL),
    myMouseControl(NULL),
    myState(S_NONE),
    myAllowAllDirectionsFlag(false),
    myFryingFlag(false),
    mySkipMouseMotion(true),
    myJoysticks(NULL),
    myNumJoysticks(0)
{
  // Erase the key mapping array
  for(int i = 0; i < KBDK_LAST; ++i)
  {
    ourKBDKMapping[i] = "";
    for(int m = 0; m < kNumModes; ++m)
      myKeyTable[i][m] = Event::NoType;
  }

  // Erase the 'combo' array
  for(int i = 0; i < kComboSize; ++i)
    for(int j = 0; j < kEventsPerCombo; ++j)
      myComboTable[i][j] = Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::~EventHandler()
{
  // Free strings created with strdup
  for(uInt32 i = 0; i < kEmulActionListSize; ++i)
    if(ourEmulActionList[i].key)
      free(ourEmulActionList[i].key);
  for(uInt32 i = 0; i < kMenuActionListSize; ++i)
    if(ourMenuActionList[i].key)
      free(ourMenuActionList[i].key);

  delete myMouseControl;
  delete[] myJoysticks;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::initialize()
{
  // Make sure the event/action mappings are correctly set,
  // and fill the ActionList structure with valid values
  setSDLMappings();
  setKeymap();
  setComboMap();
  setActionMappings(kEmulationMode);
  setActionMappings(kMenuMode);

  myUseCtrlKeyFlag = myOSystem->settings().getBool("ctrlcombo");

  Joystick::setDeadZone(myOSystem->settings().getInt("joydeadzone"));
  Paddles::setDigitalSensitivity(myOSystem->settings().getInt("dsense"));
  Paddles::setMouseSensitivity(myOSystem->settings().getInt("msense"));

  // Set quick select delay when typing characters in listwidgets
  ListWidget::setQuickSelectDelay(myOSystem->settings().getInt("listdelay"));

  // Set number of lines a mousewheel will scroll
  ScrollBarWidget::setWheelLines(myOSystem->settings().getInt("mwheel"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::reset(State state)
{
  setEventState(state);
  myOSystem->state().reset();

  setContinuousSnapshots(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setupJoysticks()
{
#ifdef JOYSTICK_SUPPORT
  // Initialize the joystick subsystem
  if((SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) || (SDL_NumJoysticks() <= 0))
  {
    myOSystem->logMessage("No joysticks present.\n\n", 1);
    return;
  }

  // We need unique names for mappable devices
  set<string> joyNames;

  // Keep track of how many Stelladaptors we've found
  int saCount = 0;

  // Open all SDL joysticks (only the first 2 Stelladaptor devices are used)
  if((myNumJoysticks = SDL_NumJoysticks()) > 0)
    myJoysticks = new StellaJoystick[myNumJoysticks];
  for(uInt32 i = 0; i < myNumJoysticks; ++i)
  {
    string name = myJoysticks[i].setStick(i);

    // Skip if we couldn't open it for any reason
    if(name == "None")
      continue;

    // Figure out what type of joystick this is
    if(name.find("2600-daptor", 0) != string::npos)
    {
      saCount++;
      if(saCount > 2)  // Ignore more than 2 2600-daptors
      {
        myJoysticks[i].type = StellaJoystick::JT_NONE;
        continue;
      }

      // 2600-daptorII devices have 3 axes and 12 buttons, and the value of the z-axis
      // determines how those 12 buttons are used (not all buttons are used in all modes)
      if(myJoysticks[i].numAxes == 3)
      {
        // TODO - stubbed out for now, until we find a way to reliably get info
        //        from the Z axis
        myJoysticks[i].name = "2600-daptor II";
      }
      else
        myJoysticks[i].name = "2600-daptor";
    }
    else if(name.find("Stelladaptor", 0) != string::npos)
    {
      saCount++;
      if(saCount > 2)  // Ignore more than 2 Stelladaptors
        continue;
      else             // Type will be set by mapStelladaptors()
        myJoysticks[i].name = "Stelladaptor";
    }
    else
    {
      // Names must be unique
      ostringstream namebuf;
      pair<set<string>::iterator,bool> ret;
      string actualName = name;
      int j = 2;
      do {
        ret = joyNames.insert(actualName);
        namebuf.str("");
        namebuf << name << " " << j++;
        actualName = namebuf.str();
      }
      while(ret.second == false);
      name = *ret.first;

      myJoysticks[i].name = name;
      myJoysticks[i].type = StellaJoystick::JT_REGULAR;
    }
  }

  // Map the stelladaptors we've found according to the specified ports
  mapStelladaptors(myOSystem->settings().getString("saport"));

  setJoymap();
  setActionMappings(kEmulationMode);
  setActionMappings(kMenuMode);

  ostringstream buf;
  buf << "Joystick devices found:" << endl;
  for(uInt32 i = 0; i < myNumJoysticks; ++i)
    buf << "  " << i << ": " << myJoysticks[i].about() << endl << endl;
  myOSystem->logMessage(buf.str(), 1);
#else
  myOSystem->logMessage("No joysticks present.\n\n", 1);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::mapStelladaptors(const string& saport)
{
#ifdef JOYSTICK_SUPPORT
  // saport will have two values:
  //   'lr' means treat first valid adaptor as left port, second as right port
  //   'rl' means treat first valid adaptor as right port, second as left port
  // We know there will be only two such devices (at most), since the logic
  // in setupJoysticks take care of that
  int saCount = 0;
  int saOrder[2];
  if(BSPF_equalsIgnoreCase(saport, "lr"))
  {
    saOrder[0] = 1; saOrder[1] = 2;
  }
  else
  {
    saOrder[0] = 2; saOrder[1] = 1;
  }

  for(uInt32 i = 0; i < myNumJoysticks; i++)
  {
    if(BSPF_startsWithIgnoreCase(myJoysticks[i].name, "Stelladaptor"))
    {
      saCount++;
      if(saOrder[saCount-1] == 1)
      {
        myJoysticks[i].name += " (emulates left joystick port)";
        myJoysticks[i].type = StellaJoystick::JT_STELLADAPTOR_LEFT;
      }
      else if(saOrder[saCount-1] == 2)
      {
        myJoysticks[i].name += " (emulates right joystick port)";
        myJoysticks[i].type = StellaJoystick::JT_STELLADAPTOR_RIGHT;
      }
    }
    else if(BSPF_startsWithIgnoreCase(myJoysticks[i].name, "2600-daptor"))
    {
      saCount++;
      if(saOrder[saCount-1] == 1)
      {
        myJoysticks[i].name += " (emulates left joystick port)";
        myJoysticks[i].type = StellaJoystick::JT_2600DAPTOR_LEFT;
      }
      else if(saOrder[saCount-1] == 2)
      {
        myJoysticks[i].name += " (emulates right joystick port)";
        myJoysticks[i].type = StellaJoystick::JT_2600DAPTOR_RIGHT;
      }
    }
  }
  myOSystem->settings().setString("saport", saport);

  // We're potentially swapping out an input device behind the back of
  // the Event system, so we make sure all Stelladaptor-generated events
  // are reset
  for(int i = 0; i < 2; ++i)
  {
    for(int j = 0; j < 2; ++j)
      myEvent.set(SA_Axis[i][j], 0);
    for(int j = 0; j < 4; ++j)
      myEvent.set(SA_Button[i][j], 0);
    for(int j = 0; j < 12; ++j)
      myEvent.set(SA_Key[i][j], 0);
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::toggleSAPortOrder()
{
  const string& saport = myOSystem->settings().getString("saport");
  if(saport == "lr")
  {
    mapStelladaptors("rl");
    myOSystem->frameBuffer().showMessage("Stelladaptor ports right/left");
  }
  else
  {
    mapStelladaptors("lr");
    myOSystem->frameBuffer().showMessage("Stelladaptor ports left/right");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::poll(uInt64 time)
{
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
        StellaKey key  = (StellaKey)event.key.keysym.sym;
        StellaMod mod  = event.key.keysym.mod;
        bool state = event.key.type == SDL_KEYDOWN;
        bool handled = true;

        // An attempt to speed up event processing
        // All SDL-specific event actions are accessed by either
        // Control or Alt/Cmd keys.  So we quickly check for those.
        if(kbdAlt(mod) && state)
        {
      #ifdef MAC_OSX
          // These keys work in all states
          if(key == KBDK_q)
          {
            handleEvent(Event::Quit, 1);
          }
          else
      #endif
          if(key == KBDK_RETURN)
          {
            myOSystem->frameBuffer().toggleFullscreen();
          }
          // These only work when in emulation mode
          else if(myState == S_EMULATE)
          {
            switch(key)
            {
              case KBDK_EQUALS:
                myOSystem->frameBuffer().changeVidMode(+1);
                break;

              case KBDK_MINUS:
                myOSystem->frameBuffer().changeVidMode(-1);
                break;

              case KBDK_LEFTBRACKET:
                myOSystem->sound().adjustVolume(-1);
                break;

              case KBDK_RIGHTBRACKET:
                myOSystem->sound().adjustVolume(+1);
                break;

              case KBDK_PAGEUP:    // Alt-PageUp increases YStart
                myOSystem->console().changeYStart(+1);
                break;

              case KBDK_PAGEDOWN:  // Alt-PageDown decreases YStart
                myOSystem->console().changeYStart(-1);
                break;

              case KBDK_f:  // Alt-f toggles NTSC filtering
                myOSystem->console().toggleNTSC();
                break;

              case KBDK_z:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().toggleP0Collision();
                else
                  myOSystem->console().toggleP0Bit();
                break;

              case KBDK_x:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().toggleP1Collision();
                else
                  myOSystem->console().toggleP1Bit();
                break;

              case KBDK_c:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().toggleM0Collision();
                else
                  myOSystem->console().toggleM0Bit();
                break;

              case KBDK_v:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().toggleM1Collision();
                else
                  myOSystem->console().toggleM1Bit();
                break;

              case KBDK_b:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().toggleBLCollision();
                else
                  myOSystem->console().toggleBLBit();
                break;

              case KBDK_n:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().togglePFCollision();
                else
                  myOSystem->console().togglePFBit();
                break;

              case KBDK_m:
                myOSystem->console().toggleHMOVE();
                break;

              case KBDK_COMMA:
                myOSystem->console().toggleFixedColors();
                break;

              case KBDK_PERIOD:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().enableCollisions(false);
                else
                  myOSystem->console().enableBits(false);
                break;

              case KBDK_SLASH:
                if(mod & KMOD_SHIFT)
                  myOSystem->console().enableCollisions(true);
                else
                  myOSystem->console().enableBits(true);
                break;

              case KBDK_p:  // Alt-p toggles phosphor effect
                myOSystem->console().togglePhosphor();
                break;

              case KBDK_l:
                myOSystem->frameBuffer().toggleFrameStats();
                break;

              case KBDK_s:  // TODO - make this remappable
                if(myContSnapshotInterval == 0)
                {
                  ostringstream buf;
                  uInt32 interval = myOSystem->settings().getInt("ssinterval");
                  buf << "Enabling shotshots in " << interval << " second intervals";
                  myOSystem->frameBuffer().showMessage(buf.str());
                  setContinuousSnapshots(interval);
                }
                else
                {
                  ostringstream buf;
                  buf << "Disabling snapshots, generated "
                      << (myContSnapshotCounter / myContSnapshotInterval)
                      << " files";
                  myOSystem->frameBuffer().showMessage(buf.str());
                  setContinuousSnapshots(0);
                }
                break;
#if 0
// these will be removed when a UI is added for event recording
              case KBDK_e:  // Alt-e starts/stops event recording
                if(myOSystem->state().toggleRecordMode())
                  myOSystem->frameBuffer().showMessage("Recording started");
                else
                  myOSystem->frameBuffer().showMessage("Recording stopped");
                break;

              case KBDK_r:  // Alt-r starts/stops rewind mode
                if(myOSystem->state().toggleRewindMode())
                  myOSystem->frameBuffer().showMessage("Rewind mode started");
                else
                  myOSystem->frameBuffer().showMessage("Rewind mode stopped");
                break;
/*
              case KBDK_l:  // Alt-l loads a recording
                if(myEventStreamer->loadRecording())
                  myOSystem->frameBuffer().showMessage("Playing recording");
                else
                  myOSystem->frameBuffer().showMessage("Playing recording error");
                return;
                break;
*/
////////////////////////////////////////////////////////////////////////
#endif
              default:
                handled = false;
                break;
            }
          }
          else
            handled = false;
        }
        else if(kbdControl(mod) && state && myUseCtrlKeyFlag)
        {
          // These keys work in all states
          if(key == KBDK_q)
          {
            handleEvent(Event::Quit, 1);
          }
          // These only work when in emulation mode
          else if(myState == S_EMULATE)
          {
            switch(key)
            {
              case KBDK_0:  // Ctrl-0 switches between mouse control modes
                if(myMouseControl)
                {
                  const string& message = myMouseControl->next();
                  myOSystem->frameBuffer().showMessage(message);
                }
                break;

              case KBDK_1:  // Ctrl-1 swaps Stelladaptor/2600-daptor ports
                toggleSAPortOrder();
                break;

              case KBDK_f:  // Ctrl-f toggles NTSC/PAL mode
                myOSystem->console().toggleFormat();
                break;

              case KBDK_g:  // Ctrl-g (un)grabs mouse
                if(!myOSystem->frameBuffer().fullScreen())
                  myOSystem->frameBuffer().toggleGrabMouse();
                break;

              case KBDK_l:  // Ctrl-l toggles PAL color-loss effect
                myOSystem->console().toggleColorLoss();
                break;

              case KBDK_p:  // Ctrl-p toggles different palettes
                myOSystem->console().togglePalette();
                break;

              case KBDK_r:  // Ctrl-r reloads the currently loaded ROM
                myOSystem->deleteConsole();
                myOSystem->createConsole();
                break;

              case KBDK_PAGEUP:    // Ctrl-PageUp increases Height
                myOSystem->console().changeHeight(+1);
                break;

              case KBDK_PAGEDOWN:  // Ctrl-PageDown decreases Height
                myOSystem->console().changeHeight(-1);
                break;

              case KBDK_s:         // Ctrl-s saves properties to a file
              {
                string filename = myOSystem->baseDir() +
                    myOSystem->console().properties().get(Cartridge_Name) + ".pro";
                ofstream out(filename.c_str(), ios::out);
                if(out)
                {
                  myOSystem->console().properties().save(out);
                  out.close();
                  myOSystem->frameBuffer().showMessage("Properties saved");
                }
                else
                  myOSystem->frameBuffer().showMessage("Error saving properties");
                break;
              }

              default:
                handled = false;
                break;
            }
          }
          else
            handled = false;
        }
        else
          handled = false;

        // Don't pass the key on if we've already taken care of it
        if(handled) break;

        // Handle keys which switch eventhandler state
        // Arrange the logic to take advantage of short-circuit evaluation
        if(!(kbdControl(mod) || kbdShift(mod) || kbdAlt(mod)) &&
            state && eventStateChange(myKeyTable[key][kEmulationMode]))
          return;

        // Otherwise, let the event handler deal with it
        if(myState == S_EMULATE)
          handleEvent(myKeyTable[key][kEmulationMode], state);
        else if(myOverlay != NULL)
          myOverlay->handleKeyEvent(key, mod, event.key.keysym.unicode & 0x7f, state);

        break;  // SDL_KEYUP, SDL_KEYDOWN
      }

      case SDL_MOUSEMOTION:
        // Determine which mode we're in, then send the event to the appropriate place
        if(myState == S_EMULATE)
        {
          if(!mySkipMouseMotion)
          {
            myEvent.set(Event::MouseAxisXValue, event.motion.xrel);
            myEvent.set(Event::MouseAxisYValue, event.motion.yrel);
          }
          mySkipMouseMotion = false;
        }
        else if(myOverlay)
          myOverlay->handleMouseMotionEvent(event.motion.x, event.motion.y, 0);

        break;  // SDL_MOUSEMOTION

      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEBUTTONDOWN:
      {
        uInt8 state = event.button.type == SDL_MOUSEBUTTONDOWN ? 1 : 0;

        // Determine which mode we're in, then send the event to the appropriate place
        if(myState == S_EMULATE)
        {
          switch(event.button.button)
          {
            case SDL_BUTTON_LEFT:
              myEvent.set(Event::MouseButtonLeftValue, state);
              break;
            case SDL_BUTTON_RIGHT:
              myEvent.set(Event::MouseButtonRightValue, state);
              break;
            default:
              break;
          }
        }
        else if(myOverlay)
        {
          Int32 x = event.button.x, y = event.button.y;

          switch(event.button.button)
          {
            case SDL_BUTTON_LEFT:
              myOverlay->handleMouseButtonEvent(
                  state ? EVENT_LBUTTONDOWN : EVENT_LBUTTONUP, x, y, state);
              break;
            case SDL_BUTTON_RIGHT:
              myOverlay->handleMouseButtonEvent(
                  state ? EVENT_RBUTTONDOWN : EVENT_RBUTTONUP, x, y, state);
              break;
            case SDL_BUTTON_WHEELDOWN:
              if(state)
                myOverlay->handleMouseButtonEvent(EVENT_WHEELDOWN, x, y, 1);
              break;
            case SDL_BUTTON_WHEELUP:
              if(state)
                myOverlay->handleMouseButtonEvent(EVENT_WHEELUP, x, y, 1);
              break;
            default:
              break;
          }
        }
        break;  // SDL_MOUSEBUTTONUP, SDL_MOUSEBUTTONDOWN
      }

      case SDL_ACTIVEEVENT:
        if((event.active.state & SDL_APPACTIVE) && (event.active.gain == 0))
          if(myState == S_EMULATE) enterMenuMode(S_MENU);
        break; // SDL_ACTIVEEVENT

      case SDL_QUIT:
        handleEvent(Event::Quit, 1);
        break;  // SDL_QUIT

      case SDL_VIDEOEXPOSE:
        myOSystem->frameBuffer().refresh();
        break;  // SDL_VIDEOEXPOSE

#ifdef JOYSTICK_SUPPORT
      case SDL_JOYBUTTONUP:
      case SDL_JOYBUTTONDOWN:
      {
        if(event.jbutton.which >= myNumJoysticks)
          break;

        // Stelladaptors handle buttons differently than regular joysticks
        const StellaJoystick& joy = myJoysticks[event.jbutton.which];
        int button = event.jbutton.button;
        int state  = event.jbutton.state == SDL_PRESSED ? 1 : 0;

        switch(joy.type)
        {
          case StellaJoystick::JT_REGULAR:
            // Handle buttons which switch eventhandler state
            if(state && eventStateChange(joy.btnTable[button][kEmulationMode]))
              return;

            // Determine which mode we're in, then send the event to the appropriate place
            if(myState == S_EMULATE)
              handleEvent(joy.btnTable[button][kEmulationMode], state);
            else if(myOverlay != NULL)
              myOverlay->handleJoyEvent(event.jbutton.which, button, state);
            break;  // Regular button

          // These events don't have to pass through handleEvent, since
          // they can never be remapped
          case StellaJoystick::JT_STELLADAPTOR_LEFT:
          case StellaJoystick::JT_STELLADAPTOR_RIGHT:
            // The 'type-2' here refers to the fact that 'StellaJoystick::JT_STELLADAPTOR_LEFT'
            // and 'StellaJoystick::JT_STELLADAPTOR_RIGHT' are at index 2 and 3 in the JoyType
            // enum; subtracting two gives us Controller 0 and 1
            if(button < 2) myEvent.set(SA_Button[joy.type-2][button], state);
            break;  // Stelladaptor button
          case StellaJoystick::JT_2600DAPTOR_LEFT:
          case StellaJoystick::JT_2600DAPTOR_RIGHT:
            // The 'type-4' here refers to the fact that 'StellaJoystick::JT_2600DAPTOR_LEFT'
            // and 'StellaJoystick::JT_2600DAPTOR_RIGHT' are at index 4 and 5 in the JoyType
            // enum; subtracting four gives us Controller 0 and 1
            if(myState == S_EMULATE)
            {
              switch(myOSystem->console().controller(Controller::Left).type())
              {
                case Controller::Keyboard:
                  if(button < 12) myEvent.set(SA_Key[joy.type-4][button], state);
                  break;
                default:
                  if(button < 4) myEvent.set(SA_Button[joy.type-4][button], state);
              }
              switch(myOSystem->console().controller(Controller::Right).type())
              {
                case Controller::Keyboard:
                  if(button < 12) myEvent.set(SA_Key[joy.type-4][button], state);
                  break;
                default:
                  if(button < 4) myEvent.set(SA_Button[joy.type-4][button], state);
              }
            }
            break;  // 2600DAPTOR button
          default:
            break;
        }
        break;  // SDL_JOYBUTTONUP, SDL_JOYBUTTONDOWN
      }  

      case SDL_JOYAXISMOTION:
      {
        if(event.jaxis.which >= myNumJoysticks)
          break;

        // Stelladaptors handle axis differently than regular joysticks
        const StellaJoystick& joy = myJoysticks[event.jaxis.which];
        int type = myJoysticks[event.jaxis.which].type;
        int axis  = event.jaxis.axis;
        int value = event.jaxis.value;

        switch(type)
        {
          case StellaJoystick::JT_REGULAR:
            if(myState == S_EMULATE)
            {
              // Every axis event has two associated values, negative and positive
              Event::Type eventAxisNeg = joy.axisTable[axis][0][kEmulationMode];
              Event::Type eventAxisPos = joy.axisTable[axis][1][kEmulationMode];

              // Check for analog events, which are handled differently
              // We'll pass them off as Stelladaptor events, and let the controllers
              // handle it
              switch((int)eventAxisNeg)
              {
                case Event::PaddleZeroAnalog:
                  myEvent.set(Event::SALeftAxis0Value, value);
                  break;
                case Event::PaddleOneAnalog:
                  myEvent.set(Event::SALeftAxis1Value, value);
                  break;
                case Event::PaddleTwoAnalog:
                  myEvent.set(Event::SARightAxis0Value, value);
                  break;
                case Event::PaddleThreeAnalog:
                  myEvent.set(Event::SARightAxis1Value, value);
                  break;
                default:
                {
                  // Otherwise, we know the event is digital
                  if(value > Joystick::deadzone())
                    handleEvent(eventAxisPos, 1);
                  else if(value < -Joystick::deadzone())
                    handleEvent(eventAxisNeg, 1);
                  else
                  {
                    // Treat any deadzone value as zero
                    value = 0;

                    // Now filter out consecutive, similar values
                    // (only pass on the event if the state has changed)
                    if(joy.axisLastValue[axis] != value)
                    {
                      // Turn off both events, since we don't know exactly which one
                      // was previously activated.
                      handleEvent(eventAxisNeg, 0);
                      handleEvent(eventAxisPos, 0);
                    }
                  }
                  joy.axisLastValue[axis] = value;
                  break;
                }
              }
            }
            else if(myOverlay != NULL)
            {
              // First, clamp the values to simulate digital input
              // (the only thing that the underlying code understands)
              if(value > Joystick::deadzone())
                value = 32000;
              else if(value < -Joystick::deadzone())
                value = -32000;
              else
                value = 0;

              // Now filter out consecutive, similar values
              // (only pass on the event if the state has changed)
              if(value != joy.axisLastValue[axis])
              {
                myOverlay->handleJoyAxisEvent(event.jaxis.which, axis, value);
                joy.axisLastValue[axis] = value;
              }
            }
            break;  // Regular joystick axis

          // Since the various controller classes deal with Stelladaptor
          // devices differently, we send the raw X and Y axis data directly,
          // and let the controller handle it
          // These events don't have to pass through handleEvent, since
          // they can never be remapped
          case StellaJoystick::JT_STELLADAPTOR_LEFT:
          case StellaJoystick::JT_STELLADAPTOR_RIGHT:
            // The 'type-2' here refers to the fact that 'StellaJoystick::JT_STELLADAPTOR_LEFT'
            // and 'StellaJoystick::JT_STELLADAPTOR_RIGHT' are at index 2 and 3 in the JoyType
            // enum; subtracting two gives us Controller 0 and 1
            if(axis < 2)
              myEvent.set(SA_Axis[type-2][axis], value);
            break;  // Stelladaptor axis
          case StellaJoystick::JT_2600DAPTOR_LEFT:
          case StellaJoystick::JT_2600DAPTOR_RIGHT:
            // The 'type-4' here refers to the fact that 'StellaJoystick::JT_2600DAPTOR_LEFT'
            // and 'StellaJoystick::JT_2600DAPTOR_RIGHT' are at index 4 and 5 in the JoyType
            // enum; subtracting four gives us Controller 0 and 1
            if(axis < 2)
              myEvent.set(SA_Axis[type-4][axis], value);
            break;  // 26000daptor axis
        }
        break;  // SDL_JOYAXISMOTION
      }

      case SDL_JOYHATMOTION:
      {
        if(event.jhat.which >= myNumJoysticks)
          break;

        const StellaJoystick& joy = myJoysticks[event.jhat.which];
        int stick = event.jhat.which;
        int hat   = event.jhat.hat;
        int value = event.jhat.value;

        // Preprocess all hat events, converting to Stella JoyHat type
        // Generate multiple equivalent hat events representing combined direction
        // when we get a diagonal hat event
        if(myState == S_EMULATE)
        {
          handleEvent(joy.hatTable[hat][EVENT_HATUP][kEmulationMode],
                      value & SDL_HAT_UP);
          handleEvent(joy.hatTable[hat][EVENT_HATRIGHT][kEmulationMode],
                      value & SDL_HAT_RIGHT);
          handleEvent(joy.hatTable[hat][EVENT_HATDOWN][kEmulationMode],
                      value & SDL_HAT_DOWN);
          handleEvent(joy.hatTable[hat][EVENT_HATLEFT][kEmulationMode],
                      value & SDL_HAT_LEFT);
        }
        else if(myOverlay != NULL)
        {
          if(value == SDL_HAT_CENTERED)
            myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATCENTER);
          else
          {
            if(value & SDL_HAT_UP)
              myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATUP);
            if(value & SDL_HAT_RIGHT)
              myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATRIGHT); 
            if(value & SDL_HAT_DOWN)
              myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATDOWN);
            if(value & SDL_HAT_LEFT)
              myOverlay->handleJoyHatEvent(stick, hat, EVENT_HATLEFT);
          }
        }
        break;  // SDL_JOYHATMOTION
      }
#endif  // JOYSTICK_SUPPORT
    }
  }

  // Update controllers and console switches, and in general all other things
  // related to emulation
  if(myState == S_EMULATE)
  {
    myOSystem->console().controller(Controller::Left).update();
    myOSystem->console().controller(Controller::Right).update();
    myOSystem->console().switches().update();

#if 0
    // Now check if the StateManager should be saving or loading state
    // Per-frame cheats are disabled if the StateManager is active, since
    // it would interfere with proper playback
    if(myOSystem->state().isActive())
    {
      myOSystem->state().update();
    }
    else
#endif
    {
    #ifdef CHEATCODE_SUPPORT
      const CheatList& cheats = myOSystem->cheat().perFrame();
      for(uInt32 i = 0; i < cheats.size(); i++)
        cheats[i]->evaluate();
    #endif

      // Handle continuous snapshots
      if(myContSnapshotInterval > 0 &&
        (++myContSnapshotCounter % myContSnapshotInterval == 0))
        takeSnapshot(time >> 10);  // not quite milliseconds, but close enough
    }
  }
  else if(myOverlay)
  {
    // Update the current dialog container at regular intervals
    // Used to implement continuous events
    myOverlay->updateTime(time);
  }

  // Turn off all mouse-related items; if they haven't been taken care of
  // in the previous ::update() methods, they're now invalid
  myEvent.set(Event::MouseAxisXValue, 0);
  myEvent.set(Event::MouseAxisYValue, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::handleEvent(Event::Type event, int state)
{
  // Take care of special events that aren't part of the emulation core
  // or need to be preprocessed before passing them on
  switch(event)
  {
    ////////////////////////////////////////////////////////////////////////
    // If enabled, make sure 'impossible' joystick directions aren't allowed
    case Event::JoystickZeroUp:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickZeroDown, 0);
      break;

    case Event::JoystickZeroDown:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickZeroUp, 0);
      break;

    case Event::JoystickZeroLeft:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickZeroRight, 0);
      break;

    case Event::JoystickZeroRight:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickZeroLeft, 0);
      break;

    case Event::JoystickOneUp:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickOneDown, 0);
      break;

    case Event::JoystickOneDown:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickOneUp, 0);
      break;

    case Event::JoystickOneLeft:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickOneRight, 0);
      break;

    case Event::JoystickOneRight:
      if(!myAllowAllDirectionsFlag && state)
        myEvent.set(Event::JoystickOneLeft, 0);
      break;
    ////////////////////////////////////////////////////////////////////////

    case Event::Fry:
      if(myUseCtrlKeyFlag) myFryingFlag = bool(state);
      return;

    case Event::VolumeDecrease:
      if(state) myOSystem->sound().adjustVolume(-1);
      return;

    case Event::VolumeIncrease:
      if(state) myOSystem->sound().adjustVolume(+1);
      return;

    case Event::SaveState:
      if(state) myOSystem->state().saveState();
      return;

    case Event::ChangeState:
      if(state) myOSystem->state().changeState();
      return;

    case Event::LoadState:
      if(state) myOSystem->state().loadState();
      return;

    case Event::TakeSnapshot:
      if(state) takeSnapshot();
      return;

    case Event::LauncherMode:
      if((myState == S_EMULATE || myState == S_CMDMENU ||
          myState == S_DEBUGGER) && state)
      {
        myOSystem->settings().saveConfig();

        // Go back to the launcher, or immediately quit
        if(myOSystem->settings().getBool("uselauncher"))
        {
          myOSystem->deleteConsole();
          myOSystem->createLauncher();
        }
        else
          myOSystem->quit();
      }
      return;

    case Event::Quit:
      if(state)
      {
        saveKeyMapping();
        saveJoyMapping();
        myOSystem->settings().saveConfig();
        myOSystem->quit();
      }
      return;

    ////////////////////////////////////////////////////////////////////////
    // A combo event is simply multiple calls to handleEvent, once for
    // each event it contains
    case Event::Combo1:
    case Event::Combo2:
    case Event::Combo3:
    case Event::Combo4:
    case Event::Combo5:
    case Event::Combo6:
    case Event::Combo7:
    case Event::Combo8:
    case Event::Combo9:
    case Event::Combo10:
    case Event::Combo11:
    case Event::Combo12:
    case Event::Combo13:
    case Event::Combo14:
    case Event::Combo15:
    case Event::Combo16:
      for(int i = 0, combo = event - Event::Combo1; i < kEventsPerCombo; ++i)
        if(myComboTable[combo][i] != Event::NoType)
          handleEvent(myComboTable[combo][i], state);
      return;
    ////////////////////////////////////////////////////////////////////////

    // Events which generate messages
    case Event::ConsoleColor:
      if(state) myOSystem->frameBuffer().showMessage("Color Mode");
      break;
    case Event::ConsoleBlackWhite:
      if(state) myOSystem->frameBuffer().showMessage("BW Mode");
      break;
    case Event::ConsoleLeftDiffA:
      if(state) myOSystem->frameBuffer().showMessage("Left Difficulty A");
      break;
    case Event::ConsoleLeftDiffB:
      if(state) myOSystem->frameBuffer().showMessage("Left Difficulty B");
      break;
    case Event::ConsoleRightDiffA:
      if(state) myOSystem->frameBuffer().showMessage("Right Difficulty A");
      break;
    case Event::ConsoleRightDiffB:
      if(state) myOSystem->frameBuffer().showMessage("Right Difficulty B");
      break;

    case Event::NoType:  // Ignore unmapped events
      return;

    default:
      break;
  }

  // Otherwise, pass it to the emulation core
  myEvent.set(event, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::eventStateChange(Event::Type type)
{
  bool handled = true;

  switch(type)
  {
    case Event::PauseMode:
      if(myState == S_EMULATE)
        setEventState(S_PAUSE);
      else if(myState == S_PAUSE)
        setEventState(S_EMULATE);
      else
        handled = false;
      break;

    case Event::MenuMode:
      if(myState == S_EMULATE)
        enterMenuMode(S_MENU);
      else
        handled = false;
      break;

    case Event::CmdMenuMode:
      if(myState == S_EMULATE)
        enterMenuMode(S_CMDMENU);
      else if(myState == S_CMDMENU)
        leaveMenuMode();
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

  ostringstream buf;

  // Fill the ActionList with the current key and joystick mappings
  for(int i = 0; i < listsize; ++i)
  {
    Event::Type event = list[i].event;
    free(list[i].key);  list[i].key = NULL;
    list[i].key = strdup("None");
    string key = "";
    for(int j = 0; j < KBDK_LAST; ++j)   // key mapping
    {
      if(myKeyTable[j][mode] == event)
      {
        if(key == "")
          key = key + ourKBDKMapping[j];
        else
          key = key + ", " + ourKBDKMapping[j];
      }
    }

#ifdef JOYSTICK_SUPPORT
    for(uInt32 stick = 0; stick < myNumJoysticks; ++stick)
    {
      const StellaJoystick& joy = myJoysticks[stick];

      // Joystick button mapping/labeling
      for(int button = 0; button < joy.numButtons; ++button)
      {
        if(joy.btnTable[button][mode] == event)
        {
          buf.str("");
          buf << "J" << stick << "/B" << button;
          if(key == "")
            key = key + buf.str();
          else
            key = key + ", " + buf.str();
        }
      }

      // Joystick axis mapping/labeling
      for(int axis = 0; axis < joy.numAxes; ++axis)
      {
        for(int dir = 0; dir < 2; ++dir)
        {
          if(joy.axisTable[axis][dir][mode] == event)
          {
            buf.str("");
            buf << "J" << stick << "/A" << axis;
            if(eventIsAnalog(event))
            {
              dir = 2;  // Immediately exit the inner loop after this iteration
              buf << "/+|-";
            }
            else if(dir == 0)
              buf << "/-";
            else
              buf << "/+";

            if(key == "")
              key = key + buf.str();
            else
              key = key + ", " + buf.str();
          }
        }
      }

      // Joystick hat mapping/labeling
      for(int hat = 0; hat < joy.numHats; ++hat)
      {
        for(int dir = 0; dir < 4; ++dir)
        {
          if(joy.hatTable[hat][dir][mode] == event)
          {
            buf.str("");
            buf << "J" << stick << "/H" << hat;
            switch(dir)
            {
              case EVENT_HATUP:    buf << "/up";    break;
              case EVENT_HATDOWN:  buf << "/down";  break;
              case EVENT_HATLEFT:  buf << "/left";  break;
              case EVENT_HATRIGHT: buf << "/right"; break;
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
    else if(event == Event::UINavNext)
      prepend = "TAB";
    else if(event == Event::UINavPrev)
      prepend = "Shift-TAB";
    // else if ...

    if(key == "")
      key = prepend;
    else if(prepend != "")
      key = prepend + ", " + key;

    if(key != "")
    {
      free(list[i].key);  list[i].key = NULL;
      list[i].key = strdup(key.c_str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setKeymap()
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = myOSystem->settings().getString("keymap");
  replace(list.begin(), list.end(), ':', ' ');
  istringstream buf(list);

  IntArray map;
  int value;
  Event::Type event;

  // Get event count, which should be the first int in the list
  buf >> value;
  event = (Event::Type) value;
  if(event == Event::LastType)
    while(buf >> value)
      map.push_back(value);

  // Only fill the key mapping array if the data is valid
  if(event == Event::LastType && map.size() == KBDK_LAST * kNumModes)
  {
    // Fill the keymap table with events
    IntArray::const_iterator event = map.begin();
    for(int mode = 0; mode < kNumModes; ++mode)
      for(int i = 0; i < KBDK_LAST; ++i)
        myKeyTable[i][mode] = (Event::Type) *event++;
  }
  else
  {
    setDefaultKeymap(Event::NoType, kEmulationMode);
    setDefaultKeymap(Event::NoType, kMenuMode);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setJoymap()
{
#ifdef JOYSTICK_SUPPORT
  setDefaultJoymap(Event::NoType, kEmulationMode);
  setDefaultJoymap(Event::NoType, kMenuMode);

  // Get all mappings from the settings
  istringstream buf(myOSystem->settings().getString("joymap"));
  string joymap;

  // First check the event type, and disregard the entire mapping if it's invalid
  getline(buf, joymap, '^');
  if(atoi(joymap.c_str()) != Event::LastType)
    return;

  // Otherwise, put each joystick mapping entry into a hashmap
  while(getline(buf, joymap, '^'))
  {
    istringstream namebuf(joymap);
    string joyname;
    getline(namebuf, joyname, '|');
    if(joyname.length() != 0)
      myJoystickMap.insert(make_pair(joyname, joymap));
  }

  // Next try to match the mappings to the specific joystick (by name)
  // We do it this way since a joystick may be unplugged and replugged,
  // but it's settings should stay the same
  for(uInt32 i = 0; i < myNumJoysticks; ++i)
  {
    StellaJoystick& joy = myJoysticks[i];
    if(joy.type == StellaJoystick::JT_REGULAR)
    {
      map<string,string>::const_iterator iter = myJoystickMap.find(joy.name);
      if(iter != myJoystickMap.end())
        joy.setMap(iter->second);
    }
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setComboMap()
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = myOSystem->settings().getString("combomap");
  replace(list.begin(), list.end(), ':', ' ');
  istringstream buf(list);

  // Get combo count, which should be the first int in the list
  // If it isn't, then we treat the entire list as invalid
  string key;
  buf >> key;
  if(atoi(key.c_str()) == kComboSize)
  {
    // Fill the combomap table with events for as long as they exist
    int combocount = 0;
    while(buf >> key && combocount < kComboSize)
    {
      // Each event in a comboevent is separated by a comma
      replace(key.begin(), key.end(), ',', ' ');
      istringstream buf2(key);

      int eventcount = 0;
      while(buf2 >> key && eventcount < kEventsPerCombo)
      {
        myComboTable[combocount][eventcount] = (Event::Type) atoi(key.c_str());
        ++eventcount;
      }
      ++combocount;
    }
  }
  else
  {
    // Erase the 'combo' array
    for(int i = 0; i < kComboSize; ++i)
      for(int j = 0; j < kEventsPerCombo; ++j)
        myComboTable[i][j] = Event::NoType;
  }

  saveComboMapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addKeyMapping(Event::Type event, EventMode mode, StellaKey key)
{
  // These keys cannot be remapped
  if(key == KBDK_TAB || eventIsAnalog(event))
    return false;
  else
  {
    myKeyTable[key][mode] = event;
    setActionMappings(mode);

    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyAxisMapping(Event::Type event, EventMode mode,
                                     int stick, int axis, int value,
                                     bool updateMenus)
{
#ifdef JOYSTICK_SUPPORT
  if(stick >= 0 && stick < (int)myNumJoysticks)
  {
    const StellaJoystick& joy = myJoysticks[stick];
    if(axis >= 0 && axis < joy.numAxes &&
       event >= 0 && event < Event::LastType)
    {
      // This confusing code is because each axis has two associated values,
      // but analog events only affect one of the axis.
      if(eventIsAnalog(event))
        joy.axisTable[axis][0][mode] =
          joy.axisTable[axis][1][mode] = event;
      else
      {
        // Otherwise, turn off the analog event(s) for this axis
        if(eventIsAnalog(joy.axisTable[axis][0][mode]))
          joy.axisTable[axis][0][mode] = Event::NoType;
        if(eventIsAnalog(joy.axisTable[axis][1][mode]))
          joy.axisTable[axis][1][mode] = Event::NoType;
    
        joy.axisTable[axis][(value > 0)][mode] = event;
      }
      if(updateMenus)
        setActionMappings(mode);
      return true;
    }
  }
  return false;
#else
  return false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyButtonMapping(Event::Type event, EventMode mode,
                                       int stick, int button,
                                       bool updateMenus)
{
#ifdef JOYSTICK_SUPPORT
  if(stick >= 0 && stick < (int)myNumJoysticks && !eventIsAnalog(event))
  {
    const StellaJoystick& joy = myJoysticks[stick];
    if(button >= 0 && button < joy.numButtons &&
       event >= 0 && event < Event::LastType)
    {
      joy.btnTable[button][mode] = event;
      if(updateMenus)
        setActionMappings(mode);
      return true;
    }
  }
  return false;
#else
  return false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::addJoyHatMapping(Event::Type event, EventMode mode,
                                    int stick, int hat, int value,
                                    bool updateMenus)
{
#ifdef JOYSTICK_SUPPORT
  if(stick >= 0 && stick < (int)myNumJoysticks && !eventIsAnalog(event))
  {
    const StellaJoystick& joy = myJoysticks[stick];
    if(hat >= 0 && hat < joy.numHats &&
       event >= 0 && event < Event::LastType && value != EVENT_HATCENTER)
    {
      joy.hatTable[hat][value][mode] = event;
      if(updateMenus)
        setActionMappings(mode);
      return true;
    }
  }
  return false;
#else
  return false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::eraseMapping(Event::Type event, EventMode mode)
{
  // Erase the KeyEvent arrays
  for(int i = 0; i < KBDK_LAST; ++i)
    if(myKeyTable[i][mode] == event && i != KBDK_TAB)
      myKeyTable[i][mode] = Event::NoType;

#ifdef JOYSTICK_SUPPORT
  // Erase the joystick mapping arrays
  for(uInt32 i = 0; i < myNumJoysticks; ++i)
    myJoysticks[i].eraseMap(mode);
#endif

  setActionMappings(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultMapping(Event::Type event, EventMode mode)
{
  setDefaultKeymap(event, mode);
  setDefaultJoymap(event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultKeymap(Event::Type event, EventMode mode)
{
#define SET_DEFAULT_KEY(sdk_key, sdk_mode, sdk_event, sdk_cmp_event) \
  if(eraseAll || sdk_cmp_event == sdk_event)  \
    myKeyTable[sdk_key][sdk_mode] = sdk_event;

  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  bool eraseAll = (event == Event::NoType);
  if(eraseAll)
  {
    // Erase all mappings
    for(int i = 0; i < KBDK_LAST; ++i)
      myKeyTable[i][mode] = Event::NoType;
  }

  switch(mode)
  {
    case kEmulationMode:
      SET_DEFAULT_KEY(KBDK_1,         mode, Event::KeyboardZero1,     event);
      SET_DEFAULT_KEY(KBDK_2,         mode, Event::KeyboardZero2,     event);
      SET_DEFAULT_KEY(KBDK_3,         mode, Event::KeyboardZero3,     event);
      SET_DEFAULT_KEY(KBDK_q,         mode, Event::KeyboardZero4,     event);
      SET_DEFAULT_KEY(KBDK_w,         mode, Event::KeyboardZero5,     event);
      SET_DEFAULT_KEY(KBDK_e,         mode, Event::KeyboardZero6,     event);
      SET_DEFAULT_KEY(KBDK_a,         mode, Event::KeyboardZero7,     event);
      SET_DEFAULT_KEY(KBDK_s,         mode, Event::KeyboardZero8,     event);
      SET_DEFAULT_KEY(KBDK_d,         mode, Event::KeyboardZero9,     event);
      SET_DEFAULT_KEY(KBDK_z,         mode, Event::KeyboardZeroStar,  event);
      SET_DEFAULT_KEY(KBDK_x,         mode, Event::KeyboardZero0,     event);
      SET_DEFAULT_KEY(KBDK_c,         mode, Event::KeyboardZeroPound, event);

      SET_DEFAULT_KEY(KBDK_8,         mode, Event::KeyboardOne1,      event);
      SET_DEFAULT_KEY(KBDK_9,         mode, Event::KeyboardOne2,      event);
      SET_DEFAULT_KEY(KBDK_0,         mode, Event::KeyboardOne3,      event);
      SET_DEFAULT_KEY(KBDK_i,         mode, Event::KeyboardOne4,      event);
      SET_DEFAULT_KEY(KBDK_o,         mode, Event::KeyboardOne5,      event);
      SET_DEFAULT_KEY(KBDK_p,         mode, Event::KeyboardOne6,      event);
      SET_DEFAULT_KEY(KBDK_k,         mode, Event::KeyboardOne7,      event);
      SET_DEFAULT_KEY(KBDK_l,         mode, Event::KeyboardOne8,      event);
      SET_DEFAULT_KEY(KBDK_SEMICOLON, mode, Event::KeyboardOne9,      event);
      SET_DEFAULT_KEY(KBDK_COMMA,     mode, Event::KeyboardOneStar,   event);
      SET_DEFAULT_KEY(KBDK_PERIOD,    mode, Event::KeyboardOne0,      event);
      SET_DEFAULT_KEY(KBDK_SLASH,     mode, Event::KeyboardOnePound,  event);

      SET_DEFAULT_KEY(KBDK_UP,        mode, Event::JoystickZeroUp,    event);
      SET_DEFAULT_KEY(KBDK_DOWN,      mode, Event::JoystickZeroDown,  event);
      SET_DEFAULT_KEY(KBDK_LEFT,      mode, Event::JoystickZeroLeft,  event);
      SET_DEFAULT_KEY(KBDK_RIGHT,     mode, Event::JoystickZeroRight, event);
      SET_DEFAULT_KEY(KBDK_SPACE,     mode, Event::JoystickZeroFire,  event);
      SET_DEFAULT_KEY(KBDK_LCTRL,     mode, Event::JoystickZeroFire,  event);
      SET_DEFAULT_KEY(KBDK_4,         mode, Event::JoystickZeroFire5, event);
      SET_DEFAULT_KEY(KBDK_5,         mode, Event::JoystickZeroFire9, event);

      SET_DEFAULT_KEY(KBDK_y,         mode, Event::JoystickOneUp,     event);
      SET_DEFAULT_KEY(KBDK_h,         mode, Event::JoystickOneDown,   event);
      SET_DEFAULT_KEY(KBDK_g,         mode, Event::JoystickOneLeft,   event);
      SET_DEFAULT_KEY(KBDK_j,         mode, Event::JoystickOneRight,  event);
      SET_DEFAULT_KEY(KBDK_f,         mode, Event::JoystickOneFire,   event);
      SET_DEFAULT_KEY(KBDK_6,         mode, Event::JoystickOneFire5,  event);
      SET_DEFAULT_KEY(KBDK_7,         mode, Event::JoystickOneFire9,  event);


      SET_DEFAULT_KEY(KBDK_F1,        mode, Event::ConsoleSelect,     event);
      SET_DEFAULT_KEY(KBDK_F2,        mode, Event::ConsoleReset,      event);
      SET_DEFAULT_KEY(KBDK_F3,        mode, Event::ConsoleColor,      event);
      SET_DEFAULT_KEY(KBDK_F4,        mode, Event::ConsoleBlackWhite, event);
      SET_DEFAULT_KEY(KBDK_F5,        mode, Event::ConsoleLeftDiffA,  event);
      SET_DEFAULT_KEY(KBDK_F6,        mode, Event::ConsoleLeftDiffB,  event);
      SET_DEFAULT_KEY(KBDK_F7,        mode, Event::ConsoleRightDiffA, event);
      SET_DEFAULT_KEY(KBDK_F8,        mode, Event::ConsoleRightDiffB, event);
      SET_DEFAULT_KEY(KBDK_F9,        mode, Event::SaveState,         event);
      SET_DEFAULT_KEY(KBDK_F10,       mode, Event::ChangeState,       event);
      SET_DEFAULT_KEY(KBDK_F11,       mode, Event::LoadState,         event);
      SET_DEFAULT_KEY(KBDK_F12,       mode, Event::TakeSnapshot,      event);
      SET_DEFAULT_KEY(KBDK_BACKSPACE, mode, Event::Fry,               event);
      SET_DEFAULT_KEY(KBDK_PAUSE,     mode, Event::PauseMode,         event);
      SET_DEFAULT_KEY(KBDK_TAB,       mode, Event::MenuMode,          event);
      SET_DEFAULT_KEY(KBDK_BACKSLASH, mode, Event::CmdMenuMode,       event);
      SET_DEFAULT_KEY(KBDK_BACKQUOTE, mode, Event::DebuggerMode,      event);
      SET_DEFAULT_KEY(KBDK_ESCAPE,    mode, Event::LauncherMode,      event);
      break;

    case kMenuMode:
      SET_DEFAULT_KEY(KBDK_UP,        mode, Event::UIUp,      event);
      SET_DEFAULT_KEY(KBDK_DOWN,      mode, Event::UIDown,    event);
      SET_DEFAULT_KEY(KBDK_LEFT,      mode, Event::UILeft,    event);
      SET_DEFAULT_KEY(KBDK_RIGHT,     mode, Event::UIRight,   event);

      SET_DEFAULT_KEY(KBDK_HOME,      mode, Event::UIHome,    event);
      SET_DEFAULT_KEY(KBDK_END,       mode, Event::UIEnd,     event);
      SET_DEFAULT_KEY(KBDK_PAGEUP,    mode, Event::UIPgUp,    event);
      SET_DEFAULT_KEY(KBDK_PAGEDOWN,  mode, Event::UIPgDown,  event);

      SET_DEFAULT_KEY(KBDK_RETURN,    mode, Event::UISelect,  event);
      SET_DEFAULT_KEY(KBDK_ESCAPE,    mode, Event::UICancel,  event);

      SET_DEFAULT_KEY(KBDK_BACKSPACE, mode, Event::UIPrevDir, event);
      break;

    default:
      return;
      break;
  }
  setActionMappings(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultJoymap(Event::Type event, EventMode mode)
{
#ifdef JOYSTICK_SUPPORT
  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  for(uInt32 i = 0; i < myNumJoysticks; ++i)
  {
    if(event == Event::NoType)
      myJoysticks[i].eraseMap(mode);     // erase *all* mappings 
    else
      myJoysticks[i].eraseEvent(event, mode);  // only reset the specific event
  }
  myOSystem->setDefaultJoymap(event, mode);
  setActionMappings(mode);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveKeyMapping()
{
  // Iterate through the keymap table and create a colon-separated list
  // Prepend the event count, so we can check it on next load
  ostringstream keybuf;
  keybuf << Event::LastType;
  for(int mode = 0; mode < kNumModes; ++mode)
    for(int i = 0; i < KBDK_LAST; ++i)
      keybuf << ":" << myKeyTable[i][mode];

  myOSystem->settings().setString("keymap", keybuf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveJoyMapping()
{
#ifdef JOYSTICK_SUPPORT
  // Don't update the joymap at all if it hasn't been modified during the
  // program run
  if(myNumJoysticks == 0)
    return;

  // Save the joystick mapping hash table, making sure to update it with
  // any changes that have been made during the program run
  for(uInt32 i = 0; i < myNumJoysticks; ++i)
  {
    const StellaJoystick& joy = myJoysticks[i];
    if(joy.type == StellaJoystick::JT_REGULAR)
    {
      // Update hashmap, removing the joystick entry (if it exists)
      // and adding the most recent mapping from the joystick itself
      map<string,string>::iterator iter = myJoystickMap.find(joy.name);
      if(iter != myJoystickMap.end())
        myJoystickMap.erase(iter);
      myJoystickMap.insert(make_pair(joy.name, joy.getMap()));
    }
  }

  // Now save the contents of the hashmap
  ostringstream joybuf;
  joybuf << Event::LastType;
  map<string,string>::const_iterator iter;
  for(iter = myJoystickMap.begin(); iter != myJoystickMap.end(); ++iter)
    joybuf << "^" << iter->second;

  myOSystem->settings().setString("joymap", joybuf.str());
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::saveComboMapping()
{
  // Iterate through the combomap table and create a colon-separated list
  // For each combo event, create a comma-separated list of its events
  // Prepend the event count, so we can check it on next load
  ostringstream buf;
  buf << kComboSize;
  for(int i = 0; i < kComboSize; ++i)
  {
    buf << ":" << myComboTable[i][0];
    for(int j = 1; j < kEventsPerCombo; ++j)
      buf << "," << myComboTable[i][j];
  }
  myOSystem->settings().setString("combomap", buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool EventHandler::eventIsAnalog(Event::Type event) const
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
void EventHandler::getActionList(EventMode mode, StringList& l) const
{
  switch(mode)
  {
    case kEmulationMode:
      for(uInt32 i = 0; i < kEmulActionListSize; ++i)
        l.push_back(EventHandler::ourEmulActionList[i].action);
      break;
    case kMenuMode:
      for(uInt32 i = 0; i < kMenuActionListSize; ++i)
        l.push_back(EventHandler::ourMenuActionList[i].action);
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::getComboList(EventMode, StringMap& l) const
{
  // For now, this only works in emulation mode

  ostringstream buf;

  l.push_back("None", "-1");
  for(uInt32 i = 0; i < kEmulActionListSize; ++i)
    if(EventHandler::ourEmulActionList[i].allow_combo)
    {
      buf << i;
      l.push_back(EventHandler::ourEmulActionList[i].action, buf.str());
      buf.str("");
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::getComboListForEvent(Event::Type event, StringList& l) const
{
  ostringstream buf;
  if(event >= Event::Combo1 && event <= Event::Combo16)
  {
    int combo = event - Event::Combo1;
    for(uInt32 i = 0; i < kEventsPerCombo; ++i)
    {
      Event::Type e = myComboTable[combo][i];
      for(uInt32 j = 0; j < kEmulActionListSize; ++j)
      {
        if(EventHandler::ourEmulActionList[j].event == e &&
           EventHandler::ourEmulActionList[j].allow_combo)
        {
          buf << j;
          l.push_back(buf.str());
          buf.str("");
        }
      }
      // Make sure entries are 1-to-1, using '-1' to indicate Event::NoType
      if(i == l.size())
        l.push_back("-1");
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setComboListForEvent(Event::Type event, const StringList& events)
{
  if(event >= Event::Combo1 && event <= Event::Combo16)
  {
    assert(events.size() == 8);
    int combo = event - Event::Combo1;
    for(int i = 0; i < 8; ++i)
    {
      int idx = atoi(events[i].c_str());
      if(idx >=0 && idx < kEmulActionListSize)
        myComboTable[combo][i] = EventHandler::ourEmulActionList[idx].event;
      else
        myComboTable[combo][i] = Event::NoType;
    }
    saveComboMapping();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type EventHandler::eventAtIndex(int idx, EventMode mode) const
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
string EventHandler::actionAtIndex(int idx, EventMode mode) const
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
string EventHandler::keyAtIndex(int idx, EventMode mode) const
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
void EventHandler::takeSnapshot(uInt32 number)
{
  // Figure out the correct snapshot name
  string filename;
  bool showmessage = number == 0;
  string sspath = myOSystem->snapshotDir() +
      myOSystem->console().properties().get(Cartridge_Name);

  // Check whether we want multiple snapshots created
  if(number > 0)
  {
    ostringstream buf;
    buf << sspath << "_" << hex << setw(8) << setfill('0') << number << ".png";
    filename = buf.str();
  }
  else if(!myOSystem->settings().getBool("sssingle"))
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    filename = sspath + ".png";
    FilesystemNode node(filename);
    if(node.exists())
    {
      ostringstream buf;
      for(uInt32 i = 1; ;++i)
      {
        buf.str("");
        buf << sspath << "_" << i << ".png";
        FilesystemNode next(buf.str());
        if(!next.exists())
          break;
      }
      filename = buf.str();
    }
  }
  else
    filename = sspath + ".png";

  // Now create a PNG snapshot
  if(myOSystem->settings().getBool("ss1x"))
  {
    string msg = Snapshot::savePNG(myOSystem->frameBuffer(),
                   myOSystem->console().tia(),
                   myOSystem->console().properties(), filename);
    if(showmessage)
      myOSystem->frameBuffer().showMessage(msg);
  }
  else
  {
    // Make sure we have a 'clean' image, with no onscreen messages
    myOSystem->frameBuffer().enableMessages(false);

    string msg = Snapshot::savePNG(myOSystem->frameBuffer(),
                   myOSystem->console().properties(), filename);

    // Re-enable old messages
    myOSystem->frameBuffer().enableMessages(true);
    if(showmessage)
      myOSystem->frameBuffer().showMessage(msg);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setMouseControllerMode(const string& mode)
{
  delete myMouseControl;  myMouseControl = NULL;
  if(&myOSystem->console())
  {
    const string& control = mode == "rom" ?
      myOSystem->console().properties().get(Controller_MouseAxis) : mode;

    myMouseControl = new MouseControl(myOSystem->console(), control);
    myMouseControl->next();  // set first available mode
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setContinuousSnapshots(uInt32 interval)
{
  myContSnapshotInterval = (uInt32) myOSystem->frameRate() * interval;
  myContSnapshotCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::enterMenuMode(State state)
{
  setEventState(state);
  myOverlay->reStack();
  myOSystem->sound().mute(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveMenuMode()
{
  setEventState(S_EMULATE);
  myOSystem->sound().mute(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::enterDebugMode()
{
#ifdef DEBUGGER_SUPPORT
  if(myState == S_DEBUGGER || !(&myOSystem->console()))
    return false;

  // Make sure debugger starts in a consistent state
  // This absolutely *has* to come before we actually change to debugger
  // mode, since it takes care of locking the debugger state, which will
  // probably be modified below
  myOSystem->debugger().setStartState();
  setEventState(S_DEBUGGER);

  FBInitStatus fbstatus = myOSystem->createFrameBuffer();
  if(fbstatus != kSuccess)
  {
    myOSystem->debugger().setQuitState();
    setEventState(S_EMULATE);
    if(fbstatus == kFailTooLarge)
      myOSystem->frameBuffer().showMessage("Debugger window too large for screen",
                                           kBottomCenter, true);
    return false;
  }
  myOverlay->reStack();
  myOSystem->sound().mute(true);
#else
  myOSystem->frameBuffer().showMessage("Debugger support not included",
                                       kBottomCenter, true);
#endif

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveDebugMode()
{
#ifdef DEBUGGER_SUPPORT
  // paranoia: this should never happen:
  if(myState != S_DEBUGGER)
    return;

  // Make sure debugger quits in a consistent state
  myOSystem->debugger().setQuitState();

  setEventState(S_EMULATE);
  myOSystem->createFrameBuffer();
  myOSystem->sound().mute(false);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setEventState(State state)
{
  myState = state;

  // Normally, the usage of Control key is determined by 'ctrlcombo'
  // For certain ROMs it may be forced off, whatever the setting
  myUseCtrlKeyFlag = myOSystem->settings().getBool("ctrlcombo");

  // Only enable Unicode in GUI modes, since there we need it for ascii data
  // Otherwise, it causes a performance hit, so leave it off
  switch(myState)
  {
    case S_EMULATE:
      myOverlay = NULL;
      myOSystem->sound().mute(false);
      SDL_EnableUNICODE(0);
      if(myOSystem->console().controller(Controller::Left).type() ==
            Controller::CompuMate)
        myUseCtrlKeyFlag = false;
      break;

    case S_PAUSE:
      myOverlay = NULL;
      myOSystem->sound().mute(true);
      SDL_EnableUNICODE(0);
      break;

    case S_MENU:
      myOverlay = &myOSystem->menu();
      SDL_EnableUNICODE(1);
      break;

    case S_CMDMENU:
      myOverlay = &myOSystem->commandMenu();
      SDL_EnableUNICODE(1);
      break;

    case S_LAUNCHER:
      myOverlay = &myOSystem->launcher();
      SDL_EnableUNICODE(1);
      break;

#ifdef DEBUGGER_SUPPORT
    case S_DEBUGGER:
      myOverlay = &myOSystem->debugger();
      SDL_EnableUNICODE(1);
      break;
#endif

    default:
      myOverlay = NULL;
      break;
  }

  // Inform various subsystems about the new state
  myOSystem->stateChanged(myState);
  if(&myOSystem->frameBuffer())
  {
    myOSystem->frameBuffer().stateChanged(myState);
    myOSystem->frameBuffer().setCursorState();
  }

  // Always clear any pending events when changing states
  myEvent.clear();

  // Sometimes an extraneous mouse motion event is generated
  // after a state change, which should be supressed
  mySkipMouseMotion = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setSDLMappings()
{
  ourKBDKMapping[ KBDK_BACKSPACE ]    = "BACKSPACE";
  ourKBDKMapping[ KBDK_TAB ]          = "TAB";
  ourKBDKMapping[ KBDK_CLEAR ]        = "CLEAR";
  ourKBDKMapping[ KBDK_RETURN ]       = "RETURN";
  ourKBDKMapping[ KBDK_PAUSE ]        = "PAUSE";
  ourKBDKMapping[ KBDK_ESCAPE ]       = "ESCAPE";
  ourKBDKMapping[ KBDK_SPACE ]        = "SPACE";
  ourKBDKMapping[ KBDK_EXCLAIM ]      = "!";
  ourKBDKMapping[ KBDK_QUOTEDBL ]     = "\"";
  ourKBDKMapping[ KBDK_HASH ]         = "#";
  ourKBDKMapping[ KBDK_DOLLAR ]       = "$";
  ourKBDKMapping[ KBDK_AMPERSAND ]    = "&";
  ourKBDKMapping[ KBDK_QUOTE ]        = "\'";
  ourKBDKMapping[ KBDK_LEFTPAREN ]    = "(";
  ourKBDKMapping[ KBDK_RIGHTPAREN ]   = ")";
  ourKBDKMapping[ KBDK_ASTERISK ]     = "*";
  ourKBDKMapping[ KBDK_PLUS ]         = "+";
  ourKBDKMapping[ KBDK_COMMA ]        = "COMMA";
  ourKBDKMapping[ KBDK_MINUS ]        = "-";
  ourKBDKMapping[ KBDK_PERIOD ]       = ".";
  ourKBDKMapping[ KBDK_SLASH ]        = "/";
  ourKBDKMapping[ KBDK_0 ]            = "0";
  ourKBDKMapping[ KBDK_1 ]            = "1";
  ourKBDKMapping[ KBDK_2 ]            = "2";
  ourKBDKMapping[ KBDK_3 ]            = "3";
  ourKBDKMapping[ KBDK_4 ]            = "4";
  ourKBDKMapping[ KBDK_5 ]            = "5";
  ourKBDKMapping[ KBDK_6 ]            = "6";
  ourKBDKMapping[ KBDK_7 ]            = "7";
  ourKBDKMapping[ KBDK_8 ]            = "8";
  ourKBDKMapping[ KBDK_9 ]            = "9";
  ourKBDKMapping[ KBDK_COLON ]        = ":";
  ourKBDKMapping[ KBDK_SEMICOLON ]    = ";";
  ourKBDKMapping[ KBDK_LESS ]         = "<";
  ourKBDKMapping[ KBDK_EQUALS ]       = "=";
  ourKBDKMapping[ KBDK_GREATER ]      = ">";
  ourKBDKMapping[ KBDK_QUESTION ]     = "?";
  ourKBDKMapping[ KBDK_AT ]           = "@";
  ourKBDKMapping[ KBDK_LEFTBRACKET ]  = "[";
  ourKBDKMapping[ KBDK_BACKSLASH ]    = "\\";
  ourKBDKMapping[ KBDK_RIGHTBRACKET ] = "]";
  ourKBDKMapping[ KBDK_CARET ]        = "^";
  ourKBDKMapping[ KBDK_UNDERSCORE ]   = "_";
  ourKBDKMapping[ KBDK_BACKQUOTE ]    = "`";
  ourKBDKMapping[ KBDK_a ]            = "A";
  ourKBDKMapping[ KBDK_b ]            = "B";
  ourKBDKMapping[ KBDK_c ]            = "C";
  ourKBDKMapping[ KBDK_d ]            = "D";
  ourKBDKMapping[ KBDK_e ]            = "E";
  ourKBDKMapping[ KBDK_f ]            = "F";
  ourKBDKMapping[ KBDK_g ]            = "G";
  ourKBDKMapping[ KBDK_h ]            = "H";
  ourKBDKMapping[ KBDK_i ]            = "I";
  ourKBDKMapping[ KBDK_j ]            = "J";
  ourKBDKMapping[ KBDK_k ]            = "K";
  ourKBDKMapping[ KBDK_l ]            = "L";
  ourKBDKMapping[ KBDK_m ]            = "M";
  ourKBDKMapping[ KBDK_n ]            = "N";
  ourKBDKMapping[ KBDK_o ]            = "O";
  ourKBDKMapping[ KBDK_p ]            = "P";
  ourKBDKMapping[ KBDK_q ]            = "Q";
  ourKBDKMapping[ KBDK_r ]            = "R";
  ourKBDKMapping[ KBDK_s ]            = "S";
  ourKBDKMapping[ KBDK_t ]            = "T";
  ourKBDKMapping[ KBDK_u ]            = "U";
  ourKBDKMapping[ KBDK_v ]            = "V";
  ourKBDKMapping[ KBDK_w ]            = "W";
  ourKBDKMapping[ KBDK_x ]            = "X";
  ourKBDKMapping[ KBDK_y ]            = "Y";
  ourKBDKMapping[ KBDK_z ]            = "Z";
  ourKBDKMapping[ KBDK_DELETE ]       = "DELETE";
  ourKBDKMapping[ KBDK_WORLD_0 ]      = "WORLD_0";
  ourKBDKMapping[ KBDK_WORLD_1 ]      = "WORLD_1";
  ourKBDKMapping[ KBDK_WORLD_2 ]      = "WORLD_2";
  ourKBDKMapping[ KBDK_WORLD_3 ]      = "WORLD_3";
  ourKBDKMapping[ KBDK_WORLD_4 ]      = "WORLD_4";
  ourKBDKMapping[ KBDK_WORLD_5 ]      = "WORLD_5";
  ourKBDKMapping[ KBDK_WORLD_6 ]      = "WORLD_6";
  ourKBDKMapping[ KBDK_WORLD_7 ]      = "WORLD_7";
  ourKBDKMapping[ KBDK_WORLD_8 ]      = "WORLD_8";
  ourKBDKMapping[ KBDK_WORLD_9 ]      = "WORLD_9";
  ourKBDKMapping[ KBDK_WORLD_10 ]     = "WORLD_10";
  ourKBDKMapping[ KBDK_WORLD_11 ]     = "WORLD_11";
  ourKBDKMapping[ KBDK_WORLD_12 ]     = "WORLD_12";
  ourKBDKMapping[ KBDK_WORLD_13 ]     = "WORLD_13";
  ourKBDKMapping[ KBDK_WORLD_14 ]     = "WORLD_14";
  ourKBDKMapping[ KBDK_WORLD_15 ]     = "WORLD_15";
  ourKBDKMapping[ KBDK_WORLD_16 ]     = "WORLD_16";
  ourKBDKMapping[ KBDK_WORLD_17 ]     = "WORLD_17";
  ourKBDKMapping[ KBDK_WORLD_18 ]     = "WORLD_18";
  ourKBDKMapping[ KBDK_WORLD_19 ]     = "WORLD_19";
  ourKBDKMapping[ KBDK_WORLD_20 ]     = "WORLD_20";
  ourKBDKMapping[ KBDK_WORLD_21 ]     = "WORLD_21";
  ourKBDKMapping[ KBDK_WORLD_22 ]     = "WORLD_22";
  ourKBDKMapping[ KBDK_WORLD_23 ]     = "WORLD_23";
  ourKBDKMapping[ KBDK_WORLD_24 ]     = "WORLD_24";
  ourKBDKMapping[ KBDK_WORLD_25 ]     = "WORLD_25";
  ourKBDKMapping[ KBDK_WORLD_26 ]     = "WORLD_26";
  ourKBDKMapping[ KBDK_WORLD_27 ]     = "WORLD_27";
  ourKBDKMapping[ KBDK_WORLD_28 ]     = "WORLD_28";
  ourKBDKMapping[ KBDK_WORLD_29 ]     = "WORLD_29";
  ourKBDKMapping[ KBDK_WORLD_30 ]     = "WORLD_30";
  ourKBDKMapping[ KBDK_WORLD_31 ]     = "WORLD_31";
  ourKBDKMapping[ KBDK_WORLD_32 ]     = "WORLD_32";
  ourKBDKMapping[ KBDK_WORLD_33 ]     = "WORLD_33";
  ourKBDKMapping[ KBDK_WORLD_34 ]     = "WORLD_34";
  ourKBDKMapping[ KBDK_WORLD_35 ]     = "WORLD_35";
  ourKBDKMapping[ KBDK_WORLD_36 ]     = "WORLD_36";
  ourKBDKMapping[ KBDK_WORLD_37 ]     = "WORLD_37";
  ourKBDKMapping[ KBDK_WORLD_38 ]     = "WORLD_38";
  ourKBDKMapping[ KBDK_WORLD_39 ]     = "WORLD_39";
  ourKBDKMapping[ KBDK_WORLD_40 ]     = "WORLD_40";
  ourKBDKMapping[ KBDK_WORLD_41 ]     = "WORLD_41";
  ourKBDKMapping[ KBDK_WORLD_42 ]     = "WORLD_42";
  ourKBDKMapping[ KBDK_WORLD_43 ]     = "WORLD_43";
  ourKBDKMapping[ KBDK_WORLD_44 ]     = "WORLD_44";
  ourKBDKMapping[ KBDK_WORLD_45 ]     = "WORLD_45";
  ourKBDKMapping[ KBDK_WORLD_46 ]     = "WORLD_46";
  ourKBDKMapping[ KBDK_WORLD_47 ]     = "WORLD_47";
  ourKBDKMapping[ KBDK_WORLD_48 ]     = "WORLD_48";
  ourKBDKMapping[ KBDK_WORLD_49 ]     = "WORLD_49";
  ourKBDKMapping[ KBDK_WORLD_50 ]     = "WORLD_50";
  ourKBDKMapping[ KBDK_WORLD_51 ]     = "WORLD_51";
  ourKBDKMapping[ KBDK_WORLD_52 ]     = "WORLD_52";
  ourKBDKMapping[ KBDK_WORLD_53 ]     = "WORLD_53";
  ourKBDKMapping[ KBDK_WORLD_54 ]     = "WORLD_54";
  ourKBDKMapping[ KBDK_WORLD_55 ]     = "WORLD_55";
  ourKBDKMapping[ KBDK_WORLD_56 ]     = "WORLD_56";
  ourKBDKMapping[ KBDK_WORLD_57 ]     = "WORLD_57";
  ourKBDKMapping[ KBDK_WORLD_58 ]     = "WORLD_58";
  ourKBDKMapping[ KBDK_WORLD_59 ]     = "WORLD_59";
  ourKBDKMapping[ KBDK_WORLD_60 ]     = "WORLD_60";
  ourKBDKMapping[ KBDK_WORLD_61 ]     = "WORLD_61";
  ourKBDKMapping[ KBDK_WORLD_62 ]     = "WORLD_62";
  ourKBDKMapping[ KBDK_WORLD_63 ]     = "WORLD_63";
  ourKBDKMapping[ KBDK_WORLD_64 ]     = "WORLD_64";
  ourKBDKMapping[ KBDK_WORLD_65 ]     = "WORLD_65";
  ourKBDKMapping[ KBDK_WORLD_66 ]     = "WORLD_66";
  ourKBDKMapping[ KBDK_WORLD_67 ]     = "WORLD_67";
  ourKBDKMapping[ KBDK_WORLD_68 ]     = "WORLD_68";
  ourKBDKMapping[ KBDK_WORLD_69 ]     = "WORLD_69";
  ourKBDKMapping[ KBDK_WORLD_70 ]     = "WORLD_70";
  ourKBDKMapping[ KBDK_WORLD_71 ]     = "WORLD_71";
  ourKBDKMapping[ KBDK_WORLD_72 ]     = "WORLD_72";
  ourKBDKMapping[ KBDK_WORLD_73 ]     = "WORLD_73";
  ourKBDKMapping[ KBDK_WORLD_74 ]     = "WORLD_74";
  ourKBDKMapping[ KBDK_WORLD_75 ]     = "WORLD_75";
  ourKBDKMapping[ KBDK_WORLD_76 ]     = "WORLD_76";
  ourKBDKMapping[ KBDK_WORLD_77 ]     = "WORLD_77";
  ourKBDKMapping[ KBDK_WORLD_78 ]     = "WORLD_78";
  ourKBDKMapping[ KBDK_WORLD_79 ]     = "WORLD_79";
  ourKBDKMapping[ KBDK_WORLD_80 ]     = "WORLD_80";
  ourKBDKMapping[ KBDK_WORLD_81 ]     = "WORLD_81";
  ourKBDKMapping[ KBDK_WORLD_82 ]     = "WORLD_82";
  ourKBDKMapping[ KBDK_WORLD_83 ]     = "WORLD_83";
  ourKBDKMapping[ KBDK_WORLD_84 ]     = "WORLD_84";
  ourKBDKMapping[ KBDK_WORLD_85 ]     = "WORLD_85";
  ourKBDKMapping[ KBDK_WORLD_86 ]     = "WORLD_86";
  ourKBDKMapping[ KBDK_WORLD_87 ]     = "WORLD_87";
  ourKBDKMapping[ KBDK_WORLD_88 ]     = "WORLD_88";
  ourKBDKMapping[ KBDK_WORLD_89 ]     = "WORLD_89";
  ourKBDKMapping[ KBDK_WORLD_90 ]     = "WORLD_90";
  ourKBDKMapping[ KBDK_WORLD_91 ]     = "WORLD_91";
  ourKBDKMapping[ KBDK_WORLD_92 ]     = "WORLD_92";
  ourKBDKMapping[ KBDK_WORLD_93 ]     = "WORLD_93";
  ourKBDKMapping[ KBDK_WORLD_94 ]     = "WORLD_94";
  ourKBDKMapping[ KBDK_WORLD_95 ]     = "WORLD_95";
  ourKBDKMapping[ KBDK_KP0 ]          = "KP0";
  ourKBDKMapping[ KBDK_KP1 ]          = "KP1";
  ourKBDKMapping[ KBDK_KP2 ]          = "KP2";
  ourKBDKMapping[ KBDK_KP3 ]          = "KP3";
  ourKBDKMapping[ KBDK_KP4 ]          = "KP4";
  ourKBDKMapping[ KBDK_KP5 ]          = "KP5";
  ourKBDKMapping[ KBDK_KP6 ]          = "KP6";
  ourKBDKMapping[ KBDK_KP7 ]          = "KP7";
  ourKBDKMapping[ KBDK_KP8 ]          = "KP8";
  ourKBDKMapping[ KBDK_KP9 ]          = "KP9";
  ourKBDKMapping[ KBDK_KP_PERIOD ]    = "KP .";
  ourKBDKMapping[ KBDK_KP_DIVIDE ]    = "KP /";
  ourKBDKMapping[ KBDK_KP_MULTIPLY ]  = "KP *";
  ourKBDKMapping[ KBDK_KP_MINUS ]     = "KP -";
  ourKBDKMapping[ KBDK_KP_PLUS ]      = "KP +";
  ourKBDKMapping[ KBDK_KP_ENTER ]     = "KP ENTER";
  ourKBDKMapping[ KBDK_KP_EQUALS ]    = "KP =";
  ourKBDKMapping[ KBDK_UP ]           = "UP";
  ourKBDKMapping[ KBDK_DOWN ]         = "DOWN";
  ourKBDKMapping[ KBDK_RIGHT ]        = "RIGHT";
  ourKBDKMapping[ KBDK_LEFT ]         = "LEFT";
  ourKBDKMapping[ KBDK_INSERT ]       = "INS";
  ourKBDKMapping[ KBDK_HOME ]         = "HOME";
  ourKBDKMapping[ KBDK_END ]          = "END";
  ourKBDKMapping[ KBDK_PAGEUP ]       = "PGUP";
  ourKBDKMapping[ KBDK_PAGEDOWN ]     = "PGDN";
  ourKBDKMapping[ KBDK_F1 ]           = "F1";
  ourKBDKMapping[ KBDK_F2 ]           = "F2";
  ourKBDKMapping[ KBDK_F3 ]           = "F3";
  ourKBDKMapping[ KBDK_F4 ]           = "F4";
  ourKBDKMapping[ KBDK_F5 ]           = "F5";
  ourKBDKMapping[ KBDK_F6 ]           = "F6";
  ourKBDKMapping[ KBDK_F7 ]           = "F7";
  ourKBDKMapping[ KBDK_F8 ]           = "F8";
  ourKBDKMapping[ KBDK_F9 ]           = "F9";
  ourKBDKMapping[ KBDK_F10 ]          = "F10";
  ourKBDKMapping[ KBDK_F11 ]          = "F11";
  ourKBDKMapping[ KBDK_F12 ]          = "F12";
  ourKBDKMapping[ KBDK_F13 ]          = "F13";
  ourKBDKMapping[ KBDK_F14 ]          = "F14";
  ourKBDKMapping[ KBDK_F15 ]          = "F15";
  ourKBDKMapping[ KBDK_NUMLOCK ]      = "NUMLOCK";
  ourKBDKMapping[ KBDK_CAPSLOCK ]     = "CAPSLOCK";
  ourKBDKMapping[ KBDK_SCROLLOCK ]    = "SCROLLOCK";
  ourKBDKMapping[ KBDK_RSHIFT ]       = "RSHIFT";
  ourKBDKMapping[ KBDK_LSHIFT ]       = "LSHIFT";
  ourKBDKMapping[ KBDK_RCTRL ]        = "RCTRL";
  ourKBDKMapping[ KBDK_LCTRL ]        = "LCTRL";
  ourKBDKMapping[ KBDK_RALT ]         = "RALT";
  ourKBDKMapping[ KBDK_LALT ]         = "LALT";
  ourKBDKMapping[ KBDK_RMETA ]        = "RMETA";
  ourKBDKMapping[ KBDK_LMETA ]        = "LMETA";
  ourKBDKMapping[ KBDK_LSUPER ]       = "LSUPER";
  ourKBDKMapping[ KBDK_RSUPER ]       = "RSUPER";
  ourKBDKMapping[ KBDK_MODE ]         = "MODE";
  ourKBDKMapping[ KBDK_COMPOSE ]      = "COMPOSE";
  ourKBDKMapping[ KBDK_HELP ]         = "HELP";
  ourKBDKMapping[ KBDK_PRINT ]        = "PRINT";
  ourKBDKMapping[ KBDK_SYSREQ ]       = "SYSREQ";
  ourKBDKMapping[ KBDK_BREAK ]        = "BREAK";
  ourKBDKMapping[ KBDK_MENU ]         = "MENU";
  ourKBDKMapping[ KBDK_POWER ]        = "POWER";
  ourKBDKMapping[ KBDK_EURO ]         = "EURO";
  ourKBDKMapping[ KBDK_UNDO ]         = "UNDO";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::ActionList EventHandler::ourEmulActionList[kEmulActionListSize] = {
  { Event::ConsoleSelect,       "Select",                      0, true  },
  { Event::ConsoleReset,        "Reset",                       0, true  },
  { Event::ConsoleColor,        "Color TV",                    0, true  },
  { Event::ConsoleBlackWhite,   "Black & White TV",            0, true  },
  { Event::ConsoleLeftDiffA,    "P0 Difficulty A",             0, true  },
  { Event::ConsoleLeftDiffB,    "P0 Difficulty B",             0, true  },
  { Event::ConsoleRightDiffA,   "P1 Difficulty A",             0, true  },
  { Event::ConsoleRightDiffB,   "P1 Difficulty B",             0, true  },
  { Event::SaveState,           "Save State",                  0, false },
  { Event::ChangeState,         "Change State",                0, false },
  { Event::LoadState,           "Load State",                  0, false },
  { Event::TakeSnapshot,        "Snapshot",                    0, false },
  { Event::Fry,                 "Fry cartridge",               0, false },
  { Event::VolumeDecrease,      "Decrease volume",             0, false },
  { Event::VolumeIncrease,      "Increase volume",             0, false },
  { Event::PauseMode,           "Pause",                       0, false },
  { Event::MenuMode,            "Enter options menu mode",     0, false },
  { Event::CmdMenuMode,         "Toggle command menu mode",    0, false },
  { Event::DebuggerMode,        "Toggle debugger mode",        0, false },
  { Event::LauncherMode,        "Enter ROM launcher",          0, false },
  { Event::Quit,                "Quit",                        0, false },

  { Event::JoystickZeroUp,      "P0 Joystick Up",              0, true  },
  { Event::JoystickZeroDown,    "P0 Joystick Down",            0, true  },
  { Event::JoystickZeroLeft,    "P0 Joystick Left",            0, true  },
  { Event::JoystickZeroRight,   "P0 Joystick Right",           0, true  },
  { Event::JoystickZeroFire,    "P0 Joystick Fire",            0, true  },
  { Event::JoystickZeroFire5,   "P0 Booster Top Trigger",      0, true  },
  { Event::JoystickZeroFire9,   "P0 Booster Handle Grip",      0, true  },

  { Event::JoystickOneUp,       "P1 Joystick Up",              0, true  },
  { Event::JoystickOneDown,     "P1 Joystick Down",            0, true  },
  { Event::JoystickOneLeft,     "P1 Joystick Left",            0, true  },
  { Event::JoystickOneRight,    "P1 Joystick Right",           0, true  },
  { Event::JoystickOneFire,     "P1 Joystick Fire",            0, true  },
  { Event::JoystickOneFire5,    "P1 Booster Top Trigger",      0, true  },
  { Event::JoystickOneFire9,    "P1 Booster Handle Grip",      0, true  },

  { Event::PaddleZeroAnalog,    "Paddle 0 Analog",             0, true  },
  { Event::PaddleZeroDecrease,  "Paddle 0 Decrease",           0, true  },
  { Event::PaddleZeroIncrease,  "Paddle 0 Increase",           0, true  },
  { Event::PaddleZeroFire,      "Paddle 0 Fire",               0, true  },

  { Event::PaddleOneAnalog,     "Paddle 1 Analog",             0, true  },
  { Event::PaddleOneDecrease,   "Paddle 1 Decrease",           0, true  },
  { Event::PaddleOneIncrease,   "Paddle 1 Increase",           0, true  },
  { Event::PaddleOneFire,       "Paddle 1 Fire",               0, true  },

  { Event::PaddleTwoAnalog,     "Paddle 2 Analog",             0, true  },
  { Event::PaddleTwoDecrease,   "Paddle 2 Decrease",           0, true  },
  { Event::PaddleTwoIncrease,   "Paddle 2 Increase",           0, true  },
  { Event::PaddleTwoFire,       "Paddle 2 Fire",               0, true  },

  { Event::PaddleThreeAnalog,   "Paddle 3 Analog",             0, true  },
  { Event::PaddleThreeDecrease, "Paddle 3 Decrease",           0, true  },
  { Event::PaddleThreeIncrease, "Paddle 3 Increase",           0, true  },
  { Event::PaddleThreeFire,     "Paddle 3 Fire",               0, true  },

  { Event::KeyboardZero1,       "P0 Keyboard 1",               0, true  },
  { Event::KeyboardZero2,       "P0 Keyboard 2",               0, true  },
  { Event::KeyboardZero3,       "P0 Keyboard 3",               0, true  },
  { Event::KeyboardZero4,       "P0 Keyboard 4",               0, true  },
  { Event::KeyboardZero5,       "P0 Keyboard 5",               0, true  },
  { Event::KeyboardZero6,       "P0 Keyboard 6",               0, true  },
  { Event::KeyboardZero7,       "P0 Keyboard 7",               0, true  },
  { Event::KeyboardZero8,       "P0 Keyboard 8",               0, true  },
  { Event::KeyboardZero9,       "P0 Keyboard 9",               0, true  },
  { Event::KeyboardZeroStar,    "P0 Keyboard *",               0, true  },
  { Event::KeyboardZero0,       "P0 Keyboard 0",               0, true  },
  { Event::KeyboardZeroPound,   "P0 Keyboard #",               0, true  },

  { Event::KeyboardOne1,        "P1 Keyboard 1",               0, true  },
  { Event::KeyboardOne2,        "P1 Keyboard 2",               0, true  },
  { Event::KeyboardOne3,        "P1 Keyboard 3",               0, true  },
  { Event::KeyboardOne4,        "P1 Keyboard 4",               0, true  },
  { Event::KeyboardOne5,        "P1 Keyboard 5",               0, true  },
  { Event::KeyboardOne6,        "P1 Keyboard 6",               0, true  },
  { Event::KeyboardOne7,        "P1 Keyboard 7",               0, true  },
  { Event::KeyboardOne8,        "P1 Keyboard 8",               0, true  },
  { Event::KeyboardOne9,        "P1 Keyboard 9",               0, true  },
  { Event::KeyboardOneStar,     "P1 Keyboard *",               0, true  },
  { Event::KeyboardOne0,        "P1 Keyboard 0",               0, true  },
  { Event::KeyboardOnePound,    "P1 Keyboard #",               0, true  },

  { Event::Combo1,              "Combo 1",                     0, false },
  { Event::Combo2,              "Combo 2",                     0, false },
  { Event::Combo3,              "Combo 3",                     0, false },
  { Event::Combo4,              "Combo 4",                     0, false },
  { Event::Combo5,              "Combo 5",                     0, false },
  { Event::Combo6,              "Combo 6",                     0, false },
  { Event::Combo7,              "Combo 7",                     0, false },
  { Event::Combo8,              "Combo 8",                     0, false },
  { Event::Combo9,              "Combo 9",                     0, false },
  { Event::Combo10,             "Combo 10",                    0, false },
  { Event::Combo11,             "Combo 11",                    0, false },
  { Event::Combo12,             "Combo 12",                    0, false },
  { Event::Combo13,             "Combo 13",                    0, false },
  { Event::Combo14,             "Combo 14",                    0, false },
  { Event::Combo15,             "Combo 15",                    0, false },
  { Event::Combo16,             "Combo 16",                    0, false }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::ActionList EventHandler::ourMenuActionList[kMenuActionListSize] = {
  { Event::UIUp,        "Move Up",              0, false },
  { Event::UIDown,      "Move Down",            0, false },
  { Event::UILeft,      "Move Left",            0, false },
  { Event::UIRight,     "Move Right",           0, false },

  { Event::UIHome,      "Home",                 0, false },
  { Event::UIEnd,       "End",                  0, false },
  { Event::UIPgUp,      "Page Up",              0, false },
  { Event::UIPgDown,    "Page Down",            0, false },

  { Event::UIOK,        "OK",                   0, false },
  { Event::UICancel,    "Cancel",               0, false },
  { Event::UISelect,    "Select item",          0, false },

  { Event::UINavPrev,   "Previous object",      0, false },
  { Event::UINavNext,   "Next object",          0, false },

  { Event::UIPrevDir,   "Parent directory",     0, false }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the Stelladaptor to send absolute axis values
const Event::Type EventHandler::SA_Axis[2][2] = {
  { Event::SALeftAxis0Value,  Event::SALeftAxis1Value  },
  { Event::SARightAxis0Value, Event::SARightAxis1Value }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the Stelladaptor to map button presses to joystick or paddles
//  (driving controllers and boostergrip are considered the same as joysticks)
const Event::Type EventHandler::SA_Button[2][4] = {
  { Event::JoystickZeroFire,  Event::JoystickZeroFire9,
    Event::JoystickZeroFire5, Event::JoystickZeroFire9 },
  { Event::JoystickOneFire,   Event::JoystickOneFire9,
    Event::JoystickOneFire5,  Event::JoystickOneFire9  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the 2600-daptor to map button presses to keypad keys
const Event::Type EventHandler::SA_Key[2][12] = {
  { Event::KeyboardZero1,    Event::KeyboardZero2,  Event::KeyboardZero3,
    Event::KeyboardZero4,    Event::KeyboardZero5,  Event::KeyboardZero6,
    Event::KeyboardZero7,    Event::KeyboardZero8,  Event::KeyboardZero9,
    Event::KeyboardZeroStar, Event::KeyboardZero0,  Event::KeyboardZeroPound },
  { Event::KeyboardOne1,     Event::KeyboardOne2,   Event::KeyboardOne3,
    Event::KeyboardOne4,     Event::KeyboardOne5,   Event::KeyboardOne6,
    Event::KeyboardOne7,     Event::KeyboardOne8,   Event::KeyboardOne9,
    Event::KeyboardOneStar,  Event::KeyboardOne0,   Event::KeyboardOnePound  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::StellaJoystick::StellaJoystick()
  : type(JT_NONE),
    name("None"),
    stick(NULL),
    numAxes(0),
    numButtons(0),
    numHats(0),
    axisTable(NULL),
    btnTable(NULL),
    hatTable(NULL),
    axisLastValue(NULL)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::StellaJoystick::~StellaJoystick()
{
  delete[] axisTable;      axisTable = NULL;
  delete[] btnTable;       btnTable = NULL;
  delete[] hatTable;       hatTable = NULL;
  delete[] axisLastValue;  axisLastValue = NULL;
  if(stick)
    SDL_JoystickClose(stick);
  stick = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::StellaJoystick::setStick(int index)
{
  stick = SDL_JoystickOpen(index);
  if(stick)
  {
    // Dynamically create the various mapping arrays for this joystick,
    // based on its specific attributes
    numAxes    = SDL_JoystickNumAxes(stick);
    numButtons = SDL_JoystickNumButtons(stick);
    numHats    = SDL_JoystickNumHats(stick);
    axisTable = new Event::Type[numAxes][2][kNumModes];
    btnTable  = new Event::Type[numButtons][kNumModes];
    hatTable  = new Event::Type[numHats][4][kNumModes];
    axisLastValue = new int[numAxes];

    // Erase the joystick axis mapping array and last axis value
    for(int a = 0; a < numAxes; ++a)
    {
      axisLastValue[a] = 0;
      for(int m = 0; m < kNumModes; ++m)
        axisTable[a][0][m] = axisTable[a][1][m] = Event::NoType;
    }

    // Erase the joystick button mapping array
    for(int b = 0; b < numButtons; ++b)
      for(int m = 0; m < kNumModes; ++m)
        btnTable[b][m] = Event::NoType;

    // Erase the joystick hat mapping array
    for(int h = 0; h < numHats; ++h)
      for(int m = 0; m < kNumModes; ++m)
        hatTable[h][0][m] = hatTable[h][1][m] =
        hatTable[h][2][m] = hatTable[h][3][m] = Event::NoType;

    name = SDL_JoystickName(index);
  }
  return name;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::StellaJoystick::getMap() const
{
  // The mapping structure (for remappable devices) is defined as follows:
  // NAME | AXIS # + values | BUTTON # + values | HAT # + values,
  // where each subsection of values is separated by ':'
  if(type == JT_REGULAR)
  {
    ostringstream joybuf;
    joybuf << name << "|" << numAxes;
    for(int m = 0; m < kNumModes; ++m)
      for(int a = 0; a < numAxes; ++a)
        for(int k = 0; k < 2; ++k)
          joybuf << " " << axisTable[a][k][m];
    joybuf << "|" << numButtons;
    for(int m = 0; m < kNumModes; ++m)
      for(int b = 0; b < numButtons; ++b)
        joybuf << " " << btnTable[b][m];
    joybuf << "|" << numHats;
    for(int m = 0; m < kNumModes; ++m)
      for(int h = 0; h < numHats; ++h)
        for(int k = 0; k < 4; ++k)
          joybuf << " " << hatTable[h][k][m];

    return joybuf.str();
  }
  return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::StellaJoystick::setMap(const string& m)
{
  istringstream buf(m);
  StringList items;
  string item;
  while(getline(buf, item, '|'))
    items.push_back(item);

  // Error checking
  if(items.size() != 4)
    return false;

  IntArray map;

  // Parse axis/button/hat values
  getValues(items[1], map);
  if((int)map.size() == numAxes * 2 * kNumModes)
  {
    // Fill the axes table with events
    IntArray::const_iterator event = map.begin();
    for(int m = 0; m < kNumModes; ++m)
      for(int a = 0; a < numAxes; ++a)
        for(int k = 0; k < 2; ++k)
          axisTable[a][k][m] = (Event::Type) *event++;
  }
  getValues(items[2], map);
  if((int)map.size() == numButtons * kNumModes)
  {
    IntArray::const_iterator event = map.begin();
    for(int m = 0; m < kNumModes; ++m)
      for(int b = 0; b < numButtons; ++b)
        btnTable[b][m] = (Event::Type) *event++;
  }
  getValues(items[3], map);
  if((int)map.size() == numHats * 4 * kNumModes)
  {
    IntArray::const_iterator event = map.begin();
    for(int m = 0; m < kNumModes; ++m)
      for(int h = 0; h < numHats; ++h)
        for(int k = 0; k < 4; ++k)
          hatTable[h][k][m] = (Event::Type) *event++;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::StellaJoystick::eraseMap(EventMode mode)
{
  // Erase axis mappings
  for(int a = 0; a < numAxes; ++a)
    axisTable[a][0][mode] = axisTable[a][1][mode] = Event::NoType;

  // Erase button mappings
  for(int b = 0; b < numButtons; ++b)
    btnTable[b][mode] = Event::NoType;

  // Erase hat mappings
  for(int h = 0; h < numHats; ++h)
    hatTable[h][0][mode] = hatTable[h][1][mode] =
    hatTable[h][2][mode] = hatTable[h][3][mode] = Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::StellaJoystick::eraseEvent(Event::Type event, EventMode mode)
{
  // Erase axis mappings
  for(int a = 0; a < numAxes; ++a)
  {
    if(axisTable[a][0][mode] == event)  axisTable[a][0][mode] = Event::NoType;
    if(axisTable[a][1][mode] == event)  axisTable[a][1][mode] = Event::NoType;
  }

  // Erase button mappings
  for(int b = 0; b < numButtons; ++b)
    if(btnTable[b][mode] == event)  btnTable[b][mode] = Event::NoType;

  // Erase hat mappings
  for(int h = 0; h < numHats; ++h)
  {
    if(hatTable[h][0][mode] == event)  hatTable[h][0][mode] = Event::NoType;
    if(hatTable[h][1][mode] == event)  hatTable[h][1][mode] = Event::NoType;
    if(hatTable[h][2][mode] == event)  hatTable[h][2][mode] = Event::NoType;
    if(hatTable[h][3][mode] == event)  hatTable[h][3][mode] = Event::NoType;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::StellaJoystick::getValues(const string& list, IntArray& map)
{
  map.clear();
  istringstream buf(list);

  int value;
  buf >> value;  // we don't need to know the # of items at this point
  while(buf >> value)
    map.push_back(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::StellaJoystick::about() const
{
  ostringstream buf;
  buf << name;
  if(type == JT_REGULAR)
    buf << " with:" << endl << "     "
        << numAxes    << " axes, "
        << numButtons << " buttons, "
        << numHats    << " hats";

  return buf.str();
}
