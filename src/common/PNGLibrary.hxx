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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
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
class Properties;
class TIA;

#include <fstream>
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
    PNGLibrary(const FrameBuffer& fb);
    virtual ~PNGLibrary();

    /**
      Read a PNG image from the specified file into a FBSurface structure,
      scaling the image to the surface bounds.

      @param filename  The filename to load the PNG image
      @param surface   The FBSurface into which to place the PNG data

      @return  On success, the FBSurface containing image data, otherwise a
               const char* exception is thrown containing a more detailed
               error message.
    */
    void loadImage(const string& filename, FBSurface& surface);

    /**
      Save the current FrameBuffer image to a PNG file.  Note that in most cases
      this will be a TIA image, but it could actually be used for *any* mode.

      @param filename  The filename to save the PNG image
      @param props     The properties object containing info about the ROM

      @return  On success, the PNG file has been saved to 'filename',
               otherwise a const char* exception is thrown containing a
               more detailed error message.
    */
    void saveImage(const string& filename, const Properties& props);

    /**
      Save the current TIA image to a PNG file using data directly from
      the internal TIA buffer.  No filtering or scaling will be included.

      @param filename The filename to save the PNG image
      @param tia      Source of the raw TIA data
      @param props    The properties object containing info about the ROM

      @return  On success, the PNG file has been saved to 'filename',
               otherwise a const char* exception is thrown containing a
               more detailed error message.
    */
    void saveImage(const string& filename, const TIA& tia, const Properties& props);

  private:
    const FrameBuffer& myFB;

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

      @param iwidth  The width of the PNG image
      @param iheight The height of the PNG image
    */
    bool allocateStorage(png_uint_32 iwidth, png_uint_32 iheight);

    /**
      Scale the PNG data from 'ReadInfo' into the FBSurface.  For now, scaling
      is done on integer boundaries only (ie, 1x, 2x, etc up or down).

      @param surface  The FBSurface into which to place the PNG data
    */
    void scaleImagetoSurface(FBSurface& surface);

    /**
      Write PNG tEXt chunks to the image.
    */
    void writeComments(png_structp png_ptr, png_infop info_ptr,
                       const Properties& props);

    /** PNG library callback functions */
    static void png_read_data(png_structp ctx, png_bytep area, png_size_t size);
    static void png_write_data(png_structp ctx, png_bytep area, png_size_t size);
    static void png_io_flush(png_structp ctx);
    static void png_user_warn(png_structp ctx, png_const_charp str);
    static void png_user_error(png_structp ctx, png_const_charp str);
};

#endif
