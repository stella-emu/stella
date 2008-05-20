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
// $Id: FrameBuffer.cxx,v 1.129 2008-05-20 13:42:50 stephena Exp $
//============================================================================

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
    myPausedCount(0),
    myFrameStatsEnabled(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::~FrameBuffer(void)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::initialize(const string& title, uInt32 width, uInt32 height)
{
  // Now (re)initialize the SDL video system
  // These things only have to be done one per FrameBuffer creation
  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
      return;

  myInitializedCount++;

  myBaseDim.x = myBaseDim.y = 0;
  myBaseDim.w = (uInt16) width;
  myBaseDim.h = (uInt16) height;

  // Set fullscreen flag
#ifdef WINDOWED_SUPPORT
  mySDLFlags = myOSystem->settings().getBool("fullscreen") ? SDL_FULLSCREEN : 0;
#else
  mySDLFlags = 0;
#endif

  // Set the available video modes for this framebuffer
  setAvailableVidModes();

  // Set window title and icon
  setWindowTitle(title);
  if(myInitializedCount == 1) setWindowIcon();

  // Initialize video subsystem
  VideoMode mode = getSavedVidMode();
  initSubsystem(mode);

  // And refresh the display
  myOSystem->eventHandler().refreshDisplay();

  // Enable unicode so we can see translated key events
  // (lowercase vs. uppercase characters)
  SDL_EnableUNICODE(1);

  // Erase any messages from a previous run
  myMessage.counter = 0;

  // Finally, show some information about the framebuffer,
  // but only on the first initialization
  if(myInitializedCount == 1 && myOSystem->settings().getBool("showinfo"))
    cout << about() << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::update()
{
  // Do any pre-frame stuff
  preFrameUpdate();

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
      if(myFrameStatsEnabled)
      {
        // FIXME - sizes hardcoded for now; fix during UI refactoring
        uInt32 scanlines = myOSystem->console().mediaSource().scanlines();
        float fps = (scanlines <= 285 ? 15720.0 : 15600.0) / scanlines;
        char msg[30];
        sprintf(msg, "%u LINES  %2.2f FPS", scanlines, fps);
        fillRect(3, 3, 95, 9, kBGColor);
        drawString(&myOSystem->font(), msg, 3, 3, 95, kBtnTextColor, kTextAlignCenter);
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
  if(myMessage.counter > 0)
    drawMessage();

  // Do any post-frame stuff
  postFrameUpdate();

  // The frame doesn't need to be completely redrawn anymore
  theRedrawTIAIndicator = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showMessage(const string& message, MessagePosition position,
                              int color)
{
  // Erase old messages on the screen
  if(myMessage.counter > 0)
  {
    theRedrawTIAIndicator = true;
    myOSystem->eventHandler().refreshDisplay();
  }

  // Precompute the message coordinates
  myMessage.text    = message;
  myMessage.counter = uInt32(myOSystem->frameRate()) << 1; // Show message for 2 seconds
  myMessage.color   = color;

  myMessage.w = myOSystem->font().getStringWidth(myMessage.text) + 10;
  myMessage.h = myOSystem->font().getFontHeight() + 8;

  switch(position)
  {
    case kTopLeft:
      myMessage.x = 5;
      myMessage.y = 5;
      break;

    case kTopCenter:
      myMessage.x = (myBaseDim.w >> 1) - (myMessage.w >> 1);
      myMessage.y = 5;
      break;

    case kTopRight:
      myMessage.x = myBaseDim.w - myMessage.w - 5;
      myMessage.y = 5;
      break;

    case kMiddleLeft:
      myMessage.x = 5;
      myMessage.y = (myBaseDim.h >> 1) - (myMessage.h >> 1);
      break;

    case kMiddleCenter:
      myMessage.x = (myBaseDim.w >> 1) - (myMessage.w >> 1);
      myMessage.y = (myBaseDim.h >> 1) - (myMessage.h >> 1);
      break;

    case kMiddleRight:
      myMessage.x = myBaseDim.w - myMessage.w - 5;
      myMessage.y = (myBaseDim.h >> 1) - (myMessage.h >> 1);
      break;

    case kBottomLeft:
      myMessage.x = 5;//(myMessage.w >> 1);
      myMessage.y = myBaseDim.h - myMessage.h - 5;
      break;

    case kBottomCenter:
      myMessage.x = (myBaseDim.w >> 1) - (myMessage.w >> 1);
      myMessage.y = myBaseDim.h - myMessage.h - 5;
      break;

    case kBottomRight:
      myMessage.x = myBaseDim.w - myMessage.w - 5;
      myMessage.y = myBaseDim.h - myMessage.h - 5;
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
  myFrameStatsEnabled = enable;
  myOSystem->eventHandler().refreshDisplay(true);  // Do this twice for
  myOSystem->eventHandler().refreshDisplay(true);  // double-buffered modes
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::enableMessages(bool enable)
{
  if(enable)
  {
    // Only re-anable frame stats if they were already enabled before
    myFrameStatsEnabled = myOSystem->settings().getBool("stats");
  }
  else
  {
    // Temporarily disable frame stats
    myFrameStatsEnabled = false;

    // Erase old messages on the screen
    myMessage.counter = 0;

    myOSystem->eventHandler().refreshDisplay(true);  // Do this twice for
    myOSystem->eventHandler().refreshDisplay(true);  // double-buffered modes
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void FrameBuffer::drawMessage()
{
  // Draw the bounded box and text
  fillRect(myMessage.x+1, myMessage.y+2, myMessage.w-2, myMessage.h-4, kBGColor);
  box(myMessage.x, myMessage.y+1, myMessage.w, myMessage.h-2, kColor, kShadowColor);
  drawString(&myOSystem->font(), myMessage.text, myMessage.x+1, myMessage.y+4,
             myMessage.w, myMessage.color, kTextAlignCenter);
  myMessage.counter--;

  // Either erase the entire message (when time is reached),
  // or show again this frame
  if(myMessage.counter == 0)  // Force an immediate update
    myOSystem->eventHandler().refreshDisplay(true);
  else
    addDirtyRect(myMessage.x, myMessage.y, myMessage.w, myMessage.h);
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
  bool saveModeChange = true;

  VideoMode oldmode = myCurrentModeList->current();
  if(direction == +1)
    myCurrentModeList->next();
  else if(direction == -1)
    myCurrentModeList->previous();
  else
    saveModeChange = false;  // no resolution or zoom level actually changed

  VideoMode newmode = myCurrentModeList->current();
  if(!setVidMode(newmode))
    return false;

  myOSystem->eventHandler().handleResizeEvent();
  myOSystem->eventHandler().refreshDisplay(true);
  setCursorState();
  showMessage(newmode.name);

  if(saveModeChange)
  {
    // Determine which mode we're in, and save to the appropriate setting
    if(fullScreen())
    {
      myOSystem->settings().setSize("fullres", newmode.screen_w, newmode.screen_h);
    }
    else
    {
      EventHandler::State state = myOSystem->eventHandler().state();
      bool inTIAMode = (state == EventHandler::S_EMULATE ||
                        state == EventHandler::S_PAUSE   ||
                        state == EventHandler::S_MENU    ||
                        state == EventHandler::S_CMDMENU);

      if(inTIAMode)
        myOSystem->settings().setInt("zoom_tia", newmode.zoom);
      else
        myOSystem->settings().setInt("zoom_ui", newmode.zoom);
    }
  }
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

  sscanf(stella_icon[0], "%d %d %d %d", &w, &h, &ncols, &nbytes);
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
void FrameBuffer::box(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
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
void FrameBuffer::frameRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
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
void FrameBuffer::drawString(const GUI::Font* font, const string& s,
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
uInt8 FrameBuffer::getPhosphor(uInt8 c1, uInt8 c2)
{
  if(c2 > c1)
    BSPF_swap(c1, c2);

  return ((c1 - c2) * myPhosphorBlend)/100 + c2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::maxWindowSizeForScreen(uInt32 screenWidth, uInt32 screenHeight)
{
  uInt32 multiplier = 1;
  for(;;)
  {
    // Figure out the zoomed size of the window
    uInt32 width  = myBaseDim.w * multiplier;
    uInt32 height = myBaseDim.h * multiplier;

    if((width > screenWidth) || (height > screenHeight))
      break;

    ++multiplier;
  }
  return multiplier > 1 ? multiplier - 1 : 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setAvailableVidModes()
{
  // First we look at windowed modes
  // These can be sized exactly as required, since there's normally no
  // restriction on window size (up the maximum size)
  myWindowedModeList.clear();
  int max_zoom = maxWindowSizeForScreen(myOSystem->desktopWidth(),
                                        myOSystem->desktopHeight());
  for(int i = 1; i <= max_zoom; ++i)
  {
    VideoMode m;
    m.image_x = m.image_y = 0;
    m.image_w = m.screen_w = myBaseDim.w * i;
    m.image_h = m.screen_h = myBaseDim.h * i;
    m.zoom = i;
    ostringstream buf;
    buf << "Zoom " << i << "x";
    m.name = buf.str();

    myWindowedModeList.add(m);
  }

  // Now consider the fullscreen modes
  // There are often stricter requirements on these, and they're normally
  // different depending on the OSystem in use
  // As well, we usually can't get fullscreen modes in the exact size
  // we want, so we need to calculate image offsets
  myFullscreenModeList.clear();
  const ResolutionList& res = myOSystem->supportedResolutions();
  for(unsigned int i = 0; i < res.size(); ++i)
  {
    VideoMode m;
    m.screen_w = res[i].width;
    m.screen_h = res[i].height;
    m.zoom = maxWindowSizeForScreen(m.screen_w, m.screen_h);
    m.name = res[i].name;

    // Auto-calculate 'smart' centering; platform-specific framebuffers are
    // free to ignore or augment it
    m.image_w = myBaseDim.w * m.zoom;
    m.image_h = myBaseDim.h * m.zoom;
    m.image_x = (m.screen_w - m.image_w) / 2;
    m.image_y = (m.screen_h - m.image_h) / 2;

/*
cerr << "Fullscreen modes:" << endl
	<< "  Mode " << i << endl
	<< "    screen w = " << m.screen_w << endl
	<< "    screen h = " << m.screen_h << endl
	<< "    image x  = " << m.image_x << endl
	<< "    image y  = " << m.image_y << endl
	<< "    image w  = " << m.image_w << endl
	<< "    image h  = " << m.image_h << endl
	<< "    zoom     = " << m.zoom << endl
	<< endl;
*/
    myFullscreenModeList.add(m);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoMode FrameBuffer::getSavedVidMode()
{
  EventHandler::State state = myOSystem->eventHandler().state();

  if(myOSystem->settings().getBool("fullscreen"))
  {
    // Point the modelist to fullscreen modes, and set the iterator to
    // the mode closest to the given resolution
    int w = -1, h = -1;
    myOSystem->settings().getSize("fullres", w, h);
    if(w < 0 || h < 0)
    {
      w = myOSystem->desktopWidth();
      h = myOSystem->desktopHeight();
    }

    // The launcher and debugger modes are different, in that their size is
    // set at program launch and can't be changed
    // In these cases, the resolution must accommodate their size
    if(state == EventHandler::S_LAUNCHER)
    {
      int lw, lh;
      myOSystem->settings().getSize("launcherres", lw, lh);
      w = BSPF_max(w, lw);
      h = BSPF_max(h, lh);
    }
#ifdef DEBUGGER_SUPPORT
    else if(state == EventHandler::S_DEBUGGER)
    {
      int lw, lh;
      myOSystem->settings().getSize("debuggerres", lw, lh);
      w = BSPF_max(w, lw);
      h = BSPF_max(h, lh);
    }
#endif

    myCurrentModeList = &myFullscreenModeList;
    myCurrentModeList->setByResolution(w, h);
  }
  else
  {
    // Point the modelist to windowed modes, and set the iterator to
    // the mode closest to the given zoom level
    bool inTIAMode = (state == EventHandler::S_EMULATE ||
                      state == EventHandler::S_PAUSE   ||
                      state == EventHandler::S_MENU    ||
                      state == EventHandler::S_CMDMENU);
    int zoom = (inTIAMode ? myOSystem->settings().getInt("zoom_tia") :
                            myOSystem->settings().getInt("zoom_ui") );

    myCurrentModeList = &myWindowedModeList;
    myCurrentModeList->setByZoom(zoom);
  }

  return myCurrentModeList->current();
}
