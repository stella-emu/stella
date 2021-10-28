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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "PaletteHandler.hxx"
#include "QuadTari.hxx"
#include "Sound.hxx"
#include "StateManager.hxx"
#include "TIASurface.hxx"

#include "GlobalKeyHandler.hxx"

using namespace std::placeholders;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GlobalKeyHandler::GlobalKeyHandler(OSystem& osystem)
  : myOSystem{osystem}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::handleEvent(const Event::Type event, bool pressed, bool repeated)
{
  // The global settings keys change settings or values as long as the setting
  //  message from the previous settings event is still displayed.
  // Therefore, do not change global settings/values or direct values if
  //  a) the setting message is no longer shown
  //  b) other keys have been pressed
  if(!myOSystem.frameBuffer().messageShown())
  {
    myAdjustActive = false;
    myAdjustDirect = AdjustSetting::NONE;
  }

  const bool adjustActive = myAdjustActive;
  const AdjustSetting adjustAVDirect = myAdjustDirect;

  if(pressed)
  {
    myAdjustActive = false;
    myAdjustDirect = AdjustSetting::NONE;
  }

  bool handled = true;

  switch(event)
  {
    ////////////////////////////////////////////////////////////////////////
    // Allow adjusting several (mostly repeated) settings using the same six hotkeys
    case Event::PreviousSettingGroup:
    case Event::NextSettingGroup:
      if(pressed && !repeated)
      {
        const int direction = event == Event::PreviousSettingGroup ? -1 : +1;
        AdjustGroup adjustGroup = AdjustGroup(BSPF::clampw(int(getAdjustGroup()) + direction,
                                              0, int(AdjustGroup::NUM_GROUPS) - 1));
        string msg;

        switch(adjustGroup)
        {
          case AdjustGroup::AV:
            msg = "Audio & Video";
            myAdjustSetting = AdjustSetting::START_AV_ADJ;
            break;

          case AdjustGroup::INPUT:
            msg = "Input Devices & Ports";
            myAdjustSetting = AdjustSetting::START_INPUT_ADJ;
            break;

          case AdjustGroup::DEBUG:
            msg = "Debug";
            myAdjustSetting = AdjustSetting::START_DEBUG_ADJ;
            break;

          default:
            break;
        }
        myOSystem.frameBuffer().showTextMessage(msg + " settings");
        myAdjustActive = false;
      }
      break;

      // Allow adjusting several (mostly repeated) settings using the same four hotkeys
    case Event::PreviousSetting:
    case Event::NextSetting:
      if(pressed && !repeated)
      {
        const int direction = event == Event::PreviousSetting ? -1 : +1;

        // Get (and display) the previous|next adjustment function,
        //  but do not change its value
        cycleAdjustSetting(adjustActive ? direction : 0)(0);
        // Fallback message when no message is displayed by method
        //if(!myOSystem.frameBuffer().messageShown())
        //  myOSystem.frameBuffer().showMessage("Message " + std::to_string(int(myAdjustSetting)));
        myAdjustActive = true;
      }
      break;

    case Event::SettingDecrease:
    case Event::SettingIncrease:
      if(pressed)
      {
        const int direction = event == Event::SettingDecrease ? -1 : +1;

        // if a "direct only" hotkey was pressed last, use this one
        if(adjustAVDirect != AdjustSetting::NONE)
        {
          myAdjustDirect = adjustAVDirect;
          if(!repeated || isAdjustRepeated(myAdjustDirect))
            getAdjustSetting(myAdjustDirect)(direction);
        }
        else
        {
          // Get (and display) the current adjustment function,
          //  but only change its value if the function was already active before
          if(!repeated || isAdjustRepeated(myAdjustSetting))
          {
            getAdjustSetting(myAdjustSetting)(adjustActive ? direction : 0);
            myAdjustActive = true;
          }
        }
      }
      break;

    default:
      handled = false;
  }
  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalKeyHandler::setAdjustSetting(const AdjustSetting setting)
{
  myAdjustSetting = setting;
  myAdjustActive = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalKeyHandler::setAdjustDirect(const AdjustSetting setting)
{
  myAdjustDirect = setting;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const GlobalKeyHandler::AdjustGroup GlobalKeyHandler::getAdjustGroup() const
{
  if(myAdjustSetting >= AdjustSetting::START_DEBUG_ADJ && myAdjustSetting <= AdjustSetting::END_DEBUG_ADJ)
    return AdjustGroup::DEBUG;
  if(myAdjustSetting >= AdjustSetting::START_INPUT_ADJ && myAdjustSetting <= AdjustSetting::END_INPUT_ADJ)
    return AdjustGroup::INPUT;

  return AdjustGroup::AV;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::isJoystick(const Controller& controller) const
{
  return controller.type() == Controller::Type::Joystick
    || controller.type() == Controller::Type::BoosterGrip
    || controller.type() == Controller::Type::Genesis
    || (controller.type() == Controller::Type::QuadTari
      && (isJoystick(static_cast<const QuadTari*>(&controller)->firstController())
      || isJoystick(static_cast<const QuadTari*>(&controller)->secondController())));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::isPaddle(const Controller& controller) const
{
  return controller.type() == Controller::Type::Paddles
    || controller.type() == Controller::Type::PaddlesIAxDr
    || controller.type() == Controller::Type::PaddlesIAxis
    || (controller.type() == Controller::Type::QuadTari
      && (isPaddle(static_cast<const QuadTari*>(&controller)->firstController())
      || isPaddle(static_cast<const QuadTari*>(&controller)->secondController())));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::isTrackball(const Controller& controller) const
{
  return controller.type() == Controller::Type::AmigaMouse
    || controller.type() == Controller::Type::AtariMouse
    || controller.type() == Controller::Type::TrakBall;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::skipAVSetting() const
{
  const bool isFullScreen = myOSystem.frameBuffer().fullScreen();
  const bool isCustomPalette =
    myOSystem.settings().getString("palette") == PaletteHandler::SETTING_CUSTOM;
  const bool isCustomFilter =
    myOSystem.settings().getInt("tv.filter") == int(NTSCFilter::Preset::CUSTOM);
  const bool isSoftwareRenderer =
    myOSystem.settings().getString("video") == "software";

  return (myAdjustSetting == AdjustSetting::OVERSCAN && !isFullScreen)
  #ifdef ADAPTABLE_REFRESH_SUPPORT
    || (myAdjustSetting == AdjustSetting::ADAPT_REFRESH && !isFullScreen)
  #endif
    || (myAdjustSetting >= AdjustSetting::PALETTE_PHASE
      && myAdjustSetting <= AdjustSetting::PALETTE_BLUE_SHIFT
      && !isCustomPalette)
    || (myAdjustSetting >= AdjustSetting::NTSC_SHARPNESS
      && myAdjustSetting <= AdjustSetting::NTSC_BLEEDING
      && !isCustomFilter)
    || (myAdjustSetting == AdjustSetting::INTERPOLATION && isSoftwareRenderer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::skipInputSetting() const
{
  const bool grabMouseAllowed = myOSystem.frameBuffer().grabMouseAllowed();
  const bool analog = myOSystem.console().leftController().isAnalog()
    || myOSystem.console().rightController().isAnalog();
  const bool joystick = isJoystick(myOSystem.console().leftController())
    || isJoystick(myOSystem.console().rightController());
  const bool paddle = isPaddle(myOSystem.console().leftController())
    || isPaddle(myOSystem.console().rightController());
  const bool trackball = isTrackball(myOSystem.console().leftController())
    || isTrackball(myOSystem.console().rightController());
  const bool driving =
    myOSystem.console().leftController().type() == Controller::Type::Driving
    || myOSystem.console().rightController().type() == Controller::Type::Driving;
  const bool useMouse =
    BSPF::equalsIgnoreCase("always", myOSystem.settings().getString("usemouse"))
    || (BSPF::equalsIgnoreCase("analog", myOSystem.settings().getString("usemouse"))
      && analog);
  const bool stelladapter = joyHandler().hasStelladaptors();

  return (!grabMouseAllowed && myAdjustSetting == AdjustSetting::GRAB_MOUSE)
    || (!joystick
      && (myAdjustSetting == AdjustSetting::DEADZONE
      || myAdjustSetting == AdjustSetting::FOUR_DIRECTIONS))
    || (!paddle
      && (myAdjustSetting == AdjustSetting::ANALOG_DEADZONE
      || myAdjustSetting == AdjustSetting::ANALOG_SENSITIVITY
      || myAdjustSetting == AdjustSetting::ANALOG_LINEARITY
      || myAdjustSetting == AdjustSetting::DEJITTER_AVERAGING
      || myAdjustSetting == AdjustSetting::DEJITTER_REACTION
      || myAdjustSetting == AdjustSetting::DIGITAL_SENSITIVITY
      || myAdjustSetting == AdjustSetting::SWAP_PADDLES
      || myAdjustSetting == AdjustSetting::PADDLE_CENTER_X
      || myAdjustSetting == AdjustSetting::PADDLE_CENTER_Y))
    || ((!paddle || !useMouse)
      && myAdjustSetting == AdjustSetting::PADDLE_SENSITIVITY)
    || ((!trackball || !useMouse)
      && myAdjustSetting == AdjustSetting::TRACKBALL_SENSITIVITY)
    || (!driving
      && myAdjustSetting == AdjustSetting::DRIVING_SENSITIVITY) // also affects digital device input sensitivity
    || ((!myOSystem.eventHandler().hasMouseControl() || !useMouse)
      && myAdjustSetting == AdjustSetting::MOUSE_CONTROL)
    || ((!paddle || !useMouse)
      && myAdjustSetting == AdjustSetting::MOUSE_RANGE)
    || (!stelladapter
      && myAdjustSetting == AdjustSetting::SA_PORT_ORDER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::skipDebugSetting() const
{
  const bool isPAL = myOSystem.console().timing() == ConsoleTiming::pal;

  return (myAdjustSetting == AdjustSetting::COLOR_LOSS && !isPAL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AdjustFunction GlobalKeyHandler::cycleAdjustSetting(int direction)
{
  bool skip = false;
  do
  {
    switch(getAdjustGroup())
    {
      case AdjustGroup::AV:
        myAdjustSetting =
          AdjustSetting(BSPF::clampw(int(myAdjustSetting) + direction,
            int(AdjustSetting::START_AV_ADJ), int(AdjustSetting::END_AV_ADJ)));
        // skip currently non-relevant adjustments
        skip = skipAVSetting();
        break;

      case AdjustGroup::INPUT:
        myAdjustSetting =
          AdjustSetting(BSPF::clampw(int(myAdjustSetting) + direction,
            int(AdjustSetting::START_INPUT_ADJ), int(AdjustSetting::END_INPUT_ADJ)));
        // skip currently non-relevant adjustments
        skip = skipInputSetting();
        break;

      case AdjustGroup::DEBUG:
        myAdjustSetting =
          AdjustSetting(BSPF::clampw(int(myAdjustSetting) + direction,
            int(AdjustSetting::START_DEBUG_ADJ), int(AdjustSetting::END_DEBUG_ADJ)));
        // skip currently non-relevant adjustments
        skip = skipDebugSetting();
        break;

      default:
        break;
    }
    // avoid endless loop
    if(skip && !direction)
      direction = 1;
  } while(skip);

  return getAdjustSetting(myAdjustSetting);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AdjustFunction GlobalKeyHandler::getAdjustSetting(const AdjustSetting setting) const
{
  // Notes:
  // - All methods MUST show a message
  // - This array MUST have the same order as AdjustSetting
  const AdjustFunction ADJUST_FUNCTIONS[int(AdjustSetting::NUM_ADJ)] =
  {
    // *** Audio & Video settings ***
    std::bind(&Sound::adjustVolume, &myOSystem.sound(), _1),
    std::bind(&FrameBuffer::switchVideoMode, &myOSystem.frameBuffer(), _1),
    std::bind(&FrameBuffer::toggleFullscreen, &myOSystem.frameBuffer(), _1),
  #ifdef ADAPTABLE_REFRESH_SUPPORT
    std::bind(&FrameBuffer::toggleAdaptRefresh, &myOSystem.frameBuffer(), _1),
  #endif
    std::bind(&FrameBuffer::changeOverscan, &myOSystem.frameBuffer(), _1),
    std::bind(&Console::selectFormat, &myOSystem.console(), _1), // property, not persisted
    std::bind(&Console::changeVerticalCenter, &myOSystem.console(), _1), // property, not persisted
    std::bind(&Console::toggleCorrectAspectRatio, &myOSystem.console(), _1),
    std::bind(&Console::changeVSizeAdjust, &myOSystem.console(), _1),
    // Palette adjustables
    std::bind(&PaletteHandler::cyclePalette, &myOSystem.frameBuffer().tiaSurface().paletteHandler(), _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::PHASE_SHIFT, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::RED_SCALE, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::RED_SHIFT, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::GREEN_SCALE, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::GREEN_SHIFT, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::BLUE_SCALE, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::BLUE_SHIFT, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::HUE, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::SATURATION, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::CONTRAST, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::BRIGHTNESS, _1),
    std::bind(&PaletteHandler::changeAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(),
      PaletteHandler::GAMMA, _1),
    // NTSC filter adjustables
    std::bind(&TIASurface::changeNTSC, &myOSystem.frameBuffer().tiaSurface(), _1),
    std::bind(&TIASurface::changeNTSCAdjustable, &myOSystem.frameBuffer().tiaSurface(),
      int(NTSCFilter::Adjustables::SHARPNESS), _1),
    std::bind(&TIASurface::changeNTSCAdjustable, &myOSystem.frameBuffer().tiaSurface(),
      int(NTSCFilter::Adjustables::RESOLUTION), _1),
    std::bind(&TIASurface::changeNTSCAdjustable, &myOSystem.frameBuffer().tiaSurface(),
      int(NTSCFilter::Adjustables::ARTIFACTS), _1),
    std::bind(&TIASurface::changeNTSCAdjustable, &myOSystem.frameBuffer().tiaSurface(),
      int(NTSCFilter::Adjustables::FRINGING), _1),
    std::bind(&TIASurface::changeNTSCAdjustable, &myOSystem.frameBuffer().tiaSurface(),
      int(NTSCFilter::Adjustables::BLEEDING), _1),
    std::bind(&Console::changePhosphor, &myOSystem.console(), _1),
    std::bind(&TIASurface::setScanlineIntensity, &myOSystem.frameBuffer().tiaSurface(), _1),
    std::bind(&Console::toggleInter, &myOSystem.console(), _1),

    // *** Input settings ***
    std::bind(&PhysicalJoystickHandler::changeDigitalDeadZone, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changeAnalogPaddleDeadZone, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changeAnalogPaddleSensitivity, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changeAnalogPaddleLinearity, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changePaddleDejitterAveraging, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changePaddleDejitterReaction, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changeDigitalPaddleSensitivity, &joyHandler(), _1),
    std::bind(&Console::changeAutoFireRate, &myOSystem.console(), _1),
    std::bind(&EventHandler::toggleAllow4JoyDirections, &myOSystem.eventHandler(), _1),
    std::bind(&PhysicalKeyboardHandler::toggleModKeys, &keyHandler(), _1),
    std::bind(&EventHandler::toggleSAPortOrder, &myOSystem.eventHandler(), _1),
    std::bind(&EventHandler::changeMouseControllerMode, &myOSystem.eventHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changeMousePaddleSensitivity, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changeMouseTrackballSensitivity, &joyHandler(), _1),
    std::bind(&PhysicalJoystickHandler::changeDrivingSensitivity, &joyHandler(), _1),
    std::bind(&EventHandler::changeMouseCursor, &myOSystem.eventHandler(), _1),
    std::bind(&FrameBuffer::toggleGrabMouse, &myOSystem.frameBuffer(), _1),
    // Game properties/Controllers
    std::bind(&Console::changeLeftController, &myOSystem.console(), _1), // property, not persisted
    std::bind(&Console::changeRightController, &myOSystem.console(), _1), // property, not persisted
    std::bind(&Console::toggleSwapPorts, &myOSystem.console(), _1), // property, not persisted
    std::bind(&Console::toggleSwapPaddles, &myOSystem.console(), _1), // property, not persisted
    std::bind(&Console::changePaddleCenterX, &myOSystem.console(), _1), // property, not persisted
    std::bind(&Console::changePaddleCenterY, &myOSystem.console(), _1), // property, not persisted
    std::bind(&EventHandler::changeMouseControl, &myOSystem.eventHandler(), _1), // property, not persisted
    std::bind(&Console::changePaddleAxesRange, &myOSystem.console(), _1), // property, not persisted

    // *** Debug settings ***
    std::bind(&FrameBuffer::toggleFrameStats, &myOSystem.frameBuffer(), _1),
    std::bind(&Console::toggleP0Bit, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleP1Bit, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleM0Bit, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleM1Bit, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleBLBit, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::togglePFBit, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleBits, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleP0Collision, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleP1Collision, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleM0Collision, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleM1Collision, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleBLCollision, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::togglePFCollision, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleCollisions, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleFixedColors, &myOSystem.console(), _1), // debug, not persisted
    std::bind(&Console::toggleColorLoss, &myOSystem.console(), _1),
    std::bind(&Console::toggleJitter, &myOSystem.console(), _1),

    // *** Following functions are not used when cycling settings, but for "direct only" hotkeys ***
    std::bind(&StateManager::changeState, &myOSystem.state(), _1), // temporary, not persisted
    std::bind(&PaletteHandler::changeCurrentAdjustable, &myOSystem.frameBuffer().tiaSurface().paletteHandler(), _1),
    std::bind(&TIASurface::changeCurrentNTSCAdjustable, &myOSystem.frameBuffer().tiaSurface(), _1),
    std::bind(&Console::changeSpeed, &myOSystem.console(), _1),
  };

  return ADJUST_FUNCTIONS[int(setting)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::isAdjustRepeated(const AdjustSetting setting) const
{
  const bool ADJUST_REPEATED[int(AdjustSetting::NUM_ADJ)] =
  {
    // *** Audio & Video group ***
    true,   // VOLUME
    false,  // ZOOM (always repeating)
    false,  // FULLSCREEN (always repeating)
  #ifdef ADAPTABLE_REFRESH_SUPPORT
    false,  // ADAPT_REFRESH (always repeating)
  #endif
    true,   // OVERSCAN
    false,  // TVFORMAT
    true,   // VCENTER
    false,  // ASPECT_RATIO (always repeating)
    true,   // VSIZE
    // Palette adjustables
    false,  // PALETTE
    true,   // PALETTE_PHASE
    true,   // PALETTE_RED_SCALE
    true,   // PALETTE_RED_SHIFT
    true,   // PALETTE_GREEN_SCALE
    true,   // PALETTE_GREEN_SHIFT
    true,   // PALETTE_BLUE_SCALE
    true,   // PALETTE_BLUE_SHIFT
    true,   // PALETTE_HUE
    true,   // PALETTE_SATURATION
    true,   // PALETTE_CONTRAST
    true,   // PALETTE_BRIGHTNESS
    true,   // PALETTE_GAMMA
    // NTSC filter adjustables
    false,  // NTSC_PRESET
    true,   // NTSC_SHARPNESS
    true,   // NTSC_RESOLUTION
    true,   // NTSC_ARTIFACTS
    true,   // NTSC_FRINGING
    true,   // NTSC_BLEEDING
    // Other TV effects adjustables
    true,   // PHOSPHOR
    true,   // SCANLINES
    false,  // INTERPOLATION
    // *** Input group ***
    true,   // DEADZONE
    true,   // ANALOG_DEADZONE
    true,   // ANALOG_SENSITIVITY
    true,   // ANALOG_LINEARITY
    true,   // DEJITTER_AVERAGING
    true,   // DEJITTER_REACTION
    true,   // DIGITAL_SENSITIVITY
    true,   // AUTO_FIRE
    false,  // FOUR_DIRECTIONS
    false,  // MOD_KEY_COMBOS
    false,  // SA_PORT_ORDER
    false,  // USE_MOUSE
    true,   // PADDLE_SENSITIVITY
    true,   // TRACKBALL_SENSITIVITY
    true,   // DRIVING_SENSITIVITY
    false,  // MOUSE_CURSOR
    false,  // GRAB_MOUSE
    false,  // LEFT_PORT
    false,  // RIGHT_PORT
    false,  // SWAP_PORTS
    false,  // SWAP_PADDLES
    true,   // PADDLE_CENTER_X
    true,   // PADDLE_CENTER_Y
    false,  // MOUSE_CONTROL
    true,   // MOUSE_RANGE
    // *** Debug group ***
    false,  // STATS
    false,  // P0_ENAM
    false,  // P1_ENAM
    false,  // M0_ENAM
    false,  // M1_ENAM
    false,  // BL_ENAM
    false,  // PF_ENAM
    false,  // ALL_ENAM
    false,  // P0_CX
    false,  // P1_CX
    false,  // M0_CX
    false,  // M1_CX
    false,  // BL_CX
    false,  // PF_CX
    false,  // ALL_CX
    false,  // FIXED_COL
    false,  // COLOR_LOSS
    false,  // JITTER
    // *** Only used via direct hotkeys ***
    true,   // STATE
    true,   // PALETTE_CHANGE_ATTRIBUTE
    true,   // NTSC_CHANGE_ATTRIBUTE
    true,   // CHANGE_SPEED
  };

  return ADJUST_REPEATED[int(setting)];
}
