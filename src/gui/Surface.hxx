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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef GUI_SURFACE_HXX
#define GUI_SURFACE_HXX

#include <SDL.h>

namespace GUI {

/**
  This class is basically a thin wrapper around an SDL_Surface structure.
  We do it this way so the SDL stuff won't be dragged into the depths of
  the codebase.  Although everything is public and SDL structures can be
  used directly, it's recommended to only access the variables from the
  advertised interface, or from FrameBuffer-derived classes.

  @author  Stephen Anthony
  @version $Id$
*/
class Surface
{
  public:
    Surface(int width, int height, SDL_Surface* surface);
    virtual ~Surface();

    /** Actual width and height of the SDL surface */
    inline int getWidth() const  { return myBaseWidth;  }
    inline int getHeight() const { return myBaseHeight; }

    /** Clipped/drawn width and height of the SDL surface */
    inline int getClipWidth() const  { return myClipWidth;  }
    inline int getClipHeight() const { return myClipHeight; }
    inline void setClipWidth(int w)  { myClipWidth = w;  }
    inline void setClipHeight(int h) { myClipHeight = h; }

  public:
    int myBaseWidth;
    int myBaseHeight;

    int myClipWidth;
    int myClipHeight;

    SDL_Surface* myData;
};

}  // namespace GUI

#endif
