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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
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

class FrameBuffer;
class FBSurface;

#include "bspf.hxx"

/**
  This class implements a thin wrapper around the libpng library, and
  abstracts all the irrelevant details other loading and saving an
  actual image.

  @author  Stephen Anthony
*/
class PNGLibrary
{
  public:
    PNGLibrary();
    ~PNGLibrary();

    /**
      Read a PNG image from the specified file into a FBSurface structure,
      scaling the image to the surface bounds.

      @param filename  The filename to load from
      @param fb        The main framebuffer of the application
      @param surface   The FBSurface into which to place the PNG data

      @return  On success, the FBSurface containing image data and a
               result of true, otherwise a const char* exception is thrown
               containing a more detailed error message.
    */
    bool readImage(const string& filename, const FrameBuffer& fb, FBSurface& surface);

  private:
    // The following data remains between invocations of allocateStorage,
    // and is only changed when absolutely necessary.
    typedef struct {
      uInt8* buffer;
      png_bytep* row_pointers;
      png_uint_32 width, height, pitch;
      uInt32* line;
      uInt32 buffer_size, line_size, row_size;
    } ReadInfoType;
    static ReadInfoType ReadInfo;

    /**
      Allocate memory for PNG read operations.  This is used to provide a
      basic memory manager, so that we don't constantly allocate and deallocate
      memory for each image loaded.

      The method fills the 'ReadInfo' struct with valid memory locations
      dependent on the given dimensions.  If memory has been previously
      allocated and it can accommodate the given dimensions, it is used directly.
    */
    bool allocateStorage(png_uint_32 iwidth, png_uint_32 iheight);

    /**
      Scale the PNG data from 'ReadInfo' into the FBSurface.  For now, scaling
      is done on integer boundaries only (ie, 1x, 2x, etc up or down).

      @param fb      The main framebuffer of the application
      @param surface The FBSurface into which to place the PNG data
    */
    void scaleImagetoSurface(const FrameBuffer& fb, FBSurface& surface);

    static void png_read_data(png_structp ctx, png_bytep area, png_size_t size);
    static void png_write_data(png_structp ctx, png_bytep area, png_size_t size);
    static void png_io_flush(png_structp ctx);
    static void png_user_warn(png_structp ctx, png_const_charp str);
    static void png_user_error(png_structp ctx, png_const_charp str);
};

#endif
