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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBuffer.cxx,v 1.24 2005-04-04 02:19:21 stephena Exp $
//============================================================================

#include <sstream>

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "StellaEvent.hxx"
#include "Settings.hxx"
#include "MediaSrc.hxx"
#include "FrameBuffer.hxx"
#include "FontData.hxx"
#include "StellaFont.hxx"
#include "GuiUtils.hxx"
#include "Menu.hxx"
#include "OSystem.hxx"

#include "stella.xpm"   // The Stella icon

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::FrameBuffer(OSystem* osystem)
   :  myOSystem(osystem),
      theRedrawEntireFrameIndicator(true),
      theZoomLevel(1),
      theMaxZoomLevel(1),
      theAspectRatio(1.0),
      myFrameRate(0),
      myPauseStatus(false),
      theMenuChangedIndicator(false),
      myMessageTime(0),
      myMessageText(""),
      myMenuRedraws(2),
val(0) // FIXME
{
  // Add the framebuffer to the system
  myOSystem->attach(this);

  // Fill the GUI colors array
  // The specific video subsystem will know what to do with it
  uInt8 colors[5][3] = {
    {104, 104, 104},
    {0, 0, 0},
    {64, 64, 64},
    {32, 160, 32},
    {0, 255, 0}
  };

  for(uInt8 i = 0; i < 5; i++)
    for(uInt8 j = 0; j < 3; j++)
      myGUIColors[i][j] = colors[i][j];

  // Create a font to draw text
  const FontDesc desc = {
    "04b-16b-10",
    9,
    10,
    8,
    33,
    94,
    _font_bits,
    0,  /* no encode table*/
    _sysfont_width,
    33,
    sizeof(_font_bits)/sizeof(uInt16)
  };
  myFont = new StellaFont(this, desc);

  myBaseDim.x = myBaseDim.y = myBaseDim.w = myBaseDim.h = 0;
  myImageDim = myScreenDim = myDesktopDim = myBaseDim;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::~FrameBuffer(void)
{
  if(myFont)
    delete myFont;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::initialize(const string& title, uInt32 width, uInt32 height)
{
  bool isAlreadyInitialized = (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) > 0;

  myBaseDim.w = (uInt16) width;
  myBaseDim.h = (uInt16) height;
  myFrameRate = myOSystem->settings().getInt("framerate");

  // Now (re)initialize the SDL video system
  if(!isAlreadyInitialized)
  {
    Uint32 initflags = SDL_INIT_VIDEO | SDL_INIT_TIMER;

    if(SDL_Init(initflags) < 0)
      return;

    // Calculate the desktop size
    myDesktopDim.w = myDesktopDim.h = 0;

    // Get the system-specific WM information
    SDL_SysWMinfo myWMInfo;
    SDL_VERSION(&myWMInfo.version);
    if(SDL_GetWMInfo(&myWMInfo) > 0)
    {
  #if defined(UNIX)
      if(myWMInfo.subsystem == SDL_SYSWM_X11)
      {
        myWMInfo.info.x11.lock_func();
        myDesktopDim.w = DisplayWidth(myWMInfo.info.x11.display,
                           DefaultScreen(myWMInfo.info.x11.display));
        myDesktopDim.h = DisplayHeight(myWMInfo.info.x11.display,
                           DefaultScreen(myWMInfo.info.x11.display));
        myWMInfo.info.x11.unlock_func();
      }
  #elif defined(WIN32)
      myDesktopDim.w = (uInt16) GetSystemMetrics(SM_CXSCREEN);
      myDesktopDim.h = (uInt16) GetSystemMetrics(SM_CYSCREEN);
  #elif defined(MAC_OSX)
      // FIXME - add OSX Desktop code here (I don't think SDL supports it yet)
  #endif
    }
    setWindowIcon();
  }

  mySDLFlags = myOSystem->settings().getBool("fullscreen") ? SDL_FULLSCREEN : 0;

  // Set window title
  setWindowTitle(title);

  // Initialize video subsystem
  initSubsystem();

  // Show or hide the cursor based on the current state
  setCursorState();
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
        myOSystem->console().mediaSource().update();

      // We always draw the screen, even if the core is paused
      drawMediaSource();

      if(!myPauseStatus)
      {
        // Draw any pending messages
        if(myMessageTime > 0)
        {
          uInt32 w = myFont->getStringWidth(myMessageText) + 10;
          uInt32 h = myFont->getFontHeight() + 8;
          uInt32 x = (myBaseDim.w >> 1) - (w >> 1);
          uInt32 y = myBaseDim.h - h - 10/2;

          // Draw the bounded box and text
          blendRect(x+1, y+2, w-2, h-4, kBGColor);
          box(x, y+1, w, h-2, kColor, kColor);
          myFont->drawString(myMessageText, x+1, y+4, w, kTextColor, kTextAlignCenter);
          myMessageTime--;

          // Erase this message on next update
          if(myMessageTime == 0)
            theRedrawEntireFrameIndicator = true;
        }
      }
      break;  // S_EMULATE
    }

    case EventHandler::S_MENU:
    {
      // Only update the screen if it's been invalidated or the menus have changed  
      if(theRedrawEntireFrameIndicator || theMenuChangedIndicator)
      {
        drawMediaSource();

        // Then overlay any menu items
        myOSystem->menu().draw();

        // Now the screen is up to date
        theRedrawEntireFrameIndicator = false;

        // This is a performance hack to only draw the menus when necessary
        // Software mode is single-buffered, so we don't have to worry
        // However, OpenGL mode is double-buffered, so we need to draw the
        // menus at least twice (so they'll be in both buffers)
        // Otherwise, we get horrible flickering
        myMenuRedraws--;
        theMenuChangedIndicator = (myMenuRedraws != 0);
      }
      break;
    }

    case EventHandler::S_BROWSER:
      // FIXME myOSystem->gui().browser().draw();
      break;

    case EventHandler::S_DEBUGGER:
      // Not yet implemented
      break;

    case EventHandler::S_NONE:
      return;
      break;
  }

  // Do any post-frame stuff
  postFrameUpdate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showMessage(const string& message)
{
  myMessageText = message;
  myMessageTime = myFrameRate << 1;   // Show message for 2 seconds
  theRedrawEntireFrameIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::pause(bool status)
{
  myPauseStatus = status;

  // Now notify the child object, in case it wants to do something
  // special when pause is received
//FIXME  pauseEvent(myPauseStatus); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setupPalette()
{
  // Shade the palette to 75% normal value in pause mode
  float shade = 1.0;
  if(myPauseStatus)
    shade = 0.75;

  const uInt32* gamePalette = myOSystem->console().mediaSource().palette();
  for(uInt32 i = 0; i < 256; ++i)
  {
    Uint8 r, g, b;

    r = (Uint8) (((gamePalette[i] & 0x00ff0000) >> 16) * shade);
    g = (Uint8) (((gamePalette[i] & 0x0000ff00) >> 8) * shade);
    b = (Uint8) ((gamePalette[i] & 0x000000ff) * shade);

    myPalette[i] = mapRGB(r, g, b);
  }

  theRedrawEntireFrameIndicator = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::toggleFullscreen(bool given, bool toggle)
{
  bool isFullscreen;
  if(given)
  {
    if(myOSystem->settings().getBool("fullscreen") == toggle)
      return;
    isFullscreen = toggle;
  }
  else
    isFullscreen = !myOSystem->settings().getBool("fullscreen");

  // Update the settings
  myOSystem->settings().setBool("fullscreen", isFullscreen);

  if(isFullscreen)
    mySDLFlags |= SDL_FULLSCREEN;
  else
    mySDLFlags &= ~SDL_FULLSCREEN;

  if(!createScreen())
    return;

  setCursorState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::resize(Int8 mode, Int8 zoom)
{
  // Use the specific zoom level if one is given
  // Otherwise use 'mode' to pick the next zoom level
  if(zoom != 0)
  {
//    if(myOSystem->settings().getBool("fullscreen"))
//      return;

    if(zoom < 1)
      theZoomLevel = 1;
    else if((uInt32)zoom > theMaxZoomLevel)
      theZoomLevel = theMaxZoomLevel;
    else
      theZoomLevel = zoom;
  }
  else
  {
    // reset size to that given in properties
    // this is a special case of allowing a resize while in fullscreen mode
    if(mode == 0)
    {
      myScreenDim.w = myBaseDim.w;
      myScreenDim.h = myBaseDim.h;
    }
    else if(mode == 1)   // increase size
    {
      if(myOSystem->settings().getBool("fullscreen"))
        return;

      if(theZoomLevel == theMaxZoomLevel)
        theZoomLevel = 1;
      else
        theZoomLevel++;
    }
    else if(mode == -1)   // decrease size
    {
      if(myOSystem->settings().getBool("fullscreen"))
        return;

      if(theZoomLevel == 1)
        theZoomLevel = theMaxZoomLevel;
      else
        theZoomLevel--;
    }
  }

  if(!createScreen())
    return;

  // Update the settings
  myOSystem->settings().setInt("zoom", theZoomLevel);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setCursorState()
{
  bool isFullscreen = myOSystem->settings().getBool("fullscreen");
  if(isFullscreen)
    grabMouse(true);

  switch(myOSystem->eventHandler().state())
  {
    case EventHandler::S_EMULATE:
      if(isFullscreen)
        showCursor(false);
      else
      {
        // Keep mouse in game window if grabmouse is selected
        grabMouse(myOSystem->settings().getBool("grabmouse"));

        // Show or hide the cursor depending on the 'hidecursor' argument
        showCursor(!myOSystem->settings().getBool("hidecursor"));
      }
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
  uInt32 multiplier = sWidth / myBaseDim.w;

  // If screenwidth or height could not be found, use default zoom value
  if(sWidth == 0 || sHeight == 0)
    return 4;

  bool found = false;
  while(!found && (multiplier > 0))
  {
    // Figure out the desired size of the window
    uInt32 width  = myBaseDim.w * multiplier;
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
                            OverlayColor color)
{
  hLine(x,         y,         x + w - 1, color);
  hLine(x,         y + h - 1, x + w - 1, color);
  vLine(x,         y,         y + h - 1, color);
  vLine(x + w - 1, y,         y + h - 1, color);
}

/*
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::showMenu(bool show)
{
  myMenuMode = show;

  myCurrentWidget = show ? MAIN_MENU : W_NONE;
  myRemapEventSelectedFlag = false;
  mySelectedEvent = Event::NoType;
  theRedrawEntireFrameIndicator = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void FrameBuffer::drawMainMenu()
{
  uInt32 x, y, width, height, i, xpos, ypos;

  width  = 16*FONTWIDTH + (FONTWIDTH << 1);
  height = myMainMenuItems*LINEOFFSET + (FONTHEIGHT << 1);
  x = (myWidth >> 1) - (width >> 1);
  y = (myHeight >> 1) - (height >> 1);

  // Draw the bounded box and text, leaving a little room for arrows
  xpos = x + XBOXOFFSET;
  box(x-2, y-2, width+3, height+3, kColor, kBGColor); //FIXME
  for(i = 0; i < myMainMenuItems; i++)
    drawText(xpos, LINEOFFSET*i + y + YBOXOFFSET, ourMainMenu[i].action);

  // Now draw the selection arrow around the currently selected item
  ypos = LINEOFFSET*myMainMenuIndex + y + YBOXOFFSET;
  drawChar(x, ypos, LEFTARROW);
  drawChar(x + width - FONTWIDTH, ypos, RIGHTARROW);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void FrameBuffer::drawRemapMenu()
{
  uInt32 x, y, width, height, xpos, ypos;

  width  = (myWidth >> 3) * FONTWIDTH - (FONTWIDTH << 1);
  height = myMaxRows*LINEOFFSET + (FONTHEIGHT << 1);
  x = (myWidth >> 1) - (width >> 1);
  y = (myHeight >> 1) - (height >> 1);

  // Draw the bounded box and text, leaving a little room for arrows
  box(x-2, y-2, width+3, height+3, kColor, kBGColor); //FIXME
  for(Int32 i = myRemapMenuLowIndex; i < myRemapMenuHighIndex; i++)
  {
    ypos = LINEOFFSET*(i-myRemapMenuLowIndex) + y + YBOXOFFSET;
    drawText(x + XBOXOFFSET, ypos, ourRemapMenu[i].action);

    xpos = width - ourRemapMenu[i].key.length() * FONTWIDTH;
    drawText(xpos, ypos, ourRemapMenu[i].key);
  }

  // Normally draw an arrow indicating the current line,
  // otherwise highlight the currently selected item for remapping
  if(!myRemapEventSelectedFlag)
  {
    ypos = LINEOFFSET*(myRemapMenuIndex-myRemapMenuLowIndex) + y + YBOXOFFSET;
    drawChar(x, ypos, LEFTARROW);
    drawChar(x + width - FONTWIDTH, ypos, RIGHTARROW);
  }
  else
  {
    ypos = LINEOFFSET*(myRemapMenuIndex-myRemapMenuLowIndex) + y + YBOXOFFSET;

    // Left marker is at the beginning of event name text
    xpos = width - ourRemapMenu[myRemapMenuIndex].key.length() * FONTWIDTH - FONTWIDTH;
    drawChar(xpos, ypos, LEFTMARKER);

    // Right marker is at the end of the line
    drawChar(x + width - FONTWIDTH, ypos, RIGHTMARKER);
  }

  // Finally, indicate that there are more items to the top or bottom
  xpos = (width >> 1) - (FONTWIDTH >> 1);
  if(myRemapMenuHighIndex - myMaxRows > 0)
    drawChar(xpos, y, UPARROW);

  if(myRemapMenuLowIndex + myMaxRows < myRemapMenuItems)
    drawChar(xpos, height - (FONTWIDTH >> 1), DOWNARROW);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void FrameBuffer::drawInfoMenu()
{
  uInt32 x, y, width, height, i, xpos;

  width  = myInfoMenuWidth*FONTWIDTH + (FONTWIDTH << 1);
  height = 9*LINEOFFSET + (FONTHEIGHT << 1);
  x = (myWidth >> 1) - (width >> 1);
  y = (myHeight >> 1) - (height >> 1);

  // Draw the bounded box and text
  xpos = x + XBOXOFFSET;
  box(x, y, width, height, kColor, kBGColor); //FIXME
  for(i = 0; i < 9; i++)
    drawText(xpos, LINEOFFSET*i + y + YBOXOFFSET, ourPropertiesInfo[i]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::sendKeyEvent(StellaEvent::KeyCode key, Int32 state)
{
  if(myCurrentWidget == W_NONE || state != 1)
    return;

  // Redraw the menus whenever a key event is received
  theMenuChangedIndicator = true;
  myMenuRedraws = 2;

  // Check which type of widget is pending
  switch(myCurrentWidget)
  {
    case MAIN_MENU:
      if(key == StellaEvent::KCODE_RETURN)
        myCurrentWidget = currentSelectedWidget();
      else if(key == StellaEvent::KCODE_UP)
        moveCursorUp(1);
      else if(key == StellaEvent::KCODE_DOWN)
        moveCursorDown(1);
      else if(key == StellaEvent::KCODE_PAGEUP)
        moveCursorUp(4);
      else if(key == StellaEvent::KCODE_PAGEDOWN)
        moveCursorDown(4);

      break;  // MAIN_MENU

    case REMAP_MENU:
      if(myRemapEventSelectedFlag)
      {
        if(key == StellaEvent::KCODE_ESCAPE)
          deleteBinding(mySelectedEvent);
        else
          addKeyBinding(mySelectedEvent, key);

        myRemapEventSelectedFlag = false;
      }
      else if(key == StellaEvent::KCODE_RETURN)
      {
        mySelectedEvent = currentSelectedEvent();
        myRemapEventSelectedFlag = true;
      }
      else if(key == StellaEvent::KCODE_UP)
        moveCursorUp(1);
      else if(key == StellaEvent::KCODE_DOWN)
        moveCursorDown(1);
      else if(key == StellaEvent::KCODE_PAGEUP)
        moveCursorUp(4);
      else if(key == StellaEvent::KCODE_PAGEDOWN)
        moveCursorDown(4);
      else if(key == StellaEvent::KCODE_ESCAPE)
      {
        myCurrentWidget = MAIN_MENU;
        theRedrawEntireFrameIndicator = true;
      }

      break;  // REMAP_MENU

    case INFO_MENU:
      if(key == StellaEvent::KCODE_ESCAPE)
      {
        myCurrentWidget = MAIN_MENU;
        theRedrawEntireFrameIndicator = true;
      }

      break;  // INFO_MENU

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::sendJoyEvent(StellaEvent::JoyStick stick,
     StellaEvent::JoyCode code, Int32 state)
{
  if(myCurrentWidget == W_NONE || state != 1)
    return;

  // Redraw the menus whenever a joy event is received
  theMenuChangedIndicator = true;

  // Check which type of widget is pending
  switch(myCurrentWidget)
  {
    case MAIN_MENU:
//      if(key == StellaEvent::KCODE_RETURN)
//        myCurrentWidget = currentSelectedWidget();
      if(code == StellaEvent::JAXIS_UP)
        moveCursorUp(1);
      else if(code == StellaEvent::JAXIS_DOWN)
        moveCursorDown(1);

      break;  // MAIN_MENU

    case REMAP_MENU:
      if(myRemapEventSelectedFlag)
      {
        addJoyBinding(mySelectedEvent, stick, code);
        myRemapEventSelectedFlag = false;
      }
      else if(code == StellaEvent::JAXIS_UP)
        moveCursorUp(1);
      else if(code == StellaEvent::JAXIS_DOWN)
        moveCursorDown(1);
//      else if(key == StellaEvent::KCODE_PAGEUP)
//        movePageUp();
//      else if(key == StellaEvent::KCODE_PAGEDOWN)
//        movePageDown();
//      else if(key == StellaEvent::KCODE_ESCAPE)
//        myCurrentWidget = MAIN_MENU;

      break;  // REMAP_MENU

    default:
      break;
  }
}
*/

/*
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::Widget FrameBuffer::currentSelectedWidget()
{
  if(myMainMenuIndex >= 0 && myMainMenuIndex < myMainMenuItems)
    return ourMainMenu[myMainMenuIndex].widget;
  else
    return W_NONE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type FrameBuffer::currentSelectedEvent()
{
  if(myRemapMenuIndex >= 0 && myRemapMenuIndex < myRemapMenuItems)
    return ourRemapMenu[myRemapMenuIndex].event;
  else
    return Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::moveCursorUp(uInt32 amt)
{
  switch(myCurrentWidget)
  {
    case MAIN_MENU:
      if(myMainMenuIndex > 0)
        myMainMenuIndex--;

      break;

    case REMAP_MENU:
      // First move cursor up by the given amt
      myRemapMenuIndex -= amt;

      // Move up the boundaries
      if(myRemapMenuIndex < myRemapMenuLowIndex)
      {
        Int32 x = myRemapMenuLowIndex - myRemapMenuIndex;
        myRemapMenuLowIndex -= x;
        myRemapMenuHighIndex -= x;
      }

      // Then scale back down, if necessary
      if(myRemapMenuLowIndex < 0)
      {
        Int32 x = 0 - myRemapMenuLowIndex;
        myRemapMenuIndex += x;
        myRemapMenuLowIndex += x;
        myRemapMenuHighIndex += x;
      }

      break;

    default:  // This should never happen
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::moveCursorDown(uInt32 amt)
{
  switch(myCurrentWidget)
  {
    case MAIN_MENU:
      if(myMainMenuIndex < myMainMenuItems - 1)
        myMainMenuIndex++;

      break;

    case REMAP_MENU:
      // First move cursor down by the given amount
      myRemapMenuIndex += amt;

      // Move down the boundaries
      if(myRemapMenuIndex >= myRemapMenuHighIndex)
      {
        Int32 x = myRemapMenuIndex - myRemapMenuHighIndex + 1;

        myRemapMenuLowIndex += x;
        myRemapMenuHighIndex += x;
      }

      // Then scale back up, if necessary
      if(myRemapMenuHighIndex >= myRemapMenuItems)
      {
        Int32 x = myRemapMenuHighIndex - myRemapMenuItems;
        myRemapMenuIndex -= x;
        myRemapMenuLowIndex -= x;
        myRemapMenuHighIndex -= x;
      }

      break;

    default:  // This should never happen
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::loadRemapMenu()
{
  // Fill the remap menu with the current key and joystick mappings
  for(Int32 i = 0; i < myRemapMenuItems; ++i)
  {
    Event::Type event = ourRemapMenu[i].event;
    ourRemapMenu[i].key = "None";
    string key = "";
    for(uInt32 j = 0; j < myKeyTableSize; ++j)
    {
      if(myKeyTable[j] == event)
      {
        if(key == "")
          key = key + ourEventName[j];
        else
          key = key + ", " + ourEventName[j];
      }
    }
    for(uInt32 j = 0; j < myJoyTableSize; ++j)
    {
      if(myJoyTable[j] == event)
      {
        ostringstream joyevent;
        uInt32 stick  = j / StellaEvent::LastJCODE;
        uInt32 button = j % StellaEvent::LastJCODE;

        switch(button)
        {
          case StellaEvent::JAXIS_UP:
            joyevent << "J" << stick << " UP";
            break;

          case StellaEvent::JAXIS_DOWN:
            joyevent << "J" << stick << " DOWN";
            break;

          case StellaEvent::JAXIS_LEFT:
            joyevent << "J" << stick << " LEFT";
            break;

          case StellaEvent::JAXIS_RIGHT:
            joyevent << "J" << stick << " RIGHT";
            break;

          default:
            joyevent << "J" << stick << " B" << (button-4);
            break;
        }

        if(key == "")
          key = key + joyevent.str();
        else
          key = key + ", " + joyevent.str();
      }
    }

    if(key != "")
    {
      // 19 is the max size of the event names, and 2 is for the space in between
      // (this could probably be cleaner ...)
      uInt32 len = myMaxColumns - 19 - 2;
      if(key.length() > len)
      {
        ourRemapMenu[i].key = key.substr(0, len - 3) + "...";
      }
      else
        ourRemapMenu[i].key = key;
    }
  }

  // Save the new bindings
  ostringstream keybuf, joybuf;

  // Iterate through the keymap table and create a colon-separated list
  for(uInt32 i = 0; i < StellaEvent::LastKCODE; ++i)
    keybuf << myKeyTable[i] << ":";
  myOSystem->settings().setString("keymap", keybuf.str());

  // Iterate through the joymap table and create a colon-separated list
  for(uInt32 i = 0; i < StellaEvent::LastJSTICK*StellaEvent::LastJCODE; ++i)
    joybuf << myJoyTable[i] << ":";
  myOSystem->settings().setString("joymap", joybuf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::addKeyBinding(Event::Type event, StellaEvent::KeyCode key)
{
  myKeyTable[key] = event;

  loadRemapMenu();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::addJoyBinding(Event::Type event,
       StellaEvent::JoyStick stick, StellaEvent::JoyCode code)
{
  myJoyTable[stick * StellaEvent::LastJCODE + code] = event;

  loadRemapMenu();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::deleteBinding(Event::Type event)
{
  for(uInt32 i = 0; i < myKeyTableSize; ++i)
    if(myKeyTable[i] == event)
      myKeyTable[i] = Event::NoType;

  for(uInt32 j = 0; j < myJoyTableSize; ++j)
    if(myJoyTable[j] == event)
      myJoyTable[j] = Event::NoType;

  loadRemapMenu();
}

*/

#if 0
/**
  This array must be initialized in a specific order, matching
  their initialization in StellaEvent::KeyCode.

  The other option would be to create an array of structures
  (in StellaEvent.hxx) containing event/string pairs.
  This would eliminate the use of enumerations and slow down
  lookups.  So I do it this way instead.
 */
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* FrameBuffer::ourEventName[StellaEvent::LastKCODE] = {
  "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
  "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",

  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",

  "KP 0", "KP 1", "KP 2", "KP 3", "KP 4", "KP 5", "KP 6", "KP 7", "KP 8",
  "KP 9", "KP .", "KP /", "KP *", "KP -", "KP +", "KP ENTER", "KP =",

  "BACKSP", "TAB", "CLEAR", "ENTER", "ESC", "SPACE", ",", "-", ".",
  "/", "\\", ";", "=", "\"", "`", "[", "]",

  "PRT SCRN", "SCR LOCK", "PAUSE", "INS", "HOME", "PGUP",
  "DEL", "END", "PGDN",

  "LCTRL", "RCTRL", "LALT", "RALT", "LWIN", "RWIN", "MENU",
  "UP", "DOWN", "LEFT", "RIGHT",

  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
  "F11", "F12", "F13", "F14", "F15",
};
#endif
