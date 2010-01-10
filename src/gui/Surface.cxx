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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <SDL.h>

#include "Surface.hxx"

namespace GUI {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Surface::Surface(int width, int height, SDL_Surface* data)
  : myBaseWidth(width),
    myBaseHeight(height),
    myClipWidth(width),
    myClipHeight(height),
    myData(data)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Surface::~Surface()
{
  if(myData)
    SDL_FreeSurface(myData);
  myData = NULL;
}

}  // namespace GUI
