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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: OSystem.cxx,v 1.1 2005-02-21 02:23:49 stephena Exp $
//============================================================================

#include <cassert>
#include <sstream>
#include <fstream>

#include "FrameBuffer.hxx"
#include "Sound.hxx"
#include "Settings.hxx"
#include "PropsSet.hxx"
#include "EventHandler.hxx"
#include "bspf.hxx"
#include "OSystem.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::OSystem(FrameBuffer& framebuffer, Sound& sound,
                 Settings& settings, PropertiesSet& propset)
    : myFrameBuffer(framebuffer),
      mySound(sound),
      mySettings(settings),
      myPropSet(propset)
{
  // Create an event handler which will collect and dispatch events
  myEventHandler = new EventHandler(*this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::~OSystem()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::update()
{
//  myFrameBuffer.update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::OSystem(const OSystem& osystem)
    : myFrameBuffer(osystem.myFrameBuffer),
      mySound(osystem.mySound),
      mySettings(osystem.mySettings),
      myPropSet(osystem.myPropSet)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem& OSystem::operator = (const OSystem&)
{
  assert(false);

  return *this;
}
