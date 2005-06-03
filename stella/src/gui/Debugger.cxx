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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Debugger.cxx,v 1.2 2005-06-03 17:52:06 stephena Exp $
//============================================================================

#include "Version.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "DebuggerDialog.hxx"
#include "bspf.hxx"
#include "Debugger.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::Debugger(OSystem* osystem)
    : DialogContainer(osystem),
      myConsole(NULL)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::~Debugger()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initialize()
{
  int x = 0,
      y = myConsole->mediaSource().height(),
      w = kDebuggerWidth,
      h = kDebuggerHeight - y;

  delete myBaseDialog;
  myBaseDialog = new DebuggerDialog(myOSystem, this, x, y, w, h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initializeVideo()
{
  string title = string("Stella version ") + STELLA_VERSION + ": Debugger mode";
  myOSystem->frameBuffer().initialize(title, kDebuggerWidth, kDebuggerHeight, false);
}
