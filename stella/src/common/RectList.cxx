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
// $Id: RectList.cxx,v 1.5 2008-12-28 21:01:55 stephena Exp $
//============================================================================

#include <SDL.h>
#include "bspf.hxx"
#include "RectList.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RectList::RectList(Uint32 size)
{
  currentSize = size;
  currentRect = 0;

  rectArray = new SDL_Rect[currentSize];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RectList::~RectList()
{
  delete[] rectArray;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RectList::add(SDL_Rect* newRect)
{
  if(currentRect >= currentSize)
  {
    currentSize = currentSize * 2;
    SDL_Rect *temp = new SDL_Rect[currentSize];

    for(Uint32 i = 0; i < currentRect; ++i)
      temp[i] = rectArray[i];

    delete[] rectArray;
    rectArray = temp;
  }

  rectArray[currentRect].x = newRect->x;
  rectArray[currentRect].y = newRect->y;
  rectArray[currentRect].w = newRect->w;
  rectArray[currentRect].h = newRect->h;

  ++currentRect;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SDL_Rect* RectList::rects()
{
  return rectArray;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Uint32 RectList::numRects()
{
  return currentRect;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RectList::start()
{
  currentRect = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RectList::print(int boundWidth, int boundHeight)
{
  cerr << "RectList: rects = " << numRects() << endl;
  for(Uint32 i = 0; i < currentRect; ++i)
  {
    cerr << "Rect " << i << endl
         << "  x = " << rectArray[i].x << endl
         << "  y = " << rectArray[i].y << endl
         << "  w = " << rectArray[i].w << endl
         << "  h = " << rectArray[i].h << endl;
    if((rectArray[i].x + rectArray[i].w) > boundWidth ||
       (rectArray[i].y + rectArray[i].h) > boundHeight)
      cerr << " TOO LARGE" << endl;
    cerr << endl;
  }
}
