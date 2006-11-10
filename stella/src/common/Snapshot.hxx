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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Snapshot.hxx,v 1.7 2006-11-10 06:14:14 stephena Exp $
//============================================================================

#ifndef SNAPSHOT_HXX
#define SNAPSHOT_HXX

#ifdef SNAPSHOT_SUPPORT

class FrameBuffer;

#include <fstream>
#include "bspf.hxx"

class Snapshot
{
  public:
    /**
      Save the current frame buffer to a PNG file.

      @param framebuffer The framebuffer containing the image data
      @param filename    The filename of the PNG file
    */
    static void savePNG(FrameBuffer& framebuffer, const string& filename);

  private:
    static void writePNGChunk(ofstream& out, char* type, uInt8* data, int size);
};

#endif  // SNAPSHOT_SUPPORT

#endif
