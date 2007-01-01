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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Snapshot.hxx,v 1.11 2007-01-01 18:04:40 stephena Exp $
//============================================================================

#ifndef SNAPSHOT_HXX
#define SNAPSHOT_HXX

class Properties;
class FrameBuffer;

#include <fstream>
#include "bspf.hxx"

class Snapshot
{
  public:
    /**
      Save the current frame buffer to a PNG file.

      @param framebuffer The framebuffer containing the image data
      @param props       The properties object containing info about the ROM
      @param filename    The filename of the PNG file
    */
    static void savePNG(FrameBuffer& framebuffer, const Properties& props,
                        const string& filename);

  private:
    static void writePNGChunk(ofstream& out, char* type, uInt8* data, int size);
    static void writePNGText(ofstream& out, const string& key, const string& text);
};

#endif
