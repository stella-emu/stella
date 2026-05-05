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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifdef IMAGE_SUPPORT

#ifndef JPG_LIBRARY_HXX
#define JPG_LIBRARY_HXX

class OSystem;
class FBSurface;

#include "Variant.hxx"
#include "bspf.hxx"

/**
  This class implements a thin wrapper around the nanojpeg library, and
  abstracts all the irrelevant details other loading an actual image.

  @author  Thomas Jentzsch
*/
class JPGLibrary
{
  public:
    explicit JPGLibrary(OSystem& osystem);
    ~JPGLibrary() = default;

    /**
      Read a JPG image from the specified file into a FBSurface structure,
      scaling the image to the surface bounds.

      @param filename  The filename to load the JPG image
      @param surface   The FBSurface into which to place the JPG data
      @param metaData  The meta data of the JPG image

      @post  On success, the FBSurface containing image data, otherwise a
             std::runtime_error is thrown containing a more detailed
             error message.
    */
    void loadImage(string_view filename, FBSurface& surface, VariantList& metaData);

  private:
    OSystem& myOSystem;

  private:
    // Following constructors and assignment operators not supported
    JPGLibrary() = delete;
    JPGLibrary(const JPGLibrary&) = delete;
    JPGLibrary(JPGLibrary&&) = delete;
    JPGLibrary& operator=(const JPGLibrary&) = delete;
    JPGLibrary& operator=(JPGLibrary&&) = delete;
};

#endif  // JPG_LIBRARY_HXX

#endif  // IMAGE_SUPPORT
