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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SnapSDL.hxx,v 1.1 2002-02-17 04:41:41 stephena Exp $
//============================================================================

#ifndef SNAPSHOTSDL_HXX
#define SNAPSHOTSDL_HXX

#include <SDL.h>
#include <png.h>


class SnapshotSDL
{
  public:
    SnapshotSDL();
    ~SnapshotSDL();

    int savePNG(SDL_Surface *surface, const char *file);

  private:
    static void png_write_data(png_structp ctx, png_bytep area, png_size_t size);

    static void png_io_flush(png_structp ctx);

    static void png_user_warn(png_structp ctx, png_const_charp str);

    static void png_user_error(png_structp ctx, png_const_charp str);

    int png_colortype_from_surface(SDL_Surface *surface);

    int IMG_SavePNG_RW(SDL_Surface *surface, SDL_RWops *src);
};

#endif
