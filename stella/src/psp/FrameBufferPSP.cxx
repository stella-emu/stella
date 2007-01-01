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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrameBufferPSP.cxx,v 1.3 2007-01-01 18:04:55 stephena Exp $
//============================================================================

#include <SDL.h>
#include <SDL_syswm.h>
#include <sstream>

#include "Console.hxx"
#include "FrameBufferPSP.hxx"
#include "MediaSrc.hxx"
#include "Settings.hxx"
#include "OSystem.hxx"
#include "Font.hxx"
#include "GuiUtils.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferPSP::FrameBufferPSP(OSystem* osystem)
  : FrameBufferSoft(osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferPSP::~FrameBufferPSP()
{
  delete myRectList;
  delete myOverlayRectList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferPSP::initSubsystem()
{
  // Set up the rectangle list to be used in the dirty update
  delete myRectList;
  myRectList = new RectList();
  delete myOverlayRectList;
  myOverlayRectList = new RectList();

#ifdef PSP_DEBUG
  fprintf(stdout, "FrameBufferPSP::initSubsystem\n");
#endif
  if(!myRectList || !myOverlayRectList)
  {
    cerr << "ERROR: Unable to get memory for SDL rects" << endl;
    return false;
  }

  // Create the screen
  if(!createScreen())
    return false;

  // Show some info
  if(myOSystem->settings().getBool("showinfo"))
    cout << "Video rendering: Software mode" << endl << endl;

  // Precompute the GUI palette
  // We abuse the concept of 'enum' by referring directly to the integer values
  for(uInt8 i = 0; i < kNumColors-256; i++)
    myPalette[i+256] = mapRGB(ourGUIColors[i][0], ourGUIColors[i][1], ourGUIColors[i][2]);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferPSP::createScreen()
{
  myScreenDim.x = myScreenDim.y = 0;

  myScreenDim.w = myBaseDim.w;
  myScreenDim.h = myBaseDim.h;

  // In software mode, the image and screen dimensions are always the same
  myImageDim = myScreenDim;
  if (mySDLFlags & SDL_HWSURFACE )
  {
    /* double buff is broken */
    mySDLFlags = SDL_HWSURFACE;
    myScreenDim.w = myDesktopDim.w;
    myScreenDim.h = myDesktopDim.w;
#ifdef PSP_DEBUG
    fprintf(stdout, "FrameBufferPSP::createScreen Hardware Mode "
            "myScreenDim.w='%i' myScreenDim.h='%i'\n",
            myScreenDim.w,myScreenDim.h);
#endif
}
  else
{
#ifdef PSP_DEBUG
    fprintf(stdout, "FrameBufferPSP::createScreen Software Mode "
            "myScreenDim.w='%i' myScreenDim.h='%i'\n",
            myScreenDim.w,myScreenDim.h);
#endif
}

  myScreen = SDL_SetVideoMode(myScreenDim.w, myScreenDim.h, 0, mySDLFlags);
  if(myScreen == NULL)
  {
    fprintf(stdout,"ERROR: Unable to open SDL window: %s\n",SDL_GetError());
    return false;
  }
  myOSystem->eventHandler().refreshDisplay();

  return true;
}

