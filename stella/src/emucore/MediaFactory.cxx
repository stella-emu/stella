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
// $Id: MediaFactory.cxx,v 1.2 2006-01-14 21:36:29 stephena Exp $
//============================================================================

////////////////////////////////////////////////////////////////////
// I think you can see why this mess was put into a factory class :)
////////////////////////////////////////////////////////////////////

#include "MediaFactory.hxx"

#include "OSystem.hxx"

#include "FrameBuffer.hxx"
#include "FrameBufferSoft.hxx"
#ifdef DISPLAY_OPENGL
  #include "FrameBufferGL.hxx"
#endif

#if defined(PSP)
  #include "FrameBufferPSP.hxx"
#elif defined (_WIN32_WCE)
  #include "FrameBufferWinCE.hxx"
#endif

#include "Sound.hxx"
#include "SoundNull.hxx"
#ifdef SOUND_SUPPORT
  #ifndef _WIN32_WCE
    #include "SoundSDL.hxx"
  #else
    #include "SoundWinCE.hxx"
  #endif
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer* MediaFactory::createVideo(const string& type, OSystem* parent)
{
  FrameBuffer* fb = (FrameBuffer*) NULL;

  if(type == "soft")
#if defined (PSP)
    fb = new FrameBufferPSP(parent);
#elif defined (_WIN32_WCE)
    fb = new FrameBufferWinCE(parent);
#else
    fb = new FrameBufferSoft(parent);
#endif
#ifdef DISPLAY_OPENGL
  else if(type == "gl")
  {
    const string& gl_lib = parent->settings().getString("gl_lib");
    if(FrameBufferGL::loadFuncs(gl_lib))
      fb = new FrameBufferGL(parent);
  }
#endif

  return fb;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sound* MediaFactory::createAudio(const string& type, OSystem* parent)
{
  Sound* sound = (Sound*) NULL;

#ifdef SOUND_SUPPORT
  #if defined (_WIN32_WCE)
    sound = new SoundWinCE(parent);
  #else
    sound = new SoundSDL(parent);
  #endif
#else
  sound = new SoundNull(parent);
#endif

  return sound;
}
