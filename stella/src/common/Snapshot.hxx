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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Snapshot.hxx,v 1.15 2009-01-03 22:57:12 stephena Exp $
//============================================================================

#ifndef SNAPSHOT_HXX
#define SNAPSHOT_HXX

class Properties;
class FrameBuffer;
class MediaSource;

#include <fstream>
#include "bspf.hxx"

class Snapshot
{
  public:
    /**
      Save the current TIA image to a PNG file using data from the Framebuffer.
      Any postprocessing/filtering will be included.

      @param framebuffer The framebuffer containing the image data
      @param props       The properties object containing info about the ROM
      @param filename    The filename of the PNG file
    */
    static string savePNG(const FrameBuffer& framebuffer, const Properties& props,
                          const string& filename);

    /**
      Save the current TIA image to a PNG file using data directly from
      the MediaSource/TIA.  No filtering or scaling will be included.

      @param framebuffer The framebuffer containing the image data
      @param mediasrc    Source of the raw TIA data
      @param props       The properties object containing info about the ROM
      @param filename    The filename of the PNG file
    */
    static string savePNG(const FrameBuffer& framebuffer,
                          const MediaSource& mediasrc, const Properties& props,
                          const string& filename);

  private:
    static string saveBufferToPNG(ofstream& out, uInt8* buffer,
                                  uInt32 width, uInt32 height,
                                  const Properties& props);
    static void writePNGChunk(ofstream& out, const char* type, uInt8* data, int size);
    static void writePNGText(ofstream& out, const string& key, const string& text);
};

#endif
