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
// $Id: Snapshot.hxx,v 1.2 2004-06-20 23:30:48 stephena Exp $
//============================================================================

#ifndef SNAPSHOT_HXX
#define SNAPSHOT_HXX

class FrameBuffer;

#include <png.h>
#include "bspf.hxx"

class Snapshot
{
  public:
    /**
      Create a new shapshot class for taking snapshots in PNG format.

      @param framebuffer The SDL framebuffer containing the image data
    */
    Snapshot(FrameBuffer& framebuffer);

    /**
      The destructor.
    */
    ~Snapshot();

    /**
      This routine saves the current frame buffer to a PNG file.

      @param filename  The filename of the PNG file

      @string  The resulting string to print to the framebuffer
    */
    string savePNG(string filename);

  private:
    static void png_write_data(png_structp ctx, png_bytep area, png_size_t size);

    static void png_io_flush(png_structp ctx);

    static void png_user_warn(png_structp ctx, png_const_charp str);

    static void png_user_error(png_structp ctx, png_const_charp str);

  private:
    // The Framebuffer for the system
    FrameBuffer& myFrameBuffer;
};

#endif
