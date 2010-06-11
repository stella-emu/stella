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
      Read a PNG image from the previously specified file.

      @param width  The width of the image that was read
      @param height The height of the image that was read
      @param text   Any tEXt chunks that were present in the input file

      @return  On success, a buffer containing image data, otherwise
               an exception is thrown.  Note that the caller IS NOT
               responsible for deleting the buffer array.
    */
    const uInt8* readImage(uInt32& width, uInt32& height, StringList& text);

  private:
    string myFilename;
};

#endif
