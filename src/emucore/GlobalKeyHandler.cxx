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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GlobalKeyHandler::GlobalKeyHandler(OSystem& osystem)
  : myOSystem{osystem}
{
  buildSettingMap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::handleEvent(Event::Type event, bool pressed, bool repeated)
{
  // The global settings keys change settings or values as long as the setting
  //  message from the previous settings event is still displayed.
  // Therefore, do not change global settings/values or direct values if
  //  a) the setting message is no longer shown
  //  b) other keys have been pressed
  if(!myOSystem.frameBuffer().messageShown())
  {
    mySettingActive = false;
    myDirectSetting = Setting::NONE;
  }

  const bool settingActive = mySettingActive;
  const Setting directSetting = myDirectSetting;

  if(pressed)
  {
    mySettingActive = false;
    myDirectSetting = Setting::NONE;
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
        const int direction = (event == Event::PreviousSettingGroup ? -1 : +1);
        const auto group = static_cast<Group>(
            BSPF::clampw(static_cast<int>(getGroup()) + direction,
            0, static_cast<int>(Group::NUM_GROUPS) - 1));
        static constexpr std::array<std::pair<Group, GroupData>, 3> GroupMap = {{
          {Group::AV,    {Setting::START_AV_ADJ,    "Audio & Video"}},
          {Group::INPUT, {Setting::START_INPUT_ADJ, "Input Devices & Ports"}},
          {Group::DEBUG, {Setting::START_DEBUG_ADJ, "Debug"}},
        }};
        const auto result = std::ranges::find_if(GroupMap,
            [group](const auto& entry) { return entry.first == group; });

        myOSystem.frameBuffer().showTextMessage(
            std::format("{} settings", result->second.name));
        mySetting = result->second.start;
        mySettingActive = false;
      }
      break;

    case Event::PreviousSetting:
    case Event::NextSetting:
      if(pressed && !repeated)
      {
        const int direction = (event == Event::PreviousSetting ? -1 : +1);

        // Get (and display) the previous|next adjustment function,
        //  but do not change its value
        cycleSetting(settingActive ? direction : 0)(0);
        // Fallback message when no message is displayed by method
        //if(!myOSystem.frameBuffer().messageShown())
        //  myOSystem.frameBuffer().showMessage("Message " + std::to_string(int(mySetting)));
        mySettingActive = true;
      }
      break;

    case Event::SettingDecrease:
    case Event::SettingIncrease:
      if(pressed)
      {
        const int direction = (event == Event::SettingDecrease ? -1 : +1);

        // if a "direct only" hotkey was pressed last, use this one
        if(directSetting != Setting::NONE)
        {
          const SettingData& data = getSettingData(directSetting);

          myDirectSetting = directSetting;
          if(!repeated || data.repeated)
            data.function(direction);
        }
        else
        {
          // Get (and display) the current adjustment function,
          //  but only change its value if the function was already active before
          const SettingData& data = getSettingData(mySetting);

          if(!repeated || data.repeated)
          {
            data.function(settingActive ? direction : 0);
            mySettingActive = true;
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
void GlobalKeyHandler::setSetting(Setting setting)
{
  if(setting == Setting::ZOOM && myOSystem.frameBuffer().fullScreen())
    mySetting = Setting::FS_ASPECT;
  else
    mySetting = setting;
  mySettingActive = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalKeyHandler::setDirectSetting(Setting setting)
{
  myDirectSetting = setting;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GlobalKeyHandler::Group GlobalKeyHandler::getGroup() const
{
  if(mySetting >= Setting::START_DEBUG_ADJ && mySetting <= Setting::END_DEBUG_ADJ)
    return Group::DEBUG;

  if(mySetting >= Setting::START_INPUT_ADJ && mySetting <= Setting::END_INPUT_ADJ)
    return Group::INPUT;

  return Group::AV;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::isJoystick(const Controller& controller)
{
  return controller.type() == Controller::Type::Joystick
    || controller.type() == Controller::Type::BoosterGrip
    || controller.type() == Controller::Type::Genesis
    || controller.type() == Controller::Type::Joy2BPlus
    || (controller.type() == Controller::Type::QuadTari
      && (isJoystick(static_cast<const QuadTari*>(&controller)->firstController())
      || isJoystick(static_cast<const QuadTari*>(&controller)->secondController())));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::isPaddle(const Controller& controller)
{
  return controller.type() == Controller::Type::Paddles
    || controller.type() == Controller::Type::PaddlesIAxDr
    || controller.type() == Controller::Type::PaddlesIAxis
    || (controller.type() == Controller::Type::QuadTari
      && (isPaddle(static_cast<const QuadTari*>(&controller)->firstController())
      || isPaddle(static_cast<const QuadTari*>(&controller)->secondController())));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::isTrackball(const Controller& controller)
{
  return controller.type() == Controller::Type::AmigaMouse
    || controller.type() == Controller::Type::AtariMouse
    || controller.type() == Controller::Type::TrakBall;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::skipAVSetting() const
{
  const bool isFullScreen = myOSystem.frameBuffer().fullScreen();
  const bool isFsStretch = isFullScreen &&
    myOSystem.settings().getBool("tia.fs_stretch");
  const bool isCustomPalette =
    myOSystem.settings().getString("palette") == PaletteHandler::SETTING_CUSTOM;
  const bool isCustomFilter =
    myOSystem.settings().getInt("tv.filter") == static_cast<int>(NTSCFilter::Preset::CUSTOM);
  const bool hasScanlines =
    myOSystem.settings().getInt("tv.scanlines") > 0;
  const bool isSoftwareRenderer =
    myOSystem.settings().getString("video") == "software";
  const bool allowBezel =
    myOSystem.settings().getBool("bezel.windowed") || isFullScreen;

  return (mySetting == Setting::OVERSCAN && !isFullScreen)
    || (mySetting == Setting::ZOOM && isFullScreen)
#ifdef ADAPTABLE_REFRESH_SUPPORT
    || (mySetting == Setting::ADAPT_REFRESH && !isFullScreen)
#endif
    || (mySetting == Setting::FS_ASPECT && !isFullScreen)
    || (mySetting == Setting::ASPECT_RATIO && isFsStretch)
    || (mySetting >= Setting::PALETTE_PHASE
      && mySetting <= Setting::PALETTE_BLUE_SHIFT
      && !isCustomPalette)
    || (mySetting >= Setting::NTSC_SHARPNESS
      && mySetting <= Setting::NTSC_BLEEDING
      && !isCustomFilter)
    || (mySetting == Setting::SCANLINE_MASK && !hasScanlines)
    || (mySetting == Setting::INTERPOLATION && isSoftwareRenderer)
    || (mySetting == Setting::BEZEL && !allowBezel);
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

  return (!grabMouseAllowed && mySetting == Setting::GRAB_MOUSE)
    || (!joystick
      && (mySetting == Setting::DIGITAL_DEADZONE
      || mySetting == Setting::FOUR_DIRECTIONS))
    || (!paddle
      && (mySetting == Setting::ANALOG_DEADZONE
      || mySetting == Setting::ANALOG_SENSITIVITY
      || mySetting == Setting::ANALOG_LINEARITY
      || mySetting == Setting::DEJITTER_AVERAGING
      || mySetting == Setting::DEJITTER_REACTION
      || mySetting == Setting::DIGITAL_SENSITIVITY
      || mySetting == Setting::SWAP_PADDLES
      || mySetting == Setting::PADDLE_CENTER_X
      || mySetting == Setting::PADDLE_CENTER_Y))
    || ((!paddle || !useMouse)
      && mySetting == Setting::PADDLE_SENSITIVITY)
    || ((!trackball || !useMouse)
      && mySetting == Setting::TRACKBALL_SENSITIVITY)
    || (!driving
      && mySetting == Setting::DRIVING_SENSITIVITY) // also affects digital device input sensitivity
    || ((!myOSystem.eventHandler().hasMouseControl() || !useMouse)
      && mySetting == Setting::MOUSE_CONTROL)
    || ((!paddle || !useMouse)
      && mySetting == Setting::MOUSE_RANGE)
    || (!stelladapter
      && mySetting == Setting::SA_PORT_ORDER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GlobalKeyHandler::skipDebugSetting() const
{
  const bool isPAL = myOSystem.console().timing() == ConsoleTiming::pal;

  return (mySetting == Setting::COLOR_LOSS && !isPAL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GlobalKeyHandler::Function GlobalKeyHandler::cycleSetting(int direction)
{
  bool skip = false;

  do
  {
    switch(getGroup())
    {
      case Group::AV:
        mySetting = static_cast<Setting>(
            BSPF::clampw(static_cast<int>(mySetting) + direction,
            static_cast<int>(Setting::START_AV_ADJ), static_cast<int>(Setting::END_AV_ADJ)));
        // skip currently non-relevant adjustments
        skip = skipAVSetting();
        break;

      case Group::INPUT:
        mySetting = static_cast<Setting>(
            BSPF::clampw(static_cast<int>(mySetting) + direction,
            static_cast<int>(Setting::START_INPUT_ADJ), static_cast<int>(Setting::END_INPUT_ADJ)));
        // skip currently non-relevant adjustments
        skip = skipInputSetting();
        break;

      case Group::DEBUG:
        mySetting = static_cast<Setting>(
            BSPF::clampw(static_cast<int>(mySetting) + direction,
            static_cast<int>(Setting::START_DEBUG_ADJ), static_cast<int>(Setting::END_DEBUG_ADJ)));
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

  return getSettingData(mySetting).function;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalKeyHandler::buildSettingMap()
{
  // Notes:
  // - all setting methods MUST always display a message
  // - some settings reset the repeat state, therefore the code cannot detect repeats
  mySettingMap = {
    // *** Audio & Video group ***
    {Setting::VOLUME,                 {true,  [this](int d) { myOSystem.sound().adjustVolume(d); }}},
    {Setting::ZOOM,                   {false, [this](int d) { myOSystem.frameBuffer().switchVideoMode(d); }}}, // always repeating
    {Setting::FULLSCREEN,             {false, [this](int d) { myOSystem.frameBuffer().toggleFullscreen(d); }}}, // always repeating
    {Setting::FS_ASPECT,              {false, [this](int d) { myOSystem.frameBuffer().switchVideoMode(d); }}}, // always repeating
  #ifdef ADAPTABLE_REFRESH_SUPPORT
    {Setting::ADAPT_REFRESH,          {false, [this](int d) { myOSystem.frameBuffer().toggleAdaptRefresh(d); }}}, // always repeating
  #endif
    {Setting::OVERSCAN,               {true,  [this](int d) { myOSystem.frameBuffer().changeOverscan(d); }}},
    {Setting::TVFORMAT,               {false, [this](int d) { myOSystem.console().selectFormat(d); }}}, // property, not persisted
    {Setting::VCENTER,                {true,  [this](int d) { myOSystem.console().changeVerticalCenter(d); }}}, // property, not persisted
    {Setting::ASPECT_RATIO,           {false, [this](int d) { myOSystem.console().toggleCorrectAspectRatio(d); }}}, // always repeating
    {Setting::VSIZE,                  {true,  [this](int d) { myOSystem.console().changeVSizeAdjust(d); }}},
    // Palette adjustables
    {Setting::PALETTE,                {false, [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().cyclePalette(d); }}},
    {Setting::PALETTE_PHASE,          {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeAdjustable(PaletteHandler::PHASE_SHIFT, d); }}},
    {Setting::PALETTE_RED_SCALE,      {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeAdjustable(PaletteHandler::RED_SCALE, d); }}},
    {Setting::PALETTE_RED_SHIFT,      {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeAdjustable(PaletteHandler::RED_SHIFT, d); }}},
    {Setting::PALETTE_GREEN_SCALE,    {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeAdjustable(PaletteHandler::GREEN_SCALE, d); }}},
    {Setting::PALETTE_GREEN_SHIFT,    {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeAdjustable(PaletteHandler::GREEN_SHIFT, d); }}},
    {Setting::PALETTE_BLUE_SCALE,     {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeAdjustable(PaletteHandler::BLUE_SCALE, d); }}},
    {Setting::PALETTE_BLUE_SHIFT,     {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeAdjustable(PaletteHandler::BLUE_SHIFT, d); }}},
    {Setting::PALETTE_HUE,            {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeAdjustable(PaletteHandler::HUE, d); }}},
    {Setting::PALETTE_SATURATION,     {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeAdjustable(PaletteHandler::SATURATION, d); }}},
    {Setting::PALETTE_CONTRAST,       {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeAdjustable(PaletteHandler::CONTRAST, d); }}},
    {Setting::PALETTE_BRIGHTNESS,     {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeAdjustable(PaletteHandler::BRIGHTNESS, d); }}},
    {Setting::PALETTE_GAMMA,          {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeAdjustable(PaletteHandler::GAMMA, d); }}},
    // NTSC filter adjustables
    {Setting::NTSC_PRESET,            {false, [this](int d) { myOSystem.frameBuffer().tiaSurface().changeNTSC(d); }}},
    {Setting::NTSC_SHARPNESS,         {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().changeNTSCAdjustable(static_cast<int>(NTSCFilter::Adjustables::SHARPNESS), d); }}},
    {Setting::NTSC_RESOLUTION,        {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().changeNTSCAdjustable(static_cast<int>(NTSCFilter::Adjustables::RESOLUTION), d); }}},
    {Setting::NTSC_ARTIFACTS,         {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().changeNTSCAdjustable(static_cast<int>(NTSCFilter::Adjustables::ARTIFACTS), d); }}},
    {Setting::NTSC_FRINGING,          {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().changeNTSCAdjustable(static_cast<int>(NTSCFilter::Adjustables::FRINGING), d); }}},
    {Setting::NTSC_BLEEDING,          {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().changeNTSCAdjustable(static_cast<int>(NTSCFilter::Adjustables::BLEEDING), d); }}},
    // Other TV effects adjustables
    {Setting::PHOSPHOR_MODE,          {true,  [this](int d) { myOSystem.console().cyclePhosphorMode(d); }}},
    {Setting::PHOSPHOR,               {true,  [this](int d) { myOSystem.console().changePhosphor(d); }}},
    {Setting::SCANLINES,              {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().changeScanlineIntensity(d); }}},
    {Setting::SCANLINE_MASK,          {false, [this](int d) { myOSystem.frameBuffer().tiaSurface().cycleScanlineMask(d); }}},
    {Setting::INTERPOLATION,          {false, [this](int d) { myOSystem.console().toggleInter(d); }}},
    {Setting::BEZEL,                  {false, [this](int d) { myOSystem.frameBuffer().toggleBezel(d); }}},
    // *** Input group ***
    {Setting::DIGITAL_DEADZONE,       {true,  [this](int d) { joyHandler().changeDigitalDeadZone(d); }}},
    {Setting::ANALOG_DEADZONE,        {true,  [this](int d) { joyHandler().changeAnalogPaddleDeadZone(d); }}},
    {Setting::ANALOG_SENSITIVITY,     {true,  [this](int d) { joyHandler().changeAnalogPaddleSensitivity(d); }}},
    {Setting::ANALOG_LINEARITY,       {true,  [this](int d) { joyHandler().changeAnalogPaddleLinearity(d); }}},
    {Setting::DEJITTER_AVERAGING,     {true,  [this](int d) { joyHandler().changePaddleDejitterAveraging(d); }}},
    {Setting::DEJITTER_REACTION,      {true,  [this](int d) { joyHandler().changePaddleDejitterReaction(d); }}},
    {Setting::DIGITAL_SENSITIVITY,    {true,  [this](int d) { joyHandler().changeDigitalPaddleSensitivity(d); }}},
    {Setting::AUTO_FIRE,              {true,  [this](int d) { myOSystem.console().changeAutoFireRate(d); }}},
    {Setting::FOUR_DIRECTIONS,        {false, [this](int d) { myOSystem.eventHandler().toggleAllow4JoyDirections(d); }}},
    {Setting::MOD_KEY_COMBOS,         {false, [this](int d) { keyHandler().toggleModKeys(d); }}},
    {Setting::SA_PORT_ORDER,          {false, [this](int d) { myOSystem.eventHandler().toggleSAPortOrder(d); }}},
    {Setting::USE_MOUSE,              {false, [this](int d) { myOSystem.eventHandler().changeMouseControllerMode(d); }}},
    {Setting::PADDLE_SENSITIVITY,     {true,  [this](int d) { joyHandler().changeMousePaddleSensitivity(d); }}},
    {Setting::TRACKBALL_SENSITIVITY,  {true,  [this](int d) { joyHandler().changeMouseTrackballSensitivity(d); }}},
    {Setting::DRIVING_SENSITIVITY,    {true,  [this](int d) { joyHandler().changeDrivingSensitivity(d); }}},
    {Setting::MOUSE_CURSOR,           {false, [this](int d) { myOSystem.eventHandler().changeMouseCursor(d); }}},
    {Setting::GRAB_MOUSE,             {false, [this](int d) { myOSystem.frameBuffer().toggleGrabMouse(d); }}},
    // Game properties/Controllers
    {Setting::LEFT_PORT,              {false, [this](int d) { myOSystem.console().changeLeftController(d); }}}, // property, not persisted
    {Setting::RIGHT_PORT,             {false, [this](int d) { myOSystem.console().changeRightController(d); }}}, // property, not persisted
    {Setting::SWAP_PORTS,             {false, [this](int d) { myOSystem.console().toggleSwapPorts(d); }}}, // property, not persisted
    {Setting::SWAP_PADDLES,           {false, [this](int d) { myOSystem.console().toggleSwapPaddles(d); }}}, // property, not persisted
    {Setting::PADDLE_CENTER_X,        {true,  [this](int d) { myOSystem.console().changePaddleCenterX(d); }}}, // property, not persisted
    {Setting::PADDLE_CENTER_Y,        {true,  [this](int d) { myOSystem.console().changePaddleCenterY(d); }}}, // property, not persisted
    {Setting::MOUSE_CONTROL,          {false, [this](int d) { myOSystem.eventHandler().changeMouseControl(d); }}}, // property, not persisted
    {Setting::MOUSE_RANGE,            {true,  [this](int d) { myOSystem.console().changePaddleAxesRange(d); }}}, // property, not persisted
    // *** Debug group ***
    {Setting::DEVELOPER,              {false, [this](int d) { myOSystem.console().toggleDeveloperSet(d); }}},
    {Setting::STATS,                  {false, [this](int d) { myOSystem.frameBuffer().toggleFrameStats(d); }}},
    {Setting::P0_ENAM,                {false, [this](int d) { myOSystem.console().toggleP0Bit(d); }}}, // debug, not persisted
    {Setting::P1_ENAM,                {false, [this](int d) { myOSystem.console().toggleP1Bit(d); }}}, // debug, not persisted
    {Setting::M0_ENAM,                {false, [this](int d) { myOSystem.console().toggleM0Bit(d); }}}, // debug, not persisted
    {Setting::M1_ENAM,                {false, [this](int d) { myOSystem.console().toggleM1Bit(d); }}}, // debug, not persisted
    {Setting::BL_ENAM,                {false, [this](int d) { myOSystem.console().toggleBLBit(d); }}}, // debug, not persisted
    {Setting::PF_ENAM,                {false, [this](int d) { myOSystem.console().togglePFBit(d); }}}, // debug, not persisted
    {Setting::ALL_ENAM,               {false, [this](int d) { myOSystem.console().toggleBits(d); }}}, // debug, not persisted
    {Setting::P0_CX,                  {false, [this](int d) { myOSystem.console().toggleP0Collision(d); }}}, // debug, not persisted
    {Setting::P1_CX,                  {false, [this](int d) { myOSystem.console().toggleP1Collision(d); }}}, // debug, not persisted
    {Setting::M0_CX,                  {false, [this](int d) { myOSystem.console().toggleM0Collision(d); }}}, // debug, not persisted
    {Setting::M1_CX,                  {false, [this](int d) { myOSystem.console().toggleM1Collision(d); }}}, // debug, not persisted
    {Setting::BL_CX,                  {false, [this](int d) { myOSystem.console().toggleBLCollision(d); }}}, // debug, not persisted
    {Setting::PF_CX,                  {false, [this](int d) { myOSystem.console().togglePFCollision(d); }}}, // debug, not persisted
    {Setting::ALL_CX,                 {false, [this](int d) { myOSystem.console().toggleCollisions(d); }}}, // debug, not persisted
    {Setting::FIXED_COL,              {false, [this](int d) { myOSystem.console().toggleFixedColors(d); }}}, // debug, not persisted
    {Setting::COLOR_LOSS,             {false, [this](int d) { myOSystem.console().toggleColorLoss(d); }}},
    {Setting::JITTER_SENSE,           {true,  [this](int d) { myOSystem.console().changeJitterSense(d); }}},
    {Setting::JITTER_REC,             {true,  [this](int d) { myOSystem.console().changeJitterRecovery(d); }}},
    // *** Following functions are not used when cycling settings, but for "direct only" hotkeys ***
    {Setting::STATE,                  {true,  [this](int d) { myOSystem.state().changeState(d); }}}, // temporary, not persisted
    {Setting::PALETTE_ATTRIBUTE,      {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().paletteHandler().changeCurrentAdjustable(d); }}},
    {Setting::NTSC_ATTRIBUTE,         {true,  [this](int d) { myOSystem.frameBuffer().tiaSurface().changeCurrentNTSCAdjustable(d); }}},
    {Setting::CHANGE_SPEED,           {true,  [this](int d) { myOSystem.console().changeSpeed(d); }}},
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GlobalKeyHandler::SettingData GlobalKeyHandler::getSettingData(Setting setting) const
{
  const auto result = mySettingMap.find(setting);
  if(result != mySettingMap.end())
    return result->second;

  cerr << "Error: setting " << static_cast<int>(setting)
       << " missing in SettingMap!\n";
  return mySettingMap.find(Setting::VOLUME)->second; // default function!
}
