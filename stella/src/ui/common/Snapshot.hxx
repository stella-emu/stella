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
// $Id: Snapshot.hxx,v 1.3 2003-10-17 18:02:16 stephena Exp $
//============================================================================

#ifndef SNAPSHOT_HXX
#define SNAPSHOT_HXX

#include <png.h>

#include "bspf.hxx"

class FrameBuffer;

class Snapshot
{
  public:
    Snapshot();
    ~Snapshot();

    int savePNG(string filename, FrameBuffer& mediaSource, uInt32 multiplier = 1);

  private:
    static void png_write_data(png_structp ctx, png_bytep area, png_size_t size);

    static void png_io_flush(png_structp ctx);

    static void png_user_warn(png_structp ctx, png_const_charp str);

    static void png_user_error(png_structp ctx, png_const_charp str);

    // The PNG palette
    png_colorp palette;
};

#endif
