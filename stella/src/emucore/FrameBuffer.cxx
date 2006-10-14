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
// $Id: FrameBuffer.cxx,v 1.91 2006-10-14 20:08:29 stephena Exp $
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::FrameBuffer(OSystem* osystem)
  : myOSystem(osystem),
    myScreen(0),
    theAspectRatio(1.0),
    theRedrawTIAIndicator(true),
    myUsePhosphor(false),
    myPhosphorBlend(77),
    myFrameRate(0)
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
  }
  setWindowIcon();
  cls();

  // Query the desktop size
  // This is really the job of SDL
  int dwidth = 0, dheight = 0;
  myOSystem->getScreenDimensions(dwidth, dheight);
  myDesktopDim.w = dwidth;  myDesktopDim.h = dheight;

  // Set fullscreen flag
  mySDLFlags = myOSystem->settings().getBool("fullscreen") ? SDL_FULLSCREEN : 0;

  // Set window title
  setWindowTitle(title);

  // Get the aspect ratio for the display if it's been enabled
  theAspectRatio = 1.0;
  if(useAspect)
    setAspectRatio();

  // Set the available scalers for this framebuffer, based on current eventhandler
  // state and the maximum size of a window for the current desktop
  setAvailableScalers();

  // Initialize video subsystem
  Scaler scaler;
  getScaler(scaler, 0, currentScalerName());
  setScaler(scaler);
  initSubsystem();

  // And refresh the display
  myOSystem->eventHandler().refreshDisplay();

  // Set palette for GUI
  for(int i = 0; i < kNumColors-256; i++)
    myDefPalette[i+256] = mapRGB(ourGUIColors[i][0], ourGUIColors[i][1], ourGUIColors[i][2]);

  // Enable unicode so we can see translated key events
  // (lowercase vs. uppercase characters)
  SDL_EnableUNICODE(1);

  // Erase any messages from a previous run
  myMessage.counter = 0;

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
      bool mediaSourceChanged = false;

      // Draw changes to the mediasource
      if(!myOSystem->eventHandler().isPaused())
      {
        myOSystem->console().mediaSource().update();
        if(myOSystem->eventHandler().frying())
          myOSystem->console().fry();
          mediaSourceChanged = true;  // mediasource changed, so force an update
      }

      // Only update the screen if it's been invalidated
      if(mediaSourceChanged || theRedrawTIAIndicator)
        drawMediaSource();

      // Draw any pending messages
      if(myMessage.counter > 0 && !myOSystem->eventHandler().isPaused())
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
      if(myMessage.counter > 0 && !myOSystem->eventHandler().isPaused())
        drawMessage();
      break;  // S_CMDMENU
    }

    case EventHandler::S_LAUNCHER:
    {
      myOSystem->launcher().draw();

      // Draw any pending messages
      if(myMessage.counter > 0)
        drawMessage();

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
  myMessage.counter = myFrameRate << 1;   // Show message for 2 seconds
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
void FrameBuffer::hideMessage()
{
  // Erase old messages on the screen
  if(myMessage.counter > 0)
  {
    myMessage.counter = 0;
    myOSystem->eventHandler().refreshDisplay(true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void FrameBuffer::drawMessage()
{
  // Draw the bounded box and text
  fillRect(myMessage.x+1, myMessage.y+2, myMessage.w-2, myMessage.h-4, kBGColor);
  box(myMessage.x, myMessage.y+1, myMessage.w, myMessage.h-2, kColor, kColor);
  drawString(&myOSystem->font(), myMessage.text, myMessage.x+1, myMessage.y+4,
             myMessage.w, myMessage.color, kTextAlignCenter);
  myMessage.counter--;

  // Either erase the entire message (when time is reached),
  // or show again this frame
  if(myMessage.counter == 0)
  {
    // Force an immediate update
    myOSystem->eventHandler().refreshDisplay(true);
  }
  else
    addDirtyRect(myMessage.x, myMessage.y, myMessage.w, myMessage.h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::pause(bool status)
{
  if(&myOSystem->console())
  {
    enablePhosphor(myOSystem->console().properties().get(Display_Phosphor) == "YES");
    setPalette(myOSystem->console().mediaSource().palette());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::refresh()
{
  theRedrawTIAIndicator = true;
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

    myDefPalette[i] = mapRGB(r, g, b);
  }

  // Set palette for phosphor effect
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

  myOSystem->eventHandler().refreshDisplay();
  setCursorState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::scale(int direction, const string& type)
{
  Scaler newScaler;
  const string& currentScaler = (direction == 0 ? type : currentScalerName());
  getScaler(newScaler, direction, currentScaler);

  // Only update the scaler if it's changed from the old one
  if(currentScaler != newScaler.name)
  {
    setScaler(newScaler);
    if(!createScreen())
      return false;

    myOSystem->eventHandler().refreshDisplay();
    showMessage(newScaler.name);

    EventHandler::State state = myOSystem->eventHandler().state();
    bool inTIAMode = (state == EventHandler::S_EMULATE ||
                      state == EventHandler::S_MENU    ||
                      state == EventHandler::S_CMDMENU);

    if(inTIAMode)
      myOSystem->settings().setString("scale_tia", newScaler.name);
    else
      myOSystem->settings().setString("scale_ui", newScaler.name);
  }
  return true;
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
      unsigned int i, skip, lwidth = 0;

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
    SWAP(c1, c2);

  return ((c1 - c2) * myPhosphorBlend)/100 + c2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FrameBuffer::maxWindowSizeForScreen()
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
void FrameBuffer::setAvailableScalers()
{
  /** Different emulation modes support different scalers, and the size
      of the current desktop also determines how much a window can be
      zoomed.  For now, there are two separate scaler lists; one for
      normal emulation mode (and modes that have the TIA in the background),
      and one for standard GUI modes (modes that bypass drawMediaSource).
      For reasons of efficiency, only zooming is supported in GUI mode.  Also,
      it doesn't really make sense to use high quality scalers on text-only
      windows. */

  EventHandler::State state = myOSystem->eventHandler().state();
  int maxsize = maxWindowSizeForScreen();

  // First we disable all scalers
  for(int i = 0; i < kUIScalerListSize; ++i)
    ourUIScalers[i].available = false;
  for(int i = 0; i < kTIAScalerListSize; ++i)
    ourTIAScalers[i].available = false;

  bool inTIAMode = (state == EventHandler::S_EMULATE ||
                    state == EventHandler::S_MENU    ||
                    state == EventHandler::S_CMDMENU);

/*
  cerr << "TIA mode: " << (inTIAMode ? "yes" : "no") << ", state: " << state << endl
       << "maxsize:  " << maxsize << endl
       << "type:     " << (type() == kSoftBuffer ? "soft buffer" : "GL buffer") << endl
       << endl;
*/

  // Next, determine which mode we're in and update the appropriate scaler list
  if(type() == kSoftBuffer)
  {
    if(inTIAMode)
    {
      for(int i = 0; i < kTIAScalerListSize; ++i)
        if(ourTIAScalers[i].scale == 1 && ourTIAScalers[i].zoom <= maxsize)
          ourTIAScalers[i].available = true;
    }
    else  // UI mode
    {
      for(int i = 0; i < kUIScalerListSize; ++i)
        if(ourUIScalers[i].scale == 1 && ourUIScalers[i].zoom <= maxsize)
          ourUIScalers[i].available = true;
    }
  }
  else if(type() == kGLBuffer)
  {
    if(inTIAMode)
    {
      for(int i = 0; i < kTIAScalerListSize; ++i)
        if(ourTIAScalers[i].scale <= maxsize && ourTIAScalers[i].zoom <= maxsize)
          ourTIAScalers[i].available = true;
    }
    else  // UI mode
    {
      for(int i = 0; i < kUIScalerListSize; ++i)
        if(ourUIScalers[i].scale == 1 && ourUIScalers[i].zoom <= maxsize)
          ourUIScalers[i].available = true;
    }
  }
/*
  cerr << "UI Scalers available:\n";
  for(int i = 0; i < 6; ++i)
    if(ourUIScalers[i].available)
      cerr << ourUIScalers[i].name << endl;
  for(int i = 0; i < kNumScalers; ++i)
    if(ourTIAScalers[i].available)
      cerr << ourTIAScalers[i].name << endl;
  cerr << endl << endl;
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::getScaler(Scaler& scaler, int direction, const string& name)
{
  EventHandler::State state = myOSystem->eventHandler().state();
  bool inTIAMode = (state == EventHandler::S_EMULATE ||
                    state == EventHandler::S_MENU    ||
                    state == EventHandler::S_CMDMENU);

  Scaler* list = (inTIAMode ? ourTIAScalers : ourUIScalers);
  int size = (inTIAMode ? kTIAScalerListSize : kUIScalerListSize);

  bool found = false;
  switch(direction)
  {
    case 0:  // search for the actual scaler specified in 'name'
      for(int i = 0; i < size; ++i)
      {
        if(list[i].name == name)
        {
          scaler = list[i];
          found = true;
          break;
        }
      }
      break;

    case -1:   // search for the previous scaler from one specified in 'name'
    {
      int i, pos = -1;

      // First find the current scaler
      for(i = 0; i < size; ++i)
      {
        if(list[i].name == name)
        {
          pos = i;
          break;
        }
      }
      // If we found it, then search for the previous one
      if(pos != -1)
      {
        i = size;
        while(i > 0 && !found)
        {
          pos--;
          if(pos < 0) pos = size - 1;

          if(list[pos].available)
          {
            scaler = list[pos];
            found = true;
            break;
          }
          --i;
        }
      }
      break;
    }

    case +1:   // search for the next scaler from one specified in 'name'
    {
      int i, pos = -1;

      // First find the current scaler
      for(i = 0; i < size; ++i)
      {
        if(list[i].name == name)
        {
          pos = i;
          break;
        }
      }
      // If we found it, then search for the next one
      if(pos != -1)
      {
        i = size;
        while(i > 0 && !found)
        {
          pos++;
          if(pos >= size) pos = 0;

          if(list[pos].available)
          {
            scaler = list[pos];
            found = true;
            break;
          }
          --i;
        }
      }
      break;
    }
  }

  // Default to 'Zoom1x' if we didn't find a valid scaler
  if(!found)
    scaler = list[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& FrameBuffer::currentScalerName()
{
  EventHandler::State state = myOSystem->eventHandler().state();
  bool inTIAMode = (state == EventHandler::S_EMULATE ||
                    state == EventHandler::S_MENU    ||
                    state == EventHandler::S_CMDMENU);

  return (inTIAMode ?
     myOSystem->settings().getString("scale_tia") :
     myOSystem->settings().getString("scale_ui") );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8 FrameBuffer::ourGUIColors[kNumColors-256][3] = {
  { 104, 104, 104 },  // kColor
  {   0,   0,   0 },  // kBGColor
  {  64,  64,  64 },  // kShadowColor
  { 200, 200, 255 },  // kHiliteColor
  {  32, 160,  32 },  // kTextColor
#if !defined(GP2X)  // Quick GP2X hack to change colours, until an in-game GUI is added
  {   0, 255,   0 },  // kTextColorHi
#else
  {   0,   0, 255 },  // kTextColorHi
#endif
  { 200,   0,   0 }   // kTextColorEm
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Scaler FrameBuffer::ourUIScalers[kUIScalerListSize] = {
  { "Zoom1x",  1, 1, false },
  { "Zoom2x",  2, 1, false },
  { "Zoom3x",  3, 1, false },
  { "Zoom4x",  4, 1, false },
  { "Zoom5x",  5, 1, false },
  { "Zoom6x",  6, 1, false }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Scaler FrameBuffer::ourTIAScalers[kTIAScalerListSize] = {
  { "Zoom1x",  1, 1, false },
  { "Zoom2x",  2, 1, false },
  { "Zoom3x",  3, 1, false },
  { "Zoom4x",  4, 1, false },
  { "Zoom5x",  5, 1, false },
  { "Zoom6x",  6, 1, false },

  { "Scale2x", 1, 2, false },
  { "Scale3x", 1, 3, false },
  { "Scale4x", 1, 4, false },

  { "HQ2x",    1, 2, false },
  { "HQ3x",    1, 3, false },
  { "HQ4x",    1, 4, false }
};
