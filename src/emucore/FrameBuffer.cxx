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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"

#include "Console.hxx"
#include "EventHandler.hxx"
#include "Event.hxx"
#include "Font.hxx"
#include "StellaFont.hxx"
#include "StellaMediumFont.hxx"
#include "StellaLargeFont.hxx"
#include "ConsoleFont.hxx"
#include "Launcher.hxx"
#include "Menu.hxx"
#include "CommandMenu.hxx"
#include "TimeMachine.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "TIA.hxx"

#include "FBSurface.hxx"
#include "TIASurface.hxx"
#include "FrameBuffer.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::FrameBuffer(OSystem& osystem)
  : myOSystem(osystem),
    myInitializedCount(0),
    myPausedCount(0),
    myStatsEnabled(false),
    myLastScanlines(0),
    myLastFrameRate(60),
    myGrabMouse(false),
    myCurrentModeList(nullptr),
    myTotalTime(0),
    myTotalFrames(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::~FrameBuffer()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::initialize()
{
  // Get desktop resolution and supported renderers
  queryHardware(myDisplays, myRenderers);
  uInt32 query_w = myDisplays[0].w, query_h = myDisplays[0].h;

  // Check the 'maxres' setting, which is an undocumented developer feature
  // that specifies the desktop size (not normally set)
  const GUI::Size& s = myOSystem.settings().getSize("maxres");
  if(s.valid())
  {
    query_w = s.w;
    query_h = s.h;
  }
  // Various parts of the codebase assume a minimum screen size
  myDesktopSize.w = std::max(query_w, uInt32(kFBMinW));
  myDesktopSize.h = std::max(query_h, uInt32(kFBMinH));

  ////////////////////////////////////////////////////////////////////
  // Create fonts to draw text
  // NOTE: the logic determining appropriate font sizes is done here,
  //       so that the UI classes can just use the font they expect,
  //       and not worry about it
  //       This logic should also take into account the size of the
  //       framebuffer, and try to be intelligent about font sizes
  //       We can probably add ifdefs to take care of corner cases,
  //       but that means we've failed to abstract it enough ...
  ////////////////////////////////////////////////////////////////////
  bool smallScreen = myDesktopSize.w < kFBMinW || myDesktopSize.h < kFBMinH;

  // This font is used in a variety of situations when a really small
  // font is needed; we let the specific widget/dialog decide when to
  // use it
  mySmallFont = make_unique<GUI::Font>(GUI::stellaDesc);

  // The general font used in all UI elements
  // This is determined by the size of the framebuffer
  myFont = make_unique<GUI::Font>(smallScreen ? GUI::stellaDesc : GUI::stellaMediumDesc);

  // The info font used in all UI elements
  // This is determined by the size of the framebuffer
  myInfoFont = make_unique<GUI::Font>(smallScreen ? GUI::stellaDesc : GUI::consoleDesc);

  // The font used by the ROM launcher
  // Normally, this is configurable by the user, except in the case of
  // very small screens
  if(!smallScreen)
  {
    const string& lf = myOSystem.settings().getString("launcherfont");
    if(lf == "small")
      myLauncherFont = make_unique<GUI::Font>(GUI::consoleDesc);
    else if(lf == "medium")
      myLauncherFont = make_unique<GUI::Font>(GUI::stellaMediumDesc);
    else
      myLauncherFont = make_unique<GUI::Font>(GUI::stellaLargeDesc);
  }
  else
    myLauncherFont = make_unique<GUI::Font>(GUI::stellaDesc);

  // Determine possible TIA windowed zoom levels
  uInt32 maxZoom = maxWindowSizeForScreen(uInt32(kTIAMinW), uInt32(kTIAMinH),
                     myDesktopSize.w, myDesktopSize.h);

  // Figure our the smallest zoom level we can use
  uInt32 firstZoom = smallScreen ? 1 : 2;
  for(uInt32 zoom = firstZoom; zoom <= maxZoom; ++zoom)
  {
    ostringstream desc;
    desc << "Zoom " << zoom << "x";
    VarList::push_back(myTIAZoomLevels, desc.str(), zoom);
  }

  // Set palette for GUI (upper area of array, doesn't change during execution)
  int palID = 0;
  if(myOSystem.settings().getString("uipalette") == "classic")
    palID = 1;
  else if(myOSystem.settings().getString("uipalette") == "light")
    palID = 2;

  for(int i = 0, j = 256; i < kNumColors-256; ++i, ++j)
  {
    uInt8 r = (ourGUIColors[palID][i] >> 16) & 0xff;
    uInt8 g = (ourGUIColors[palID][i] >> 8) & 0xff;
    uInt8 b = ourGUIColors[palID][i] & 0xff;

    myPalette[j] = mapRGB(r, g, b);
  }
  FBSurface::setPalette(myPalette);

  myGrabMouse = myOSystem.settings().getBool("grabmouse");

  // Create a TIA surface; we need it for rendering TIA images
  myTIASurface = make_unique<TIASurface>(myOSystem);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus FrameBuffer::createDisplay(const string& title,
                                        uInt32 width, uInt32 height)
{
  myInitializedCount++;
  myScreenTitle = title;

  // A 'windowed' system is defined as one where the window size can be
  // larger than the screen size, as there's some sort of window manager
  // that takes care of it (all current desktop systems fall in this category)
  // However, some systems have no concept of windowing, and have hard limits
  // on how large a window can be (ie, the size of the 'desktop' is the
  // absolute upper limit on window size)
  //
  // If the WINDOWED_SUPPORT macro is defined, we treat the system as the
  // former type; if not, as the latter type

  bool useFullscreen = false;
#ifdef WINDOWED_SUPPORT
  // We assume that a desktop of at least minimum acceptable size means that
  // we're running on a 'large' system, and the window size requirements
  // can be relaxed
  // Otherwise, we treat the system as if WINDOWED_SUPPORT is not defined
  if(myDesktopSize.w < kFBMinW && myDesktopSize.h < kFBMinH &&
    (myDesktopSize.w < width || myDesktopSize.h < height))
    return FBInitStatus::FailTooLarge;

  useFullscreen = myOSystem.settings().getBool("fullscreen");
#else
  // Make sure this mode is even possible
  // We only really need to worry about it in non-windowed environments,
  // where requesting a window that's too large will probably cause a crash
  if(myDesktopSize.w < width || myDesktopSize.h < height)
    return FBInitStatus::FailTooLarge;
#endif

  // Set the available video modes for this framebuffer
  setAvailableVidModes(width, height);

  // Initialize video subsystem (make sure we get a valid mode)
  string pre_about = about();
  const VideoMode& mode = getSavedVidMode(useFullscreen);
  if(width <= mode.screen.w && height <= mode.screen.h)
  {
    if(setVideoMode(myScreenTitle, mode))
    {
      myImageRect = mode.image;
      myScreenSize = mode.screen;

      // Inform TIA surface about new mode
      if(myOSystem.eventHandler().state() != EventHandlerState::LAUNCHER &&
         myOSystem.eventHandler().state() != EventHandlerState::DEBUGGER)
        myTIASurface->initialize(myOSystem.console(), mode);

      // Did we get the requested fullscreen state?
      myOSystem.settings().setValue("fullscreen", fullScreen());
      resetSurfaces();
      setCursorState();
    }
    else
    {
      myOSystem.logMessage("ERROR: Couldn't initialize video subsystem", 0);
      return FBInitStatus::FailNotSupported;
    }
  }
  else
    return FBInitStatus::FailTooLarge;

  // Erase any messages from a previous run
  myMsg.counter = 0;

  // Create surfaces for TIA statistics and general messages
  myStatsMsg.color = kColorInfo;
  myStatsMsg.w = font().getMaxCharWidth() * 30 + 3;
  myStatsMsg.h = (font().getFontHeight() + 2) * 2;

  if(!myStatsMsg.surface)
  {
    myStatsMsg.surface = allocateSurface(myStatsMsg.w, myStatsMsg.h);
    myStatsMsg.surface->attributes().blending = true;
    myStatsMsg.surface->attributes().blendalpha = 92; //aligned with TimeMachineDialog
    myStatsMsg.surface->applyAttributes();
  }

  if(!myMsg.surface)
    myMsg.surface = allocateSurface(kFBMinW, font().getFontHeight()+10);

  // Print initial usage message, but only print it later if the status has changed
  if(myInitializedCount == 1)
  {
    myOSystem.logMessage(about(), 1);
  }
  else
  {
    string post_about = about();
    if(post_about != pre_about)
      myOSystem.logMessage(post_about, 1);
  }

  return FBInitStatus::Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::update()
{
  // Determine which mode we are in (from the EventHandler)
  // Take care of S_EMULATE mode here, otherwise let the GUI
  // figure out what to draw

  invalidate();
  switch(myOSystem.eventHandler().state())
  {
    case EventHandlerState::EMULATION:
    {
      // Run the console for one frame
      // Note that the debugger can cause a breakpoint to occur, which changes
      // the EventHandler state 'behind our back' - we need to check for that
      myOSystem.console().tia().update();
  #ifdef DEBUGGER_SUPPORT
      if(myOSystem.eventHandler().state() != EventHandlerState::EMULATION) break;
  #endif
      if(myOSystem.eventHandler().frying())
        myOSystem.console().fry();

      // And update the screen
      myTIASurface->render();

      // Show frame statistics
      if(myStatsMsg.enabled)
        drawFrameStats();
      else
        myLastFrameRate = myOSystem.console().getFramerate();
      myLastScanlines = myOSystem.console().tia().scanlinesLastFrame();
      myPausedCount = 0;
      break;  // EventHandlerState::EMULATION
    }

    case EventHandlerState::PAUSE:
    {
      myTIASurface->render();

      // Show a pause message immediately and then every 7 seconds
      if (myPausedCount-- <= 0)
      {
        myPausedCount = uInt32(7 * myOSystem.frameRate());
        showMessage("Paused", MessagePosition::MiddleCenter);
      }
      break;  // EventHandlerState::PAUSE
    }

    case EventHandlerState::OPTIONSMENU:
    {
      myTIASurface->render();
      myOSystem.menu().draw(true);
      break;  // EventHandlerState::OPTIONSMENU
    }

    case EventHandlerState::CMDMENU:
    {
      myTIASurface->render();
      myOSystem.commandMenu().draw(true);
      break;  // EventHandlerState::CMDMENU
    }

    case EventHandlerState::TIMEMACHINE:
    {
      myTIASurface->render();
      myOSystem.timeMachine().draw(true);
      break;  // EventHandlerState::TIMEMACHINE
    }

    case EventHandlerState::LAUNCHER:
    {
      myOSystem.launcher().draw(true);
      break;  // EventHandlerState::LAUNCHER
    }

    case EventHandlerState::DEBUGGER:
    {
  #ifdef DEBUGGER_SUPPORT
      myOSystem.debugger().draw(true);
  #endif
      break;  // EventHandlerState::DEBUGGER
    }

    case EventHandlerState::NONE:
      return;
  }

  // Draw any pending messages
  if(myMsg.enabled)
    drawMessage();

  // Do any post-frame stuff
  postFrameUpdate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showMessage(const string& message, MessagePosition position,
                              bool force)
{
  // Only show messages if they've been enabled
  if(!(force || myOSystem.settings().getBool("uimessages")))
    return;

  // Precompute the message coordinates
  myMsg.text    = message;
  myMsg.counter = uInt32(myOSystem.frameRate()) << 1; // Show message for 2 seconds
  myMsg.color   = kBtnTextColor;

  myMsg.w = font().getStringWidth(myMsg.text) + 10;
  myMsg.h = font().getFontHeight() + 8;
  myMsg.surface->setSrcSize(myMsg.w, myMsg.h);
  myMsg.surface->setDstSize(myMsg.w, myMsg.h);
  myMsg.position = position;
  myMsg.enabled = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::drawFrameStats()
{
  const ConsoleInfo& info = myOSystem.console().about();
  char msg[30];
  uInt32 color;
  const int XPOS = 2, YPOS = 0;
  int xPos = XPOS;

  myStatsMsg.surface->invalidate();

  // draw scanlines
  color = myOSystem.console().tia().scanlinesLastFrame() != myLastScanlines ?
      uInt32(kDbgColorRed) : myStatsMsg.color;
  std::snprintf(msg, 30, "%3u", myOSystem.console().tia().scanlinesLastFrame());
  myStatsMsg.surface->drawString(font(), msg, xPos, YPOS,
                                 myStatsMsg.w, color, TextAlign::Left, 0, true, kBGColor);
  xPos += font().getStringWidth(msg);

  // draw frequency
  std::snprintf(msg, 30, " => %s", info.DisplayFormat.c_str());
  myStatsMsg.surface->drawString(font(), msg, xPos, YPOS,
                                 myStatsMsg.w, myStatsMsg.color, TextAlign::Left, 0, true, kBGColor);
  xPos += font().getStringWidth(msg);

  // draw the effective framerate
  float frameRate;
  const TimingInfo& ti = myOSystem.timingInfo();
  if(ti.totalFrames - myTotalFrames >= myLastFrameRate)
  {
    frameRate = 1000000.0 * (ti.totalFrames - myTotalFrames) / (ti.totalTime - myTotalTime);
    if(frameRate > myOSystem.console().getFramerate() + 1)
      frameRate = 1;
    myTotalFrames = ti.totalFrames;
    myTotalTime = ti.totalTime;
  }
  else
    frameRate = myLastFrameRate;
  std::snprintf(msg, 30, " @ %5.2ffps", frameRate);
  myStatsMsg.surface->drawString(font(), msg, xPos, YPOS,
                                 myStatsMsg.w, myStatsMsg.color, TextAlign::Left, 0, true, kBGColor);
  myLastFrameRate = frameRate;

  // draw bankswitching type
  string bsinfo = info.BankSwitch +
    (myOSystem.settings().getBool("dev.settings") ? "| Developer" : "");
  myStatsMsg.surface->drawString(font(), bsinfo, XPOS, YPOS + font().getFontHeight(),
                                 myStatsMsg.w, myStatsMsg.color, TextAlign::Left, 0, true, kBGColor);

  myStatsMsg.surface->setDirty();
  myStatsMsg.surface->setDstPos(myImageRect.x() + 10, myImageRect.y() + 8);
  myStatsMsg.surface->render();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFrameStats()
{
  showFrameStats(!myStatsEnabled);
  myOSystem.settings().setValue(
    myOSystem.settings().getBool("dev.settings") ? "dev.stats" : "plr.stats", myStatsEnabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showFrameStats(bool enable)
{
  myStatsEnabled = myStatsMsg.enabled = enable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::enableMessages(bool enable)
{
  if(enable)
  {
    // Only re-enable frame stats if they were already enabled before
    myStatsMsg.enabled = myStatsEnabled;
  }
  else
  {
    // Temporarily disable frame stats
    myStatsMsg.enabled = false;

    // Erase old messages on the screen
    myMsg.enabled = false;
    myMsg.counter = 0;
    update();  // Force update immediately
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void FrameBuffer::drawMessage()
{
  // Draw the bounded box and text
  const GUI::Rect& dst = myMsg.surface->dstRect();

  switch(myMsg.position)
  {
    case MessagePosition::TopLeft:
      myMsg.x = 5;
      myMsg.y = 5;
      break;

    case MessagePosition::TopCenter:
      myMsg.x = (myImageRect.width() - dst.width()) >> 1;
      myMsg.y = 5;
      break;

    case MessagePosition::TopRight:
      myMsg.x = myImageRect.width() - dst.width() - 5;
      myMsg.y = 5;
      break;

    case MessagePosition::MiddleLeft:
      myMsg.x = 5;
      myMsg.y = (myImageRect.height() - dst.height()) >> 1;
      break;

    case MessagePosition::MiddleCenter:
      myMsg.x = (myImageRect.width() - dst.width()) >> 1;
      myMsg.y = (myImageRect.height() - dst.height()) >> 1;
      break;

    case MessagePosition::MiddleRight:
      myMsg.x = myImageRect.width() - dst.width() - 5;
      myMsg.y = (myImageRect.height() - dst.height()) >> 1;
      break;

    case MessagePosition::BottomLeft:
      myMsg.x = 5;
      myMsg.y = myImageRect.height() - dst.height() - 5;
      break;

    case MessagePosition::BottomCenter:
      myMsg.x = (myImageRect.width() - dst.width()) >> 1;
      myMsg.y = myImageRect.height() - dst.height() - 5;
      break;

    case MessagePosition::BottomRight:
      myMsg.x = myImageRect.width() - dst.width() - 5;
      myMsg.y = myImageRect.height() - dst.height() - 5;
      break;
  }

  myMsg.surface->setDstPos(myMsg.x + myImageRect.x(), myMsg.y + myImageRect.y());
  myMsg.surface->fillRect(1, 1, myMsg.w-2, myMsg.h-2, kBtnColor);
  myMsg.surface->frameRect(0, 0, myMsg.w, myMsg.h, kColor);
  myMsg.surface->drawString(font(), myMsg.text, 5, 4,
                            myMsg.w, myMsg.color, TextAlign::Left);

  // Either erase the entire message (when time is reached),
  // or show again this frame
  if(myMsg.counter-- > 0)
  {
    myMsg.surface->setDirty();
    myMsg.surface->render();
  }
  else
    myMsg.enabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setPauseDelay()
{
  myPausedCount = uInt32(2 * myOSystem.frameRate());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
shared_ptr<FBSurface> FrameBuffer::allocateSurface(int w, int h, const uInt32* data)
{
  // Add new surface to the list
  mySurfaceList.push_back(createSurface(w, h, data));

  // And return a pointer to it (pointer should be treated read-only)
  return mySurfaceList.at(mySurfaceList.size() - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::resetSurfaces()
{
  // Free all resources for each surface, then reload them
  // Due to possible timing and/or synchronization issues, all free()'s
  // are done first, then all reload()'s
  // Any derived FrameBuffer classes that call this method should be
  // aware of these restrictions, and act accordingly

  for(auto& s: mySurfaceList)
    s->free();
  for(auto& s: mySurfaceList)
    s->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setPalette(const uInt32* raw_palette)
{
  // Set palette for normal fill
  for(int i = 0; i < 256; ++i)
  {
    uInt8 r = (raw_palette[i] >> 16) & 0xff;
    uInt8 g = (raw_palette[i] >> 8) & 0xff;
    uInt8 b = raw_palette[i] & 0xff;

    myPalette[i] = mapRGB(r, g, b);
  }

  // Let the TIA surface know about the new palette
  myTIASurface->setPalette(myPalette, raw_palette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::stateChanged(EventHandlerState state)
{
  // Make sure any onscreen messages are removed
  myMsg.enabled = false;
  myMsg.counter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setFullscreen(bool enable)
{
  const VideoMode& mode = getSavedVidMode(enable);
  if(setVideoMode(myScreenTitle, mode))
  {
    myImageRect = mode.image;
    myScreenSize = mode.screen;

    // Inform TIA surface about new mode
    if(myOSystem.eventHandler().state() != EventHandlerState::LAUNCHER &&
       myOSystem.eventHandler().state() != EventHandlerState::DEBUGGER)
      myTIASurface->initialize(myOSystem.console(), mode);

    // Did we get the requested fullscreen state?
    myOSystem.settings().setValue("fullscreen", fullScreen());
    resetSurfaces();
    setCursorState();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFullscreen()
{
  setFullscreen(!fullScreen());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::changeWindowedVidMode(int direction)
{
#ifdef WINDOWED_SUPPORT
  EventHandlerState state = myOSystem.eventHandler().state();
  bool tiaMode = (state != EventHandlerState::DEBUGGER &&
                  state != EventHandlerState::LAUNCHER);

  // Ignore any attempts to change video size while in invalid modes
  if(!tiaMode || fullScreen())
    return false;

  if(direction == +1)
    myCurrentModeList->next();
  else if(direction == -1)
    myCurrentModeList->previous();
  else
    return false;

  const VideoMode& mode = myCurrentModeList->current();
  if(setVideoMode(myScreenTitle, mode))
  {
    myImageRect = mode.image;
    myScreenSize = mode.screen;

    // Inform TIA surface about new mode
    myTIASurface->initialize(myOSystem.console(), mode);

    resetSurfaces();
    showMessage(mode.description);
    myOSystem.settings().setValue("tia.zoom", mode.zoom);
    return true;
  }
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setCursorState()
{
  // Always grab mouse in emulation (if enabled) and emulating a controller
  // that always uses the mouse
  bool emulation =
      myOSystem.eventHandler().state() == EventHandlerState::EMULATION;
  bool analog = myOSystem.hasConsole() ?
      (myOSystem.console().controller(Controller::Left).isAnalog() ||
       myOSystem.console().controller(Controller::Right).isAnalog()) : false;
  bool alwaysUseMouse = BSPF::equalsIgnoreCase("always", myOSystem.settings().getString("usemouse"));

  grabMouse(emulation && (analog || alwaysUseMouse) && myGrabMouse);

  // Show/hide cursor in UI/emulation mode based on 'cursor' setting
  switch(myOSystem.settings().getInt("cursor"))
  {
    case 0: showCursor(false);      break;
    case 1: showCursor(emulation);  break;
    case 2: showCursor(!emulation); break;
    case 3: showCursor(true);       break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::enableGrabMouse(bool enable)
{
  myGrabMouse = enable;
  setCursorState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleGrabMouse()
{
  myGrabMouse = !myGrabMouse;
  setCursorState();
  myOSystem.settings().setValue("grabmouse", myGrabMouse);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::maxWindowSizeForScreen(uInt32 baseWidth, uInt32 baseHeight,
                    uInt32 screenWidth, uInt32 screenHeight) const
{
  uInt32 multiplier = 1;
  for(;;)
  {
    // Figure out the zoomed size of the window
    uInt32 width  = baseWidth * multiplier;
    uInt32 height = baseHeight * multiplier;

    if((width > screenWidth) || (height > screenHeight))
      break;

    ++multiplier;
  }
  return multiplier > 1 ? multiplier - 1 : 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setAvailableVidModes(uInt32 baseWidth, uInt32 baseHeight)
{
  myWindowedModeList.clear();

  for(auto& mode: myFullscreenModeLists)
    mode.clear();
  for(size_t i = myFullscreenModeLists.size(); i < myDisplays.size(); ++i)
    myFullscreenModeLists.push_back(VideoModeList());

  // Check if zooming is allowed for this state (currently only allowed
  // for TIA screens)
  EventHandlerState state = myOSystem.eventHandler().state();
  bool tiaMode = (state != EventHandlerState::DEBUGGER &&
                  state != EventHandlerState::LAUNCHER);

  // TIA mode allows zooming at integral factors in windowed modes,
  // and also non-integral factors in fullscreen mode
  if(tiaMode)
  {
    // TIA windowed modes
    uInt32 maxZoom = maxWindowSizeForScreen(baseWidth, baseHeight,
                     myDesktopSize.w, myDesktopSize.h);

    // Aspect ratio
    bool ntsc = myOSystem.console().about().InitialFrameRate == "60";
    uInt32 aspect = myOSystem.settings().getInt(ntsc ?
                      "tia.aspectn" : "tia.aspectp");

    // Figure our the smallest zoom level we can use
    uInt32 firstZoom = 2;
    if(myDesktopSize.w < kFBMinW || myDesktopSize.h < kFBMinH)
      firstZoom = 1;
    for(uInt32 zoom = firstZoom; zoom <= maxZoom; ++zoom)
    {
      ostringstream desc;
      desc << "Zoom " << zoom << "x";

      VideoMode mode(baseWidth*zoom, baseHeight*zoom,
              baseWidth*zoom, baseHeight*zoom, -1, zoom, desc.str());
      mode.applyAspectCorrection(aspect);
      myWindowedModeList.add(mode);
    }

    // TIA fullscreen mode
    for(uInt32 i = 0; i < myDisplays.size(); ++i)
    {
      maxZoom = maxWindowSizeForScreen(baseWidth, baseHeight,
                                       myDisplays[i].w, myDisplays[i].h);
      VideoMode mode(baseWidth*maxZoom, baseHeight*maxZoom,
                     myDisplays[i].w, myDisplays[i].h, i);
      mode.applyAspectCorrection(aspect, myOSystem.settings().getBool("tia.fsfill"));
      myFullscreenModeLists[i].add(mode);
    }
  }
  else  // UI mode
  {
    // Windowed and fullscreen mode differ only in screen size
    myWindowedModeList.add(
        VideoMode(baseWidth, baseHeight, baseWidth, baseHeight, -1)
    );
    for(uInt32 i = 0; i < myDisplays.size(); ++i)
    {
      myFullscreenModeLists[i].add(
          VideoMode(baseWidth, baseHeight, myDisplays[i].w, myDisplays[i].h, i)
      );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const VideoMode& FrameBuffer::getSavedVidMode(bool fullscreen)
{
  EventHandlerState state = myOSystem.eventHandler().state();

  if(fullscreen)
  {
    Int32 i = getCurrentDisplayIndex();
    if(i < 0)
    {
      // default to the first display
      i = 0;
    }
    myCurrentModeList = &myFullscreenModeLists[i];
  }
  else
    myCurrentModeList = &myWindowedModeList;

  // Now select the best resolution depending on the state
  // UI modes (launcher and debugger) have only one supported resolution
  // so the 'current' one is the only valid one
  if(state == EventHandlerState::DEBUGGER || state == EventHandlerState::LAUNCHER)
    myCurrentModeList->setZoom(1);
  else
    myCurrentModeList->setZoom(myOSystem.settings().getInt("tia.zoom"));

  return myCurrentModeList->current();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// VideoMode implementation
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoMode::VideoMode()
  : fsIndex(-1),
    zoom(1),
    description("")
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoMode::VideoMode(uInt32 iw, uInt32 ih, uInt32 sw, uInt32 sh,
                     Int32 full, uInt32 z, const string& desc)
  : fsIndex(full),
    zoom(z),
    description(desc)
{
  sw = std::max(sw, uInt32(FrameBuffer::kTIAMinW));
  sh = std::max(sh, uInt32(FrameBuffer::kTIAMinH));
  iw = std::min(iw, sw);
  ih = std::min(ih, sh);
  int ix = (sw - iw) >> 1;
  int iy = (sh - ih) >> 1;
  image = GUI::Rect(ix, iy, ix+iw, iy+ih);
  screen = GUI::Size(sw, sh);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoMode::applyAspectCorrection(uInt32 aspect, bool stretch)
{
  // Width is modified by aspect ratio; other factors may be applied below
  uInt32 iw = uInt32(float(image.width() * aspect) / 100.0);
  uInt32 ih = image.height();

  if(fsIndex != -1)
  {
    // Fullscreen mode stretching
    float stretchFactor = 1.0;
    float scaleX = float(iw) / screen.w;
    float scaleY = float(ih) / screen.h;

    // Scale to actual or integral factors
    if(stretch)
    {
      // Scale to full (non-integral) available space
      if(scaleX > scaleY)
        stretchFactor = float(screen.w) / iw;
      else
        stretchFactor = float(screen.h) / ih;
    }
    else
    {
      // Only scale to an integral amount
      if(scaleX > scaleY)
      {
        int bw = iw / zoom;
        stretchFactor = float(int(screen.w / bw) * bw) / iw;
      }
      else
      {
        int bh = ih / zoom;
        stretchFactor = float(int(screen.h / bh) * bh) / ih;
      }
    }
    iw = uInt32(stretchFactor * iw);
    ih = uInt32(stretchFactor * ih);
  }
  else
  {
    // In windowed mode, the screen size changes to match the image width
    // Height is never modified in this mode
    screen.w = iw;
  }

  // Now re-calculate the dimensions
  iw = std::min(iw, screen.w);
  ih = std::min(ih, screen.h);

  image.moveTo((screen.w - iw) >> 1, (screen.h - ih) >> 1);
  image.setWidth(iw);
  image.setHeight(ih);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// VideoModeList implementation
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::VideoModeList::VideoModeList()
  : myIdx(-1)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::add(const VideoMode& mode)
{
  myModeList.emplace_back(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::clear()
{
  myModeList.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::VideoModeList::empty() const
{
  return myModeList.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::VideoModeList::size() const
{
  return uInt32(myModeList.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::previous()
{
  --myIdx;
  if(myIdx < 0) myIdx = int(myModeList.size()) - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const VideoMode& FrameBuffer::VideoModeList::current() const
{
  return myModeList[myIdx];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::next()
{
  myIdx = (myIdx + 1) % myModeList.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::setZoom(uInt32 zoom)
{
  for(uInt32 i = 0; i < myModeList.size(); ++i)
  {
    if(myModeList[i].zoom == zoom)
    {
      myIdx = i;
      return;
    }
  }
  myIdx = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
  Palette is defined as follows:
    *** Base colors ***
    kColor            Normal foreground color (non-text)
    kBGColor          Normal background color (non-text)
    kBGColorLo        Disabled background color dark (non-text)
    kBGColorHi        Disabled background color light (non-text)
    kShadowColor      Item is disabled
    *** Text colors ***
    kTextColor        Normal text color
    kTextColorHi      Highlighted text color
    kTextColorEm      Emphasized text color
    kTextColorInv     Color for selected text
    *** UI elements (dialog and widgets) ***
    kDlgColor         Dialog background
    kWidColor         Widget background
    kWidColorHi       Widget highlight color
    kWidFrameColor    Border for currently selected widget
    *** Button colors ***
    kBtnColor         Normal button background
    kBtnColorHi       Highlighted button background
    kBtnBorderColor,
    kBtnBorderColorHi,
    kBtnTextColor     Normal button font color
    kBtnTextColorHi   Highlighted button font color
    *** Checkbox colors ***
    kCheckColor       Color of 'X' in checkbox
    *** Scrollbar colors ***
    kScrollColor      Normal scrollbar color
    kScrollColorHi    Highlighted scrollbar color
    *** Slider colors ***
    kSliderColor          Enabled slider
    kSliderColorHi        Focussed slider
    kSliderBGColor        Enabled slider background
    kSliderBGColorHi      Focussed slider background
    kSliderBGColorLo      Disabled slider background
    *** Debugger colors ***
    kDbgChangedColor      Background color for changed cells
    kDbgChangedTextColor  Text color for changed cells
    kDbgColorHi           Highlighted color in debugger data cells
    kDbgColorRed          Red color in debugger
    *** Other colors ***
    kColorInfo            TIA output position color
    kColorTitleBar        Title bar color
    kColorTitleText       Title text color
    kColorTitleBarLo      Disabled title bar color
    kColorTitleTextLo     Disabled title text color
*/
uInt32 FrameBuffer::ourGUIColors[3][kNumColors-256] = {
  // Standard
  { 0x686868, 0x000000, 0xa38c61, 0xdccfa5, 0x404040,           // base
    0x000000, 0xac3410, 0x9f0000, 0xf0f0cf,                     // text
    0xc9af7c, 0xf0f0cf, 0xd55941, 0xc80000,                     // UI elements
    0xac3410, 0xd55941, 0x686868, 0xdccfa5, 0xf0f0cf, 0xf0f0cf, // buttons
    0xac3410,                                                   // checkbox
    0xac3410, 0xd55941,                                         // scrollbar
    0xac3410, 0xd55941, 0xdccfa5, 0xf0f0cf, 0xa38c61,           // slider
    0xc80000, 0x00ff00, 0xc8c8ff, 0xc80000,                     // debugger
    0xffffff, 0xac3410, 0xf0f0cf, 0x686868, 0xdccfa5            // other
  },
  // Classic
  { 0x686868, 0x000000, 0x404040, 0x404040, 0x404040,           // base
    0x20a020, 0x00ff00, 0xc80000, 0x000000,                     // text
    0x000000, 0x000000, 0x00ff00, 0xc80000,                     // UI elements
    0x000000, 0x000000, 0x686868, 0x00ff00, 0x20a020, 0x00ff00, // buttons
    0x20a020,                                                   // checkbox
    0x20a020, 0x00ff00,                                         // scrollbar
    0x20a020, 0x00ff00, 0x404040, 0x686868, 0x404040,           // slider
    0xc80000, 0x00ff00, 0xc8c8ff, 0xc80000,                     // debugger
    0x00ff00, 0x20a020, 0x000000, 0x686868, 0x404040            // other
  },
  // Light
  { 0x808080, 0x000000, 0xc0c0c0, 0xe1e1e1, 0x333333,           // base
    0x000000, 0xBDDEF9, 0x0078d7, 0x000000,                     // text
    0xf0f0f0, 0xffffff, 0x0078d7, 0x0f0f0f,                     // UI elements
    0xe1e1e1, 0xe5f1fb, 0x808080, 0x0078d7, 0x000000, 0x000000, // buttons
    0x333333,                                                   // checkbox
    0xc0c0c0, 0x808080,                                         // scrollbar
    0x333333, 0x0078d7, 0xc0c0c0, 0xffffff, 0xc0c0c0,           // slider 0xBDDEF9| 0xe1e1e1 | 0xffffff
    0xffc0c0, 0x000000, 0xe00000, 0xc00000,                     // debugger
    0xffffff, 0x333333, 0xf0f0f0, 0x808080, 0xc0c0c0            // other
  }
};
