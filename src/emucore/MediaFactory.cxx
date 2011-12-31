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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

////////////////////////////////////////////////////////////////////
// I think you can see why this mess was put into a factory class :)
////////////////////////////////////////////////////////////////////

#include "MediaFactory.hxx"

#include "OSystem.hxx"
#include "Settings.hxx"

#include "FrameBuffer.hxx"
#include "FrameBufferSoft.hxx"
#ifdef DISPLAY_OPENGL
  #include "FrameBufferGL.hxx"
#endif

#include "Sound.hxx"
#ifdef SOUND_SUPPORT
  #include "SoundSDL.hxx"
#else
  #include "SoundNull.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer* MediaFactory::createVideo(OSystem* osystem)
{
  FrameBuffer* fb = (FrameBuffer*) NULL;

  // OpenGL mode *may* fail, so we check for it first
#ifdef DISPLAY_OPENGL
  if(osystem->settings().getString("video") == "gl")
  {
    const string& gl_lib = osystem->settings().getString("gl_lib");
    if(FrameBufferGL::loadLibrary(gl_lib))
      fb = new FrameBufferGL(osystem);
  }
#endif

  // If OpenGL failed, or if it wasn't requested, create the appropriate
  // software framebuffer
  if(!fb)
    fb = new FrameBufferSoft(osystem);

  // This should never happen
  assert(fb != NULL);

  return fb;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sound* MediaFactory::createAudio(OSystem* osystem)
{
  Sound* sound = (Sound*) NULL;

#ifdef SOUND_SUPPORT
  sound = new SoundSDL(osystem);
#else
  sound = new SoundNull(osystem);
#endif

  return sound;
}
