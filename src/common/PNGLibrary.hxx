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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef PNGLIBRARY_HXX
#define PNGLIBRARY_HXX

#include <png.h>

#include "bspf.hxx"
#include "FrameBuffer.hxx"
#include "StringList.hxx"

/**
  This class implements a thin wrapper around the libpng library, and
  abstracts all the irrelevant details other loading and saving an
  actual image.

  @author  Stephen Anthony
*/
class PNGLibrary
{
  public:
    /** Access a PNG image with the given filename. */
    PNGLibrary(const string& filename);
    ~PNGLibrary();

    /**
      Read a PNG image from the previously specified file into a
      FBSurface structure, scaling the image to the surface bounds.

      @param fb      The main framebuffer of the application
      @param surface The FBSurface into which to place the PNG data

      @return  On success, the FBSurface containing image data and a
               result of true, otherwise an exception is thrown.
    */
    bool readImage(const FrameBuffer& fb, FBSurface& surface);

  private:
    /**
      Scale the PNG data from 'buffer' into the FBSurface.  For now, scaling
      is done on integer boundaries only (ie, 1x, 2x, etc up or down).

      @param iwidth  Width of the PNG data (3 bytes per pixel)
      @param iheight Number of row of PNG data
      @param buffer  The PNG data
      @param fb      The main framebuffer of the application
      @param surface The FBSurface into which to place the PNG data
    */
    static void scaleImagetoSurface(uInt32 iwidth, uInt32 iheight, uInt8* buffer,
                                    const FrameBuffer& fb, FBSurface& surface);

    static void png_read_data(png_structp ctx, png_bytep area, png_size_t size);
    static void png_write_data(png_structp ctx, png_bytep area, png_size_t size);
    static void png_io_flush(png_structp ctx);
    static void png_user_warn(png_structp ctx, png_const_charp str);
    static void png_user_error(png_structp ctx, png_const_charp str);

  private:
    string myFilename;
};

#endif
