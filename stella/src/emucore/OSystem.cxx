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
// $Id: OSystem.cxx,v 1.4 2005-02-22 20:19:32 stephena Exp $
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
OSystem::OSystem()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::~OSystem()
{
cerr << "OSystem::~OSystem()\n";
  // Remove any game console that is currently attached
  detachConsole();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setPropertiesFiles(const string& userprops,
                                 const string& systemprops)
{
  // Set up the input and output properties files
  myPropertiesOutputFile = userprops;

  if(fileExists(userprops))
    myPropertiesInputFile = userprops;
  else if(fileExists(systemprops))
    myPropertiesInputFile = systemprops;
  else
    myPropertiesInputFile = "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setConfigFiles(const string& userconfig,
                             const string& systemconfig)
{
  // Set up the names of the input and output config files
  myConfigOutputFile = userconfig;

  if(fileExists(userconfig))
    myConfigInputFile = userconfig;
  else if(fileExists(systemconfig))
    myConfigInputFile = systemconfig;
  else
    myConfigInputFile = "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::OSystem(const OSystem& osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem& OSystem::operator = (const OSystem&)
{
  assert(false);

  return *this;
}
