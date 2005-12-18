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
// $Id: MediaFactory.hxx,v 1.1 2005-12-18 18:37:03 stephena Exp $
//============================================================================

#ifndef MEDIA_FACTORY_HXX
#define MEDIA_FACTORY_HXX

#include "bspf.hxx"

class FrameBuffer;
class Sound;
class OSystem;

/**
  This class deals with the different framebuffer/sound implementations
  for the various ports of Stella, and always returns a valid media object
  based on the specific port and restrictions on that port.

  @author  Stephen Anthony
  @version $Id: MediaFactory.hxx,v 1.1 2005-12-18 18:37:03 stephena Exp $
*/
class MediaFactory
{
  public:
    static FrameBuffer* createVideo(const string& type, OSystem* parent);
    static Sound* createAudio(const string& type, OSystem* parent);
};

#endif
