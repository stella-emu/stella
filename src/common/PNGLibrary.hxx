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

#ifndef PNGLIBRARY_HXX
#define PNGLIBRARY_HXX

#include <fstream>
#include <bit>
#include <png.h>

#include "bspf.hxx"
#include "Variant.hxx"

class OSystem;
class FBSurface;
class FSNode;

/**
  This class implements a thin wrapper around the libpng library, and
  abstracts all the irrelevant details of loading and saving an actual image.

  @author  Stephen Anthony
*/
class PNGLibrary
{
  public:
    explicit PNGLibrary(OSystem& osystem);
    ~PNGLibrary() = default;

    /**
      Read a PNG image from the specified file into a FBSurface structure,
      scaling the image to the surface bounds.

      @param filename  The filename to load the PNG image
      @param surface   The FBSurface into which to place the PNG data
      @param metaData  The meta data of the PNG image

      @post  On success, the FBSurface containing image data, otherwise a
             std::runtime_error is thrown containing a more detailed
             error message.
    */
    void loadImage(string_view filename, FBSurface& surface,
                   VariantList& metaData);

    /**
      Save the given surface to a PNG file.

      @param filename  The filename to save the PNG image
      @param surface   The surface data for the PNG image
      @param rect      The area of the surface to use
      @param metaData  The meta data to add to the PNG image

      @post  On success, the PNG file has been saved to 'filename',
             otherwise a std::runtime_error is thrown containing a
             more detailed error message.
    */
    void saveImage(string_view filename, const FBSurface& surface,
                   const Common::Rect& rect = Common::Rect{},
                   const VariantList& metaData = VariantList{});

    /**
      Called at regular intervals, and used to determine whether a
      continuous snapshot is due to be taken.

      @param time  The current time in microseconds
    */
    void updateTime(uInt64 time);

    /**
      Answer whether continuous snapshot mode is enabled.
    */
    bool continuousSnapEnabled() const { return mySnapInterval > 0; }

    /**
      Enable/disable continuous snapshot mode.

      @param perFrame  Toggle snapshots every frame, or that specified by
                       'ssinterval' setting.
    */
    void toggleContinuousSnapshots(bool perFrame);

    /**
      Set the number of seconds between taking a snapshot in
      continuous snapshot mode.  Setting an interval of 0 disables
      continuous snapshots.

      @param interval  Interval in seconds between snapshots
    */
    void setContinuousSnapInterval(uInt32 interval);

    /**
      Create a new snapshot based on the name of the ROM, and also
      optionally using the number given as a parameter.

      @param number  Optional number to append to the snapshot name
    */
    void takeSnapshot(uInt32 number = 0);

  private:
    // Global OSystem object
    OSystem& myOSystem;

    // Used for continuous snapshot mode
    uInt32 mySnapInterval{0};
    uInt32 mySnapCounter{0};

    /**
      Write PNG tEXt chunks to the image.
    */
    static void writeMetaData(png_structp png_ptr, png_infop info_ptr,
                              const VariantList& metaData);

    /**
      Read PNG tEXt chunks from the image.
    */
    static void readMetaData(png_structp png_ptr, png_infop info_ptr,
                             VariantList& metaData);

    /** PNG library callback functions */
    static void png_read_data(png_structp ctx, png_bytep area, png_size_t size) {
      (static_cast<std::ifstream*>(png_get_io_ptr(ctx)))->read(
                                   reinterpret_cast<char*>(area), size);
    }
    static void png_write_data(png_structp ctx, png_bytep area, png_size_t size) {
      (static_cast<std::ofstream*>(png_get_io_ptr(ctx)))->write(
                                   reinterpret_cast<const char*>(area), size);
    }
    static void png_io_flush(png_structp ctx) {
      (static_cast<std::ofstream*>(png_get_io_ptr(ctx)))->flush();
    }
    static void png_user_warn(png_structp, png_const_charp str) {
      // Optional: log, but DO NOT throw
      cerr << "libpng warning: " << str << '\n';
    }
    [[noreturn]] static void png_user_error(png_structp, png_const_charp msg) {
      throw std::runtime_error(msg);
    }

  private:
    // Following constructors and assignment operators not supported
    PNGLibrary() = delete;
    PNGLibrary(const PNGLibrary&) = delete;
    PNGLibrary(PNGLibrary&&) = delete;
    PNGLibrary& operator=(const PNGLibrary&) = delete;
    PNGLibrary& operator=(PNGLibrary&&) = delete;
};

#endif

#endif  // IMAGE_SUPPORT
