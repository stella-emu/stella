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
// $Id: FrameBuffer.cxx,v 1.71 2006-01-10 20:37:00 stephena Exp $
//============================================================================

#include <sstream>

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "Settings.hxx"
#include "MediaSrc.hxx"
#include "FrameBuffer.hxx"
#include "Font.hxx"
#include "GuiUtils.hxx"
#include "Menu.hxx"
#include "CommandMenu.hxx"
#include "Launcher.hxx"
#include "OSystem.hxx"

#ifdef DEVELOPER_SUPPORT
  #include "Debugger.hxx"
#endif

#if defined(OS2) // FIXME - make proper OS/2 port
  #define INCL_WIN
  #include <os2emx.h>
#endif

#include "stella.xpm"   // The Stella icon

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::FrameBuffer(OSystem* osystem)
  : myOSystem(osystem),
    theRedrawTIAIndicator(true),
    theZoomLevel(2),
    theMaxZoomLevel(2),
    theAspectRatio(1.0),
    myUsePhosphor(false),
    myPhosphorBlend(77),
    myFrameRate(0),
    myPauseStatus(false),
    myMessageTime(0),
    myMessageText(""),
    myNumRedraws(0)
{
  myBaseDim.x = myBaseDim.y = myBaseDim.w = myBaseDim.h = 0;
  myImageDim = myScreenDim = myDesktopDim = myBaseDim;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::~FrameBuffer(void)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::initialize(const string& title, uInt32 width, uInt32 height,
                             bool useAspect)
{
  bool isAlreadyInitialized = (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) > 0;

  myBaseDim.w = (uInt16) width;
  myBaseDim.h = (uInt16) height;
  myFrameRate = myOSystem->frameRate();

  // Now (re)initialize the SDL video system
  if(!isAlreadyInitialized)
  {
    Uint32 initflags = SDL_INIT_VIDEO | SDL_INIT_TIMER;

    if(SDL_Init(initflags) < 0)
      return;

    setWindowIcon();
  }

  // Query the desktop size
  // This is really the job of SDL
  myDesktopDim.w = myDesktopDim.h = 0;
#if defined(OS2)  // FIXME - make proper OS/2 port
  myDesktopDim.w = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
  myDesktopDim.h = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
#else
  myOSystem->getScreenDimensions((int&)myDesktopDim.w, (int&)myDesktopDim.h);
#endif

  // Set fullscreen flag
  mySDLFlags = myOSystem->settings().getBool("fullscreen") ? SDL_FULLSCREEN : 0;

  // Set window title
  setWindowTitle(title);

  // Get the aspect ratio for the display if it's been enabled
  theAspectRatio = 1.0;
  if(useAspect)
    setAspectRatio();

  // Get the maximum size of a window for the current desktop
  theMaxZoomLevel = maxWindowSizeForScreen();

  // Check to see if window size will fit in the screen
  if((uInt32)myOSystem->settings().getInt("zoom") > theMaxZoomLevel)
    theZoomLevel = theMaxZoomLevel;
  else
    theZoomLevel = myOSystem->settings().getInt("zoom");

  // Initialize video subsystem
  initSubsystem();

  // Set emulation palette if a console exists
  // Used when entering/exiting debugger
#ifdef DEVELOPER_SUPPORT
  if(&myOSystem->console())
  {
    enablePhosphor(myOSystem->console().properties().get("Display.Phosphor", true) == "YES");
    setPalette(myOSystem->console().mediaSource().palette());
  }
#endif

  // Enable unicode so we can see translated key events
  // (lowercase vs. uppercase characters)
  SDL_EnableUNICODE(1);

  // Erase any messages from a previous run
  myMessageTime = 0;

  myUseDirtyRects = myOSystem->settings().getBool("dirtyrects");
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
      // Draw changes to the mediasource
      if(!myPauseStatus)
      {
        myOSystem->console().mediaSource().update();
        if(myOSystem->eventHandler().frying())
          myOSystem->console().fry();
      }

      // We always draw the screen, even if the core is paused
      drawMediaSource();

      // Draw any pending messages
      if(myMessageTime > 0 && !myPauseStatus)
        drawMessage();
      break;  // S_EMULATE
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

      // Draw any pending messages
      if(myMessageTime > 0 && !myPauseStatus)
        drawMessage();
      break;  // S_CMDMENU
    }

    case EventHandler::S_LAUNCHER:
    {
      myOSystem->launcher().draw();
      break;  // S_LAUNCHER
    }

#ifdef DEVELOPER_SUPPORT
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

  // Do any post-frame stuff
  postFrameUpdate();

  // The frame doesn't need to be completely redrawn anymore
  theRedrawTIAIndicator = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::refresh(bool now)
{
//  cerr << "refreshTIA() " << myNumRedraws++ << endl;
  theRedrawTIAIndicator = true;
  if(now)
  {
    myMessageTime = 0;
    update();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showMessage(const string& message)
{
  // Erase old messages on the screen
  if(myMessageTime > 0)
  {
    theRedrawTIAIndicator = true;
    drawMediaSource();
  }

  myMessageText = message;
  myMessageTime = myFrameRate << 1;   // Show message for 2 seconds
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::hideMessage()
{
  // Erase old messages on the screen
  if(myMessageTime > 0)
    refresh(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::pause(bool status)
{
  myPauseStatus = status;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setPalette(const uInt32* palette)
{
  int i, j;

  // Set palette for normal fill
  for(i = 0; i < 256; ++i)
  {
    Uint8 r = (Uint8) ((palette[i] & 0x00ff0000) >> 16);
    Uint8 g = (Uint8) ((palette[i] & 0x0000ff00) >> 8);
    Uint8 b = (Uint8) (palette[i] & 0x000000ff);

    myPalette[i] = mapRGB(r, g, b);
  }

  // Set palette for phosphor effect
  if(myUsePhosphor)
  {
    for(i = 0; i < 256; ++i)
    {
      for(j = 0; j < 256; ++j)
      {
        uInt8 ri = (uInt8) ((palette[i] & 0x00ff0000) >> 16);
        uInt8 gi = (uInt8) ((palette[i] & 0x0000ff00) >> 8);
        uInt8 bi = (uInt8) (palette[i] & 0x000000ff);
        uInt8 rj = (uInt8) ((palette[j] & 0x00ff0000) >> 16);
        uInt8 gj = (uInt8) ((palette[j] & 0x0000ff00) >> 8);
        uInt8 bj = (uInt8) (palette[j] & 0x000000ff);

        Uint8 r = (Uint8) getPhosphor(ri, rj);
        Uint8 g = (Uint8) getPhosphor(gi, gj);
        Uint8 b = (Uint8) getPhosphor(bi, bj);

        myAvgPalette[i][j] = mapRGB(r, g, b);
      }
    }
  }

  theRedrawTIAIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFullscreen()
{
  setFullscreen(!myOSystem->settings().getBool("fullscreen"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setFullscreen(bool enable)
{
  // Update the settings
  myOSystem->settings().setBool("fullscreen", enable);

  if(enable)
    mySDLFlags |= SDL_FULLSCREEN;
  else
    mySDLFlags &= ~SDL_FULLSCREEN;

  if(!createScreen())
    return;

  setCursorState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::resize(Size size, Int8 zoom)
{
  switch(size)
  {
    case PreviousSize:   // decrease size
      if(myOSystem->settings().getBool("fullscreen"))
        return;
      if(theZoomLevel == 1)
        theZoomLevel = theMaxZoomLevel;
      else
        theZoomLevel--;
      break;

    case NextSize:       // increase size
      if(myOSystem->settings().getBool("fullscreen"))
        return;
      if(theZoomLevel == theMaxZoomLevel)
        theZoomLevel = 1;
      else
        theZoomLevel++;
      break;

    case GivenSize:      // use 'zoom' quantity
      if(zoom < 1)
        theZoomLevel = 1;
      else if((uInt32)zoom > theMaxZoomLevel)
        theZoomLevel = theMaxZoomLevel;
      else
        theZoomLevel = zoom;
      break;

    default:   // should never happen
      return;
      break;
  }

  if(!createScreen())
    return;
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
      showCursor(false);
      break;

    default:
      showCursor(true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showCursor(bool show)
{
  if(show)
    SDL_ShowCursor(SDL_ENABLE);
  else
    SDL_ShowCursor(SDL_DISABLE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::grabMouse(bool grab)
{
  if(grab)
    SDL_WM_GrabInput(SDL_GRAB_ON);
  else
    SDL_WM_GrabInput(SDL_GRAB_OFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::fullScreen()
{
  return myOSystem->settings().getBool("fullscreen");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::maxWindowSizeForScreen()
{
  uInt32 sWidth     = myDesktopDim.w;
  uInt32 sHeight    = myDesktopDim.h;
  uInt32 multiplier = 10;

  // If screenwidth or height could not be found, use default zoom value
  if(sWidth == 0 || sHeight == 0)
    return 4;

  bool found = false;
  while(!found && (multiplier > 0))
  {
    // Figure out the desired size of the window
    uInt32 width  = (uInt32) (myBaseDim.w * multiplier * theAspectRatio);
    uInt32 height = myBaseDim.h * multiplier;

    if((width < sWidth) && (height < sHeight))
      found = true;
    else
      multiplier--;
  }

  if(found)
    return multiplier;
  else
    return 1;
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
inline void FrameBuffer::drawMessage()
{
  int w = myOSystem->font().getStringWidth(myMessageText) + 10;
  int h = myOSystem->font().getFontHeight() + 8;
  int x = (myBaseDim.w >> 1) - (w >> 1);
  int y = myBaseDim.h - h - 10/2;

  // Draw the bounded box and text
  fillRect(x+1, y+2, w-2, h-4, kBGColor);
  box(x, y+1, w, h-2, kColor, kColor);
  drawString(&myOSystem->font(), myMessageText, x+1, y+4, w, kTextColor, kTextAlignCenter);
  myMessageTime--;

  // Either erase the entire message (when time is reached),
  // or show again this frame
  if(myMessageTime == 0)
  {
	theRedrawTIAIndicator = true;
	drawMediaSource();
  }
  else
	addDirtyRect(x, y, w, h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::box(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                      OverlayColor colorA, OverlayColor colorB)
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
                            OverlayColor color, FrameStyle style)
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
                             OverlayColor color, TextAlignment align,
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
    SWAP(c1, c2);

  return ((c1 - c2) * myPhosphorBlend)/100 + c2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8 FrameBuffer::ourGUIColors[kNumColors-256][3] = {
  { 104, 104, 104 },  // kColor
  {   0,   0,   0 },  // kBGColor
  {  64,  64,  64 },  // kShadowColor
  { 200, 200, 255 },  // kHiliteColor
  {  32, 160,  32 },  // kTextColor
  {   0, 255,   0 },  // kTextColorHi
  { 200,   0,   0 }   // kTextColorEm
};
