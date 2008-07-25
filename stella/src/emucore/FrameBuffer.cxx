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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBuffer.cxx,v 1.137 2008-07-25 12:41:41 stephena Exp $
//============================================================================

#include <algorithm>
#include <sstream>

#include "bspf.hxx"

#include "CommandMenu.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "Event.hxx"
#include "Font.hxx"
#include "Launcher.hxx"
#include "MediaSrc.hxx"
#include "Menu.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"

#include "FrameBuffer.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::FrameBuffer(OSystem* osystem)
  : myOSystem(osystem),
    myScreen(0),
    theRedrawTIAIndicator(true),
    myUsePhosphor(false),
    myPhosphorBlend(77),
    myInitializedCount(0),
    myPausedCount(0)
{
  myMsg.surface = myStatsMsg.surface = 0;
  myMsg.enabled = myStatsMsg.enabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::~FrameBuffer(void)
{
  delete myMsg.surface;
  delete myStatsMsg.surface;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::initialize(const string& title, uInt32 width, uInt32 height)
{
  // Now (re)initialize the SDL video system
  // These things only have to be done one per FrameBuffer creation
  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
  {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
      cerr << "ERROR: Couldn't initialize SDL: " << SDL_GetError() << endl;
      return false;
    }
  }
  myInitializedCount++;

  // Set fullscreen flag
#ifdef WINDOWED_SUPPORT
  mySDLFlags = myOSystem->settings().getBool("fullscreen") ? SDL_FULLSCREEN : 0;
#else
  mySDLFlags = 0;
#endif

cerr << " <== FrameBuffer::initialize: w = " << width << ", h = " << height << endl;

  // Set the available video modes for this framebuffer
  setAvailableVidModes(width, height);

  // Initialize video subsystem (make sure we get a valid mode)
  VideoMode mode = getSavedVidMode();
  if(width <= mode.screen_w && height <= mode.screen_h)
  {
    // Set window title and icon
    setWindowTitle(title);
    if(myInitializedCount == 1) setWindowIcon();

    if(!initSubsystem(mode))
    {
      cerr << "ERROR: Couldn't initialize video subsystem" << endl;
      return false;
    }
    else
    {
      myImageRect.setWidth(mode.image_w);
      myImageRect.setHeight(mode.image_h);
      myImageRect.moveTo(mode.image_x, mode.image_y);

      myScreenRect.setWidth(mode.screen_w);
      myScreenRect.setHeight(mode.screen_h);
    }
  }
  else
    return false;

  // And refresh the display
  myOSystem->eventHandler().refreshDisplay();

  // Enable unicode so we can see translated key events
  // (lowercase vs. uppercase characters)
  SDL_EnableUNICODE(1);

  // Erase any messages from a previous run
  myMsg.counter = 0;

  // Create surfaces for TIA statistics and general messages
  myStatsMsg.color = kBtnTextColor;
  myStatsMsg.w = myOSystem->consoleFont().getStringWidth("000 LINES  %00.00 FPS");
  myStatsMsg.h = myOSystem->consoleFont().getFontHeight();
  if(!myStatsMsg.surface)
    myStatsMsg.surface = createSurface(myStatsMsg.w, myStatsMsg.h);
  if(!myMsg.surface)
    myMsg.surface = createSurface(320, 15);  // TODO - size depends on font used

  // Finally, show some information about the framebuffer,
  // but only on the first initialization
  if(myInitializedCount == 1 && myOSystem->settings().getBool("showinfo"))
    cout << about() << endl;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::update()
{
  // Determine which mode we are in (from the EventHandler)
  // Take care of S_EMULATE mode here, otherwise let the GUI
  // figure out what to draw
  switch(myOSystem->eventHandler().state())
  {
    case EventHandler::S_EMULATE:
    {
      // Run the console for one frame
      myOSystem->console().mediaSource().update();
      if(myOSystem->eventHandler().frying())
        myOSystem->console().fry();

      // And update the screen
      drawMediaSource();

      // Show frame statistics
      if(myStatsMsg.enabled)
      {
        // FIXME - sizes hardcoded for now; fix during UI refactoring
        char msg[30];
        sprintf(msg, "%u LINES  %2.2f FPS",
                myOSystem->console().mediaSource().scanlines(),
                myOSystem->console().getFramerate());
        myStatsMsg.surface->fillRect(0, 0, myStatsMsg.w, myStatsMsg.h, kBGColor);
        myStatsMsg.surface->drawString(&myOSystem->consoleFont(), msg, 0, 0,
                                       myStatsMsg.w, myStatsMsg.color, kTextAlignLeft);
        myStatsMsg.surface->addDirtyRect(0, 0, 0, 0);
        myStatsMsg.surface->setPos(myImageRect.x() + 3, myImageRect.y() + 3);
        myStatsMsg.surface->update();
      }
      break;  // S_EMULATE
    }

    case EventHandler::S_PAUSE:
    {
      // Only update the screen if it's been invalidated
      if(theRedrawTIAIndicator)
        drawMediaSource();

      // Show a pause message every 5 seconds
      if(myPausedCount++ >= 7*myOSystem->frameRate())
      {
        myPausedCount = 0;
        showMessage("Paused", kMiddleCenter);
      }
      break;  // S_PAUSE
    }

    case EventHandler::S_MENU:
    {
      // Only update the screen if it's been invalidated
      if(theRedrawTIAIndicator)
        drawMediaSource();

      myOSystem->menu().draw();
      break;  // S_MENU
    }

    case EventHandler::S_CMDMENU:
    {
      // Only update the screen if it's been invalidated
      if(theRedrawTIAIndicator)
        drawMediaSource();

      myOSystem->commandMenu().draw();
      break;  // S_CMDMENU
    }

    case EventHandler::S_LAUNCHER:
    {
      myOSystem->launcher().draw();
      break;  // S_LAUNCHER
    }

#ifdef DEBUGGER_SUPPORT
    case EventHandler::S_DEBUGGER:
    {
      myOSystem->debugger().draw();
      break;  // S_DEBUGGER
    }
#endif

    default:
      return;
      break;
  }

  // Draw any pending messages
  if(myMsg.counter > 0)
    drawMessage();

  // The frame doesn't need to be completely redrawn anymore
  theRedrawTIAIndicator = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showMessage(const string& message, MessagePosition position,
                              int color)
{
  // Erase old messages on the screen
  if(myMsg.counter > 0)
  {
    theRedrawTIAIndicator = true;
    myOSystem->eventHandler().refreshDisplay();
  }

  // Precompute the message coordinates
  myMsg.text    = message;
  myMsg.counter = uInt32(myOSystem->frameRate()) << 1; // Show message for 2 seconds
  myMsg.color   = color;

  myMsg.w = myOSystem->font().getStringWidth(myMsg.text) + 10;
  myMsg.h = myOSystem->font().getFontHeight() + 8;
  myMsg.surface->setWidth(myMsg.w);
  myMsg.surface->setHeight(myMsg.h);

  switch(position)
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFrameStats()
{
  showFrameStats(!myOSystem->settings().getBool("stats"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showFrameStats(bool enable)
{
  myOSystem->settings().setBool("stats", enable);
  myStatsMsg.enabled = enable;
  myOSystem->eventHandler().refreshDisplay();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::enableMessages(bool enable)
{
  if(enable)
  {
    // Only re-anable frame stats if they were already enabled before
    myStatsMsg.enabled = myOSystem->settings().getBool("stats");
  }
  else
  {
    // Temporarily disable frame stats
    myStatsMsg.enabled = false;

    // Erase old messages on the screen
    myMsg.counter = 0;

    myOSystem->eventHandler().refreshDisplay(true);  // Do this twice for
    myOSystem->eventHandler().refreshDisplay(true);  // double-buffered modes
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void FrameBuffer::drawMessage()
{
  // Draw the bounded box and text
  myMsg.surface->setPos(myMsg.x + myImageRect.x(), myMsg.y + myImageRect.y());
  myMsg.surface->fillRect(0, 0, myMsg.w-2, myMsg.h-4, kBGColor);
  myMsg.surface->box(0, 0, myMsg.w, myMsg.h-2, kColor, kShadowColor);
  myMsg.surface->drawString(&myOSystem->font(), myMsg.text, 4, 4,
                               myMsg.w, myMsg.color, kTextAlignLeft);
  myMsg.counter--;

  // Either erase the entire message (when time is reached),
  // or show again this frame
  if(myMsg.counter == 0)  // Force an immediate update
    myOSystem->eventHandler().refreshDisplay(true);
  else
  {
    myMsg.surface->addDirtyRect(0, 0, 0, 0);
    myMsg.surface->update();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::refresh()
{
  theRedrawTIAIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setTIAPalette(const uInt32* palette)
{
  int i, j;

  // Set palette for normal fill
  for(i = 0; i < 256; ++i)
  {
    Uint8 r = (palette[i] >> 16) & 0xff;
    Uint8 g = (palette[i] >> 8) & 0xff;
    Uint8 b = palette[i] & 0xff;

    myDefPalette[i] = mapRGB(r, g, b);
  }

  // Set palette for phosphor effect
  for(i = 0; i < 256; ++i)
  {
    for(j = 0; j < 256; ++j)
    {
      uInt8 ri = (palette[i] >> 16) & 0xff;
      uInt8 gi = (palette[i] >> 8) & 0xff;
      uInt8 bi = palette[i] & 0xff;
      uInt8 rj = (palette[j] >> 16) & 0xff;
      uInt8 gj = (palette[j] >> 8) & 0xff;
      uInt8 bj = palette[j] & 0xff;

      Uint8 r = (Uint8) getPhosphor(ri, rj);
      Uint8 g = (Uint8) getPhosphor(gi, gj);
      Uint8 b = (Uint8) getPhosphor(bi, bj);

      myAvgPalette[i][j] = mapRGB(r, g, b);
    }
  }

  theRedrawTIAIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setUIPalette(const uInt32* palette)
{
  // Set palette for GUI
  for(int i = 0; i < kNumColors-256; ++i)
  {
    Uint8 r = (palette[i] >> 16) & 0xff;
    Uint8 g = (palette[i] >> 8) & 0xff;
    Uint8 b = palette[i] & 0xff;
    myDefPalette[i+256] = mapRGB(r, g, b);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFullscreen()
{
  setFullscreen(!myOSystem->settings().getBool("fullscreen"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setFullscreen(bool enable)
{
#ifdef WINDOWED_SUPPORT
  // Update the settings
  myOSystem->settings().setBool("fullscreen", enable);
  if(enable)
    mySDLFlags |= SDL_FULLSCREEN;
  else
    mySDLFlags &= ~SDL_FULLSCREEN;

  // Do a dummy call to getSavedVidMode to set up the modelists
  // and have it point to the correct 'current' mode
  getSavedVidMode();

  // Do a mode change to the 'current' mode by not passing a '+1' or '-1'
  // to changeVidMode()
  changeVidMode(0);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::changeVidMode(int direction)
{
  EventHandler::State state = myOSystem->eventHandler().state();
  bool inUIMode = (state == EventHandler::S_DEBUGGER ||
                   state == EventHandler::S_LAUNCHER);

  // Ignore any attempts to change video size while in UI mode
  if(inUIMode && direction != 0)
    return false;

  // Only save mode changes in TIA mode with a valid selector
  bool saveModeChange = !inUIMode && (direction == -1 || direction == +1);

  if(direction == +1)
    myCurrentModeList->next();
  else if(direction == -1)
    myCurrentModeList->previous();

  VideoMode vidmode = myCurrentModeList->current(myOSystem->settings());
  if(setVidMode(vidmode))
  {
    myImageRect.setWidth(vidmode.image_w);
    myImageRect.setHeight(vidmode.image_h);
    myImageRect.moveTo(vidmode.image_x, vidmode.image_y);

    myScreenRect.setWidth(vidmode.screen_w);
    myScreenRect.setHeight(vidmode.screen_h);

    if(!inUIMode)
    {
      myOSystem->eventHandler().handleResizeEvent();
      myOSystem->eventHandler().refreshDisplay(true);
      setCursorState();
      showMessage(vidmode.gfxmode.description);
    }
    if(saveModeChange)
      myOSystem->settings().setString("tia_filter", vidmode.gfxmode.name);
  }
  else
    return false;

  return true;
/*
cerr << "New mode:" << endl
	<< "    screen w = " << newmode.screen_w << endl
	<< "    screen h = " << newmode.screen_h << endl
	<< "    image x  = " << newmode.image_x << endl
	<< "    image y  = " << newmode.image_y << endl
	<< "    image w  = " << newmode.image_w << endl
	<< "    image h  = " << newmode.image_h << endl
	<< "    zoom     = " << newmode.zoom << endl
	<< "    name     = " << newmode.name << endl
	<< endl;
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setCursorState()
{
  bool isFullscreen = myOSystem->settings().getBool("fullscreen");

  if(isFullscreen)
    grabMouse(true);
  else
    grabMouse(myOSystem->settings().getBool("grabmouse"));

  switch(myOSystem->eventHandler().state())
  {
    case EventHandler::S_EMULATE:
    case EventHandler::S_PAUSE:
      showCursor(false);
      break;
    default:
      showCursor(true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showCursor(bool show)
{
  SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::grabMouse(bool grab)
{
  SDL_WM_GrabInput(grab ? SDL_GRAB_ON : SDL_GRAB_OFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::fullScreen() const
{
#ifdef WINDOWED_SUPPORT
    return myOSystem->settings().getBool("fullscreen");
#else
    return true;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setWindowTitle(const string& title)
{
  SDL_WM_SetCaption(title.c_str(), "stella");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setWindowIcon()
{
#ifndef MAC_OSX
  #include "stella.xpm"   // The Stella icon

  // Set the window icon
  uInt32 w, h, ncols, nbytes;
  uInt32 rgba[256], icon[32 * 32];
  uInt8  mask[32][4];

  sscanf(stella_icon[0], "%u %u %u %u", &w, &h, &ncols, &nbytes);
  if((w != 32) || (h != 32) || (ncols > 255) || (nbytes > 1))
  {
    cerr << "ERROR: Couldn't load the icon.\n";
    return;
  }

  for(uInt32 i = 0; i < ncols; i++)
  {
    unsigned char code;
    char color[32];
    uInt32 col;

    sscanf(stella_icon[1 + i], "%c c %s", &code, color);
    if(!strcmp(color, "None"))
      col = 0x00000000;
    else if(!strcmp(color, "black"))
      col = 0xFF000000;
    else if (color[0] == '#')
    {
      sscanf(color + 1, "%06x", &col);
      col |= 0xFF000000;
    }
    else
    {
      cerr << "ERROR: Couldn't load the icon.\n";
      return;
    }
    rgba[code] = col;
  }

  memset(mask, 0, sizeof(mask));
  for(h = 0; h < 32; h++)
  {
    const char* line = stella_icon[1 + ncols + h];
    for(w = 0; w < 32; w++)
    {
      icon[w + 32 * h] = rgba[(int)line[w]];
      if(rgba[(int)line[w]] & 0xFF000000)
        mask[h][w >> 3] |= 1 << (7 - (w & 0x07));
    }
  }

  SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(icon, 32, 32, 32,
                         32 * 4, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF000000);
  SDL_WM_SetIcon(surface, (unsigned char *) mask);
  SDL_FreeSurface(surface);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 FrameBuffer::getPhosphor(uInt8 c1, uInt8 c2)
{
  if(c2 > c1)
    BSPF_swap(c1, c2);

  return ((c1 - c2) * myPhosphorBlend)/100 + c2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StringMap& FrameBuffer::supportedTIAFilters(const string& type)
{
  uInt32 max_zoom = maxWindowSizeForScreen(320, 210,
                    myOSystem->desktopWidth(), myOSystem->desktopHeight());
#ifdef SMALL_SCREEN
  uInt32 firstmode = 0;
#else
  uInt32 firstmode = 1;
#endif
  myTIAFilters.clear();
  for(uInt32 i = firstmode; i < GFX_NumModes; ++i)
  {
    // For now, just include all filters
    // This will change once OpenGL-only filters are added
    if(ourGraphicsModes[i].zoom <= max_zoom)
    {
      myTIAFilters.push_back(ourGraphicsModes[i].description,
                             ourGraphicsModes[i].name);
    }
  }
  return myTIAFilters;
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
  // Modelists are different depending on what state we're in
  EventHandler::State state = myOSystem->eventHandler().state();
  bool inUIMode = (state == EventHandler::S_DEBUGGER ||
                   state == EventHandler::S_LAUNCHER);

  myWindowedModeList.clear();
  myFullscreenModeList.clear();

  // In UI/windowed mode, there's only one valid video mode we can use
  if(inUIMode)
  {
    VideoMode m;
    m.image_x = m.image_y = 0;
    m.image_w = m.screen_w = baseWidth;
    m.image_h = m.screen_h = baseHeight;
    m.gfxmode = ourGraphicsModes[0];  // this should be zoom1x

    addVidMode(m);
  }
  else
  {
    // Scan list of filters, adding only those which are appropriate
    // for the given dimensions
    uInt32 max_zoom = maxWindowSizeForScreen(baseWidth, baseHeight,
                      myOSystem->desktopWidth(), myOSystem->desktopHeight());
  #ifdef SMALL_SCREEN
    uInt32 firstmode = 0;
  #else
    uInt32 firstmode = 1;
  #endif
    for(uInt32 i = firstmode; i < GFX_NumModes; ++i)
    {
      uInt32 zoom = ourGraphicsModes[i].zoom;
      if(zoom <= max_zoom)
      {
        VideoMode m;
        m.image_x = m.image_y = 0;
        m.image_w = m.screen_w = baseWidth * zoom;
        m.image_h = m.screen_h = baseHeight * zoom;
        m.gfxmode = ourGraphicsModes[i];

        addVidMode(m);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::addVidMode(VideoMode& mode)
{
  // Windowed modes can be sized exactly as required, since there's normally
  // no restriction on window size (up the maximum size)
  myWindowedModeList.add(mode);

  // There are often stricter requirements on fullscreen modes, and they're
  // normally different depending on the OSystem in use
  // As well, we usually can't get fullscreen modes in the exact size
  // we want, so we need to calculate image offsets
  const ResolutionList& res = myOSystem->supportedResolutions();
  for(uInt32 i = 0; i < res.size(); ++i)
  {
    if(mode.screen_w <= res[i].width && mode.screen_h <= res[i].height)
    {
      // Auto-calculate 'smart' centering; platform-specific framebuffers are
      // free to ignore or augment it
      mode.screen_w = res[i].width;
      mode.screen_h = res[i].height;
      mode.image_x = (mode.screen_w - mode.image_w) >> 1;
      mode.image_y = (mode.screen_h - mode.image_h) >> 1;
      break;
    }
  }
  myFullscreenModeList.add(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::VideoMode FrameBuffer::getSavedVidMode()
{
  EventHandler::State state = myOSystem->eventHandler().state();

  if(myOSystem->settings().getBool("fullscreen"))
    myCurrentModeList = &myFullscreenModeList;
  else
    myCurrentModeList = &myWindowedModeList;

  // Now select the best resolution depending on the state
  // UI modes (launcher and debugger) have only one supported resolution
  // so the 'current' one is the only valid one
  if(state == EventHandler::S_DEBUGGER || state == EventHandler::S_LAUNCHER)
  {
    myCurrentModeList->setByGfxMode(GFX_Zoom1x);
  }
  else
  {
    const string& name = myOSystem->settings().getString("tia_filter");
    myCurrentModeList->setByGfxMode(name);
  }

  return myCurrentModeList->current(myOSystem->settings());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::VideoModeList::VideoModeList()
{
  myIdx = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::VideoModeList::~VideoModeList()
{
  clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::add(VideoMode mode)
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
const FrameBuffer::VideoMode FrameBuffer::
  VideoModeList::current(const Settings& settings) const
{
  // Fullscreen modes are related to the 'fullres' setting
  //   If it's 'auto', we just use the mode as already previously defined
  //   If it's not 'auto', attempt to fit the mode into the resolution
  //   specified by 'fullres' (if possible)
  if(settings.getBool("fullscreen") &&
     BSPF_tolower(settings.getString("fullres")) != "auto")
  {
    // Only use 'fullres' if it's *bigger* than the requested mode
    int w, h;
    settings.getSize("fullres", w, h);

    if(w != -1 && h != -1 && (uInt32)w > myModeList[myIdx].screen_w &&
      (uInt32)h > myModeList[myIdx].screen_h)
    {
      VideoMode mode = myModeList[myIdx];
      mode.screen_w = w;
      mode.screen_h = h;
      mode.image_x = (mode.screen_w - mode.image_w) >> 1;
      mode.image_y = (mode.screen_h - mode.image_h) >> 1;

      return mode;
    }
  }

  // Otherwise, we just use the mode has it was defined in ::addVidMode()
  return myModeList[myIdx];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::next()
{
  myIdx = (myIdx + 1) % myModeList.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::setByGfxMode(GfxID id)
{
  // First we determine which graphics mode is being requested
  bool found = false;
  GraphicsMode gfxmode;
  for(uInt32 i = 0; i < GFX_NumModes; ++i)
  {
    if(ourGraphicsModes[i].type == id)
    {
      gfxmode = ourGraphicsModes[i];
      found = true;
      break;
    }
  }
  if(!found) gfxmode = ourGraphicsModes[0];

  // Now we scan the list for the applicable video mode
  set(gfxmode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::setByGfxMode(const string& name)
{
  // First we determine which graphics mode is being requested
  bool found = false;
  GraphicsMode gfxmode;
  for(uInt32 i = 0; i < GFX_NumModes; ++i)
  {
    if(ourGraphicsModes[i].name == BSPF_tolower(name) ||
       ourGraphicsModes[i].description == BSPF_tolower(name))
    {
      gfxmode = ourGraphicsModes[i];
      found = true;
      break;
    }
  }
  if(!found) gfxmode = ourGraphicsModes[0];

  // Now we scan the list for the applicable video mode
  set(gfxmode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::set(const GraphicsMode& gfxmode)
{
  // Attempt to point the current mode to the one given
  myIdx = -1;

  // First search for the given gfx id
  for(unsigned int i = 0; i < myModeList.size(); ++i)
  {
    if(myModeList[i].gfxmode.type == gfxmode.type)
    {
      myIdx = i;
      return;
    }
  }

  // If we get here, then the gfx type couldn't be found, so we search
  // for the first mode with the same zoomlevel
  for(unsigned int i = 0; i < myModeList.size(); ++i)
  {
    if(myModeList[i].gfxmode.zoom == gfxmode.zoom)
    {
      myIdx = i;
      return;
    }
  }

  // Finally, just pick the lowes video mode
  myIdx = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::print()
{
  cerr << "VideoModeList: " << endl << endl;
  for(Common::Array<VideoMode>::const_iterator i = myModeList.begin();
      i != myModeList.end(); ++i)
  {
    cerr << "  Mode " << i << endl
         << "    screen w = " << i->screen_w << endl
         << "    screen h = " << i->screen_h << endl
         << "    image x  = " << i->image_x << endl
         << "    image y  = " << i->image_y << endl
         << "    image w  = " << i->image_w << endl
         << "    image h  = " << i->image_h << endl
         << "    gfx id   = " << i->gfxmode.type << endl
         << "    gfx name = " << i->gfxmode.name << endl
         << "    gfx desc = " << i->gfxmode.description << endl
         << "    gfx zoom = " << i->gfxmode.zoom << endl
         << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::box(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                    int colorA, int colorB)
{
  hLine(x + 1, y,     x + w - 2, colorA);
  hLine(x,     y + 1, x + w - 1, colorA);
  vLine(x,     y + 1, y + h - 2, colorA);
  vLine(x + 1, y,     y + h - 1, colorA);

  hLine(x + 1,     y + h - 2, x + w - 1, colorB);
  hLine(x + 1,     y + h - 1, x + w - 2, colorB);
  vLine(x + w - 1, y + 1,     y + h - 2, colorB);
  vLine(x + w - 2, y + 1,     y + h - 1, colorB);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::frameRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                          int color, FrameStyle style)
{
  switch(style)
  {
    case kSolidLine:
      hLine(x,         y,         x + w - 1, color);
      hLine(x,         y + h - 1, x + w - 1, color);
      vLine(x,         y,         y + h - 1, color);
      vLine(x + w - 1, y,         y + h - 1, color);
      break;

    case kDashLine:
      unsigned int i, skip, lwidth = 1;

      for(i = x, skip = 1; i < x+w-1; i=i+lwidth+1, ++skip)
      {
        if(skip % 2)
        {
          hLine(i, y,         i + lwidth, color);
          hLine(i, y + h - 1, i + lwidth, color);
        }
      }
      for(i = y, skip = 1; i < y+h-1; i=i+lwidth+1, ++skip)
      {
        if(skip % 2)
        {
          vLine(x,         i, i + lwidth, color);
          vLine(x + w - 1, i, i + lwidth, color);
        }
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::drawString(const GUI::Font* font, const string& s,
                           int x, int y, int w,
                           int color, TextAlignment align,
                           int deltax, bool useEllipsis)
{
  const int leftX = x, rightX = x + w;
  unsigned int i;
  int width = font->getStringWidth(s);
  string str;
	
  if(useEllipsis && width > w)
  {
    // String is too wide. So we shorten it "intelligently", by replacing
    // parts of it by an ellipsis ("..."). There are three possibilities
    // for this: replace the start, the end, or the middle of the string.
    // What is best really depends on the context; but unless we want to
    // make this configurable, replacing the middle probably is a good
    // compromise.
    const int ellipsisWidth = font->getStringWidth("...");
		
    // SLOW algorithm to remove enough of the middle. But it is good enough for now.
    const int halfWidth = (w - ellipsisWidth) / 2;
    int w2 = 0;
		
    for(i = 0; i < s.size(); ++i)
    {
      int charWidth = font->getCharWidth(s[i]);
      if(w2 + charWidth > halfWidth)
        break;

      w2 += charWidth;
      str += s[i];
    }

    // At this point we know that the first 'i' chars are together 'w2'
    // pixels wide. We took the first i-1, and add "..." to them.
    str += "...";
		
    // The original string is width wide. Of those we already skipped past
    // w2 pixels, which means (width - w2) remain.
    // The new str is (w2+ellipsisWidth) wide, so we can accomodate about
    // (w - (w2+ellipsisWidth)) more pixels.
    // Thus we skip ((width - w2) - (w - (w2+ellipsisWidth))) =
    // (width + ellipsisWidth - w)
    int skip = width + ellipsisWidth - w;
    for(; i < s.size() && skip > 0; ++i)
      skip -= font->getCharWidth(s[i]);

    // Append the remaining chars, if any
    for(; i < s.size(); ++i)
      str += s[i];

    width = font->getStringWidth(str);
  }
  else
    str = s;

  if(align == kTextAlignCenter)
    x = x + (w - width - 1)/2;
  else if(align == kTextAlignRight)
    x = x + w - width;

  x += deltax;
  for(i = 0; i < str.size(); ++i)
  {
    w = font->getCharWidth(str[i]);
    if(x+w > rightX)
      break;
    if(x >= leftX)
      drawChar(font, str[i], x, y, color);

    x += w;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::GraphicsMode FrameBuffer::ourGraphicsModes[GFX_NumModes] = {
  { GFX_Zoom1x,  "zoom1x",  "Zoom 1x",  1,  0x3 },
  { GFX_Zoom2x,  "zoom2x",  "Zoom 2x",  2,  0x3 },
  { GFX_Zoom3x,  "zoom3x",  "Zoom 3x",  3,  0x3 },
  { GFX_Zoom4x,  "zoom4x",  "Zoom 4x",  4,  0x3 },
  { GFX_Zoom5x,  "zoom5x",  "Zoom 5x",  5,  0x3 },
  { GFX_Zoom6x,  "zoom6x",  "Zoom 6x",  6,  0x3 },
  { GFX_Zoom7x,  "zoom7x",  "Zoom 7x",  7,  0x3 },
  { GFX_Zoom8x,  "zoom8x",  "Zoom 8x",  8,  0x3 },
  { GFX_Zoom9x,  "zoom9x",  "Zoom 9x",  9,  0x3 },
  { GFX_Zoom10x, "zoom10x", "Zoom 10x", 10, 0x3 }
};
