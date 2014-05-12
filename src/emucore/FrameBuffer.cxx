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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <algorithm>
#include <sstream>

#include "bspf.hxx"

#include "CommandMenu.hxx"
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
    myRedrawEntireFrame(true),
    myInitializedCount(0),
    myPausedCount(0),
    myTIASurface(NULL)
{
  myMsg.surface = myStatsMsg.surface = NULL;
  myMsg.enabled = myStatsMsg.enabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::~FrameBuffer(void)
{
  delete myFont;
  delete myInfoFont;
  delete mySmallFont;
  delete myLauncherFont;
  delete myTIASurface;

  // Free all allocated surfaces
  while(!mySurfaceList.empty())
  {
    delete (*mySurfaceList.begin()).second;
    mySurfaceList.erase(mySurfaceList.begin());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::initialize()
{
  // Get desktop resolution and supported renderers
  uInt32 query_w, query_h;
  queryHardware(query_w, query_h, myRenderers);

  // Check the 'maxres' setting, which is an undocumented developer feature
  // that specifies the desktop size (not normally set)
  const GUI::Size& s = myOSystem.settings().getSize("maxres");
  if(s.w > 0 && s.h > 0)
  {
    query_w = s.w;
    query_h = s.h;
  }
  // Various parts of the codebase assume a minimum screen size
  myDesktopSize.w = BSPF_max(query_w, (uInt32)kFBMinW);
  myDesktopSize.h = BSPF_max(query_h, (uInt32)kFBMinH);

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
  bool smallScreen = myDesktopSize.w < kFBMinW ||
                     myDesktopSize.h < kFBMinH;

  // This font is used in a variety of situations when a really small
  // font is needed; we let the specific widget/dialog decide when to
  // use it
  mySmallFont = new GUI::Font(GUI::stellaDesc);

  // The general font used in all UI elements
  // This is determined by the size of the framebuffer
  myFont = new GUI::Font(smallScreen ? GUI::stellaDesc : GUI::stellaMediumDesc);

  // The info font used in all UI elements
  // This is determined by the size of the framebuffer
  myInfoFont = new GUI::Font(smallScreen ? GUI::stellaDesc : GUI::consoleDesc);

  // The font used by the ROM launcher
  // Normally, this is configurable by the user, except in the case of
  // very small screens
  if(!smallScreen)
  {    
    const string& lf = myOSystem.settings().getString("launcherfont");
    if(lf == "small")
      myLauncherFont = new GUI::Font(GUI::consoleDesc);
    else if(lf == "medium")
      myLauncherFont = new GUI::Font(GUI::stellaMediumDesc);
    else
      myLauncherFont = new GUI::Font(GUI::stellaLargeDesc);
  }
  else
    myLauncherFont = new GUI::Font(GUI::stellaDesc);

  // Determine possible TIA windowed zoom levels
  uInt32 maxZoom = maxWindowSizeForScreen((uInt32)kTIAMinW, (uInt32)kTIAMinH,
                     myDesktopSize.w, myDesktopSize.h);

  // Figure our the smallest zoom level we can use
  uInt32 firstZoom = smallScreen ? 1 : 2;
  for(uInt32 zoom = firstZoom; zoom <= maxZoom; ++zoom)
  {
    ostringstream desc;
    desc << "Zoom " << zoom << "x";
    myTIAZoomLevels.push_back(desc.str(), zoom);
  }

  // Set palette for GUI (upper area of array, doesn't change during execution)
  for(int i = 0, j = 256; i < kNumColors-256; ++i, ++j)
  {
    Uint8 r = (ourGUIColors[i] >> 16) & 0xff;
    Uint8 g = (ourGUIColors[i] >> 8) & 0xff;
    Uint8 b = ourGUIColors[i] & 0xff;

    myPalette[j] = mapRGB(r, g, b);
  }
  FBSurface::setPalette(myPalette);

  // Create a TIA surface; we need it for rendering TIA images
  myTIASurface = new TIASurface(*this, myOSystem);

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
    return kFailTooLarge;

// FIXSDL - remove size limitations here?
  if(myOSystem.settings().getString("fullscreen") == "1")
  {
    if(myDesktopSize.w < width || myDesktopSize.h < height)
      return kFailTooLarge;

    useFullscreen = true;
  }
  else
    useFullscreen = false;
#else
  // Make sure this mode is even possible
  // We only really need to worry about it in non-windowed environments,
  // where requesting a window that's too large will probably cause a crash
  if(myDesktopSize.w < width || myDesktopSize.h < height)
    return kFailTooLarge;
#endif

  // Set the available video modes for this framebuffer
  setAvailableVidModes(width, height);

  // Initialize video subsystem (make sure we get a valid mode)
  string pre_about = about();
  const VideoMode& mode = getSavedVidMode(useFullscreen);
  myImageRect = mode.image;
  myScreenSize = mode.screen;
  if(width <= (uInt32)myScreenSize.w && height <= (uInt32)myScreenSize.h)
  {
    if(setVideoMode(myScreenTitle, mode))
    {
      // Inform TIA surface about new mode
      if(myOSystem.eventHandler().state() != EventHandler::S_LAUNCHER &&
         myOSystem.eventHandler().state() != EventHandler::S_DEBUGGER)
        myTIASurface->initialize(myOSystem.console(), mode);

      // Did we get the requested fullscreen state?
      myOSystem.settings().setValue("fullscreen", fullScreen());
      resetSurfaces();
      setCursorState();
    }
    else
    {
      myOSystem.logMessage("ERROR: Couldn't initialize video subsystem", 0);
      return kFailNotSupported;
    }
  }
  else
    return kFailTooLarge;

  // Erase any messages from a previous run
  myMsg.counter = 0;

  // Create surfaces for TIA statistics and general messages
  myStatsMsg.color = kBtnTextColor;
  myStatsMsg.w = infoFont().getMaxCharWidth() * 24 + 2;
  myStatsMsg.h = (infoFont().getFontHeight() + 2) * 2;

  if(myStatsMsg.surface == NULL)
  {
    uInt32 surfaceID = allocateSurface(myStatsMsg.w, myStatsMsg.h);
    myStatsMsg.surface = surface(surfaceID);
  }
  if(myMsg.surface == NULL)
  {
    uInt32 surfaceID = allocateSurface((uInt32)kFBMinW, font().getFontHeight()+10);
    myMsg.surface = surface(surfaceID);
  }

  // Take care of some items that are only done once per framebuffer creation.
  if(myInitializedCount == 1)
  {
    myOSystem.logMessage(about(), 1);
    setWindowIcon();
  }
  else
  {
    string post_about = about();
    if(post_about != pre_about)
      myOSystem.logMessage(post_about, 1);
  }

  return kSuccess;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::update()
{
  // Determine which mode we are in (from the EventHandler)
  // Take care of S_EMULATE mode here, otherwise let the GUI
  // figure out what to draw
  switch(myOSystem.eventHandler().state())
  {
    case EventHandler::S_EMULATE:
    {
      // Run the console for one frame
      // Note that the debugger can cause a breakpoint to occur, which changes
      // the EventHandler state 'behind our back' - we need to check for that
      myOSystem.console().tia().update();
  #ifdef DEBUGGER_SUPPORT
      if(myOSystem.eventHandler().state() != EventHandler::S_EMULATE) break;
  #endif
      if(myOSystem.eventHandler().frying())
        myOSystem.console().fry();

      // And update the screen
      drawTIA();

      // Show frame statistics
      if(myStatsMsg.enabled)
      {
        const ConsoleInfo& info = myOSystem.console().about();
        char msg[30];
        BSPF_snprintf(msg, 30, "%3u @ %3.2ffps => %s",
                myOSystem.console().tia().scanlines(),
                myOSystem.console().getFramerate(), info.DisplayFormat.c_str());
        myStatsMsg.surface->fillRect(0, 0, myStatsMsg.w, myStatsMsg.h, kBGColor);
        myStatsMsg.surface->drawString(infoFont(),
          msg, 1, 1, myStatsMsg.w, myStatsMsg.color, kTextAlignLeft);
        myStatsMsg.surface->drawString(infoFont(),
          info.BankSwitch, 1, 15, myStatsMsg.w, myStatsMsg.color, kTextAlignLeft);
        myStatsMsg.surface->addDirtyRect(0, 0, 0, 0);  // force a full draw
        myStatsMsg.surface->setDstPos(myImageRect.x() + 1, myImageRect.y() + 1);
        myStatsMsg.surface->render();
      }
      break;  // S_EMULATE
    }

    case EventHandler::S_PAUSE:
    {
      // Only update the screen if it's been invalidated
      if(myRedrawEntireFrame)
        drawTIA();

      // Show a pause message every 5 seconds
      if(myPausedCount++ >= 7*myOSystem.frameRate())
      {
        myPausedCount = 0;
        showMessage("Paused", kMiddleCenter);
      }
      break;  // S_PAUSE
    }

    case EventHandler::S_MENU:
    {
      // When onscreen messages are enabled in double-buffer mode,
      // a full redraw is required
      myOSystem.menu().draw(myMsg.enabled && isDoubleBuffered());
      break;  // S_MENU
    }

    case EventHandler::S_CMDMENU:
    {
      // When onscreen messages are enabled in double-buffer mode,
      // a full redraw is required
      myOSystem.commandMenu().draw(myMsg.enabled && isDoubleBuffered());
      break;  // S_CMDMENU
    }

    case EventHandler::S_LAUNCHER:
    {
      // When onscreen messages are enabled in double-buffer mode,
      // a full redraw is required
      myOSystem.launcher().draw(myMsg.enabled && isDoubleBuffered());
      break;  // S_LAUNCHER
    }

#ifdef DEBUGGER_SUPPORT
    case EventHandler::S_DEBUGGER:
    {
      // When onscreen messages are enabled in double-buffer mode,
      // a full redraw is required
      myOSystem.debugger().draw(myMsg.enabled && isDoubleBuffered());
      break;  // S_DEBUGGER
    }
#endif

    default:
      return;
  }

  // Draw any pending messages
  if(myMsg.enabled)
    drawMessage();

  // Do any post-frame stuff
  postFrameUpdate();

  // The frame doesn't need to be completely redrawn anymore
  myRedrawEntireFrame = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showMessage(const string& message, MessagePosition position,
                              bool force)
{
  // Only show messages if they've been enabled
  if(!(force || myOSystem.settings().getBool("uimessages")))
    return;

  // Erase old messages on the screen
  if(myMsg.counter > 0)
  {
    myRedrawEntireFrame = true;
    refresh();
  }

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
void FrameBuffer::toggleFrameStats()
{
  showFrameStats(!myOSystem.settings().getBool("stats"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showFrameStats(bool enable)
{
  myOSystem.settings().setValue("stats", enable);
  myStatsMsg.enabled = enable;
  refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::enableMessages(bool enable)
{
  if(enable)
  {
    // Only re-enable frame stats if they were already enabled before
    myStatsMsg.enabled = myOSystem.settings().getBool("stats");
  }
  else
  {
    // Temporarily disable frame stats
    myStatsMsg.enabled = false;

    // Erase old messages on the screen
    myMsg.enabled = false;
    myMsg.counter = 0;

    refresh();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void FrameBuffer::drawMessage()
{
  // Draw the bounded box and text
  switch(myMsg.position)
  {
    case kTopLeft:
      myMsg.x = 5;
      myMsg.y = 5;
      break;

    case kTopCenter:
      myMsg.x = (myImageRect.width() - myMsg.w) >> 1;
      myMsg.y = 5;
      break;

    case kTopRight:
      myMsg.x = myImageRect.width() - myMsg.w - 5;
      myMsg.y = 5;
      break;

    case kMiddleLeft:
      myMsg.x = 5;
      myMsg.y = (myImageRect.height() - myMsg.h) >> 1;
      break;

    case kMiddleCenter:
      myMsg.x = (myImageRect.width() - myMsg.w) >> 1;
      myMsg.y = (myImageRect.height() - myMsg.h) >> 1;
      break;

    case kMiddleRight:
      myMsg.x = myImageRect.width() - myMsg.w - 5;
      myMsg.y = (myImageRect.height() - myMsg.h) >> 1;
      break;

    case kBottomLeft:
      myMsg.x = 5;
      myMsg.y = myImageRect.height() - myMsg.h - 5;
      break;

    case kBottomCenter:
      myMsg.x = (myImageRect.width() - myMsg.w) >> 1;
      myMsg.y = myImageRect.height() - myMsg.h - 5;
      break;

    case kBottomRight:
      myMsg.x = myImageRect.width() - myMsg.w - 5;
      myMsg.y = myImageRect.height() - myMsg.h - 5;
      break;
  }

  myMsg.surface->setDstPos(myMsg.x + myImageRect.x(), myMsg.y + myImageRect.y());
  myMsg.surface->fillRect(1, 1, myMsg.w-2, myMsg.h-2, kBtnColor);
  myMsg.surface->box(0, 0, myMsg.w, myMsg.h, kColor, kShadowColor);
  myMsg.surface->drawString(font(), myMsg.text, 4, 4,
                            myMsg.w, myMsg.color, kTextAlignLeft);
  myMsg.counter--;

  // Either erase the entire message (when time is reached),
  // or show again this frame
  if(myMsg.counter == 0)  // Force an immediate update
  {
    myMsg.enabled = false;
    refresh();
  }
  else
  {
    myMsg.surface->addDirtyRect(0, 0, 0, 0);
    myMsg.surface->render();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void FrameBuffer::drawTIA()
{
  myTIASurface->render();

  // Let postFrameUpdate() know that a change has been made
//FIXSDL  invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::refresh()
{
  // This method partly duplicates the behaviour in ::update()
  // Here, however, make sure to redraw *all* surfaces applicable to the
  // current EventHandler state
  // We also check for double-buffered modes, and when present
  // update both buffers accordingly
  //
  // This method is in essence a FULL refresh, putting all rendering
  // buffers in a known, fully redrawn state

  switch(myOSystem.eventHandler().state())
  {
    case EventHandler::S_EMULATE:
    case EventHandler::S_PAUSE:
      invalidate();
      drawTIA();
      if(isDoubleBuffered())
      {
        postFrameUpdate();
        invalidate();
        drawTIA();
      }
      break;

    case EventHandler::S_MENU:
      invalidate();
      drawTIA();
      myOSystem.menu().draw(true);
      if(isDoubleBuffered())
      {
        postFrameUpdate();
        invalidate();
        drawTIA();
        myOSystem.menu().draw(true);
      }
      break;

    case EventHandler::S_CMDMENU:
      invalidate();
      drawTIA();
      myOSystem.commandMenu().draw(true);
      if(isDoubleBuffered())
      {
        postFrameUpdate();
        invalidate();
        drawTIA();
        myOSystem.commandMenu().draw(true);
      }
      break;

    case EventHandler::S_LAUNCHER:
      invalidate();
      myOSystem.launcher().draw(true);
      if(isDoubleBuffered())
      {
        postFrameUpdate();
        invalidate();
        myOSystem.launcher().draw(true);
      }
      break;

  #ifdef DEBUGGER_SUPPORT
    case EventHandler::S_DEBUGGER:
      invalidate();
      myOSystem.debugger().draw(true);
      if(isDoubleBuffered())
      {
        postFrameUpdate();
        invalidate();
        myOSystem.debugger().draw(true);
      }
      break;
  #endif

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::allocateSurface(int w, int h)
{
  // Create a new surface
  FBSurface* surface = createSurface(w, h);

  // Add it to the list
  mySurfaceList.insert(make_pair((uInt32)mySurfaceList.size(), surface));

  // Return a reference to it
  return (uInt32)mySurfaceList.size() - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurface* FrameBuffer::surface(uInt32 id) const
{
  map<uInt32,FBSurface*>::const_iterator iter = mySurfaceList.find(id);
  return iter != mySurfaceList.end() ? iter->second : NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::resetSurfaces()
{
  // Free all resources for each surface, then reload them
  // Due to possible timing and/or synchronization issues, all free()'s
  // are done first, then all reload()'s
  // Any derived FrameBuffer classes that call this method should be
  // aware of these restrictions, and act accordingly

  map<uInt32,FBSurface*>::iterator iter;
  for(iter = mySurfaceList.begin(); iter != mySurfaceList.end(); ++iter)
    iter->second->free();
  for(iter = mySurfaceList.begin(); iter != mySurfaceList.end(); ++iter)
    iter->second->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setPalette(const uInt32* palette)
{
cerr << "FrameBuffer::setPalette\n";
  // Set palette for normal fill
  for(int i = 0; i < 256; ++i)
  {
    Uint8 r = (palette[i] >> 16) & 0xff;
    Uint8 g = (palette[i] >> 8) & 0xff;
    Uint8 b = palette[i] & 0xff;

    myPalette[i] = mapRGB(r, g, b);
  }

  // Let the TIA surface know about the new palette
  myTIASurface->setPalette(myPalette, palette);

  myRedrawEntireFrame = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::stateChanged(EventHandler::State state)
{
  // Make sure any onscreen messages are removed
  myMsg.enabled = false;
  myMsg.counter = 0;

  myRedrawEntireFrame = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setFullscreen(bool enable)
{
cerr << "setFullscreen: " << enable << endl;
enableFullscreen(enable);



#if 0 //FIXSDL
#ifdef WINDOWED_SUPPORT
  // '-1' means fullscreen mode is completely disabled
  bool full = enable && myOSystem.settings().getString("fullscreen") != "-1";
  setHint(kFullScreen, full);

  // Do a dummy call to getSavedVidMode to set up the modelists
  // and have it point to the correct 'current' mode
  getSavedVidMode();

  // Do a mode change to the 'current' mode by not passing a '+1' or '-1'
  // to changeVidMode()
  changeVidMode(0);
#endif
#endif
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
  EventHandler::State state = myOSystem.eventHandler().state();
  bool tiaMode = (state != EventHandler::S_DEBUGGER &&
                  state != EventHandler::S_LAUNCHER);

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
  myImageRect = mode.image;
  myScreenSize = mode.screen;
  if(setVideoMode(myScreenTitle, mode))
  {
    resetSurfaces();
    showMessage(mode.description);
    myOSystem.settings().setValue("tia.zoom", mode.zoom);
    refresh();
    return true;
  }
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setCursorState()
{
  // Always grab mouse in fullscreen or during emulation (if enabled),
  // and don't show the cursor during emulation
  bool emulation =
      myOSystem.eventHandler().state() == EventHandler::S_EMULATE;
  grabMouse(fullScreen() ||
    (emulation && myOSystem.settings().getBool("grabmouse")));
  showCursor(!emulation);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleGrabMouse()
{
  bool state = myOSystem.settings().getBool("grabmouse");
  myOSystem.settings().setValue("grabmouse", !state);
  setCursorState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::maxWindowSizeForScreen(uInt32 baseWidth, uInt32 baseHeight,
                    uInt32 screenWidth, uInt32 screenHeight)
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
  myFullscreenModeList.clear();

  // Check if zooming is allowed for this state (currently only allowed
  // for TIA screens)
  EventHandler::State state = myOSystem.eventHandler().state();
  bool tiaMode = (state != EventHandler::S_DEBUGGER &&
                  state != EventHandler::S_LAUNCHER);

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
    if(myDesktopSize.w < 640 || myDesktopSize.h < 480)
      firstZoom = 1;
    for(uInt32 zoom = firstZoom; zoom <= maxZoom; ++zoom)
    {
      ostringstream desc;
      desc << "Zoom " << zoom << "x";
      
      VideoMode mode(baseWidth*zoom, baseHeight*zoom,
              baseWidth*zoom, baseHeight*zoom, false, zoom, desc.str());
      mode.applyAspectCorrection(aspect);
      myWindowedModeList.add(mode);
    }

    // TIA fullscreen mode
    //  GUI::Size screen(myDesktopWidth, myDesktopHeight);
    myFullscreenModeList.add(
        VideoMode(baseWidth, baseHeight, myDesktopSize.w, myDesktopSize.h, true)
    );

  }
  else  // UI mode
  {
    // Windowed and fullscreen mode differ only in screen size
    myWindowedModeList.add(
        VideoMode(baseWidth, baseHeight, baseWidth, baseHeight, false)
    );
    myFullscreenModeList.add(
        VideoMode(baseWidth, baseHeight, myDesktopSize.w, myDesktopSize.h, true)
    );
  }

#if 0 //FIXSDL
cerr << "Windowed modes:\n" << myWindowedModeList << endl
     << "Fullscreen modes:\n" << myFullscreenModeList << endl
     << endl;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const VideoMode& FrameBuffer::getSavedVidMode(bool fullscreen)
{
  EventHandler::State state = myOSystem.eventHandler().state();

  if(fullscreen)
    myCurrentModeList = &myFullscreenModeList;
  else
    myCurrentModeList = &myWindowedModeList;

  // Now select the best resolution depending on the state
  // UI modes (launcher and debugger) have only one supported resolution
  // so the 'current' one is the only valid one
  if(state == EventHandler::S_DEBUGGER || state == EventHandler::S_LAUNCHER)
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
  : fullscreen(false),
    zoom(1),
    description("")
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoMode::VideoMode(uInt32 iw, uInt32 ih, uInt32 sw, uInt32 sh,
                                  bool full, uInt32 z, const string& desc)
  : fullscreen(full),
    zoom(z),
    description(desc)
{
  sw = BSPF_max(sw, (uInt32)FrameBuffer::kTIAMinW);
  sh = BSPF_max(sh, (uInt32)FrameBuffer::kTIAMinH);
  iw = BSPF_min(iw, sw);
  ih = BSPF_min(ih, sh);
  int ix = (sw - iw) >> 1;
  int iy = (sh - ih) >> 1;
  image = GUI::Rect(ix, iy, ix+iw, iy+ih);
  screen = GUI::Size(sw, sh);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoMode::applyAspectCorrection(uInt32 aspect, uInt32 stretch)
{
  // Width is modified by aspect ratio; other factors may be applied below
  uInt32 iw = (uInt32)(float(image.width() * aspect) / 100.0);
  uInt32 ih = image.height();

  if(stretch)
  {
#if 0
    // Fullscreen mode stretching
    if(fullScreen() &&
       (mode.image_w < mode.screen_w) && (mode.image_h < mode.screen_h))
    {
      float stretchFactor = 1.0;
      float scaleX = float(mode.image_w) / mode.screen_w;
      float scaleY = float(mode.image_h) / mode.screen_h;

      // Scale to actual or integral factors
      if(myOSystem.settings().getBool("gl_fsscale"))
      {
        // Scale to full (non-integral) available space
        if(scaleX > scaleY)
          stretchFactor = float(mode.screen_w) / mode.image_w;
        else
          stretchFactor = float(mode.screen_h) / mode.image_h;
      }
      else
      {
        // Only scale to an integral amount
        if(scaleX > scaleY)
        {
          int bw = mode.image_w / mode.gfxmode.zoom;
          stretchFactor = float(int(mode.screen_w / bw) * bw) / mode.image_w;
        }
        else
        {
          int bh = mode.image_h / mode.gfxmode.zoom;
          stretchFactor = float(int(mode.screen_h / bh) * bh) / mode.image_h;
        }
      }
      mode.image_w = (Uint16) (stretchFactor * mode.image_w);
      mode.image_h = (Uint16) (stretchFactor * mode.image_h);
    }
#endif
  }
  else
  {
    // In non-stretch mode, the screen size changes to match the image width
    // Height is never modified in this mode
    screen.w = iw;
  }

  // Now re-calculate the dimensions
  iw = BSPF_min(iw, (uInt32)screen.w);
  ih = BSPF_min(ih, (uInt32)screen.h);

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
FrameBuffer::VideoModeList::~VideoModeList()
{
  clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::add(const VideoMode& mode)
{
  myModeList.push_back(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::clear()
{
  myModeList.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::VideoModeList::isEmpty() const
{
  return myModeList.isEmpty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::VideoModeList::size() const
{
  return myModeList.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::previous()
{
  --myIdx;
  if(myIdx < 0) myIdx = myModeList.size() - 1;
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
    // Base colors
    kColor            Normal foreground color (non-text)
    kBGColor          Normal background color (non-text)
    kShadowColor      Item is disabled
    kTextColor        Normal text color
    kTextColorHi      Highlighted text color
    kTextColorEm      Emphasized text color

    // UI elements (dialog and widgets)
    kDlgColor         Dialog background
    kWidColor         Widget background
    kWidFrameColor    Border for currently selected widget

    // Button colors
    kBtnColor         Normal button background
    kBtnColorHi       Highlighted button background
    kBtnTextColor     Normal button font color
    kBtnTextColorHi   Highlighted button font color

    // Checkbox colors
    kCheckColor       Color of 'X' in checkbox

    // Scrollbar colors
    kScrollColor      Normal scrollbar color
    kScrollColorHi    Highlighted scrollbar color

    // Debugger colors
    kDbgChangedColor      Background color for changed cells
    kDbgChangedTextColor  Text color for changed cells
    kDbgColorHi           Highlighted color in debugger data cells
*/
uInt32 FrameBuffer::ourGUIColors[kNumColors-256] = {
  0x686868, 0x000000, 0x404040, 0x000000, 0x62a108, 0x9f0000,
  0xc9af7c, 0xf0f0cf, 0xc80000,
  0xac3410, 0xd55941, 0xffffff, 0xffd652,
  0xac3410,
  0xac3410, 0xd55941,
  0xac3410, 0xd55941,
  0xc80000, 0x00ff00, 0xc8c8ff
};
