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
// $Id: Surface.hxx,v 1.1 2007-09-03 18:37:23 stephena Exp $
//============================================================================

#ifndef GUI_SURFACE_HXX
#define GUI_SURFACE_HXX

#include <SDL.h>

namespace GUI {

/**
  This class is basically a thin wrapper around an SDL_Surface structure.
  We do it this way so the SDL stuff won't be dragged into the depths of
  the codebase.

  @author  Stephen Anthony
  @version $Id: Surface.hxx,v 1.1 2007-09-03 18:37:23 stephena Exp $
*/
class Surface
{
  friend class FrameBuffer;

  public:
    Surface(int width, int height, SDL_Surface* surface);
    virtual ~Surface();

    int getWidth() const  { return myBaseWidth;  }
    int getHeight() const { return myBaseHeight; }

  public:
    int myBaseWidth;
    int myBaseHeight;

    SDL_Surface* myData;
};

}  // namespace GUI

#endif
