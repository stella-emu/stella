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
// $Id: Debugger.cxx,v 1.1 2005-06-12 18:18:00 stephena Exp $
//============================================================================

#include "bspf.hxx"

#include <sstream>

#include "Version.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "DebuggerDialog.hxx"
#include "DebuggerParser.hxx"

#include "Console.hxx"
#include "System.hxx"
#include "D6502.hxx"

#include "Debugger.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::Debugger(OSystem* osystem)
  : DialogContainer(osystem),
    myConsole(NULL),
    mySystem(NULL),
    myParser(NULL),
    myDebugger(NULL)
{
  // Init parser
  myParser = new DebuggerParser(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::~Debugger()
{
  delete myParser;
  delete myDebugger;
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setConsole(Console* console)
{
  assert(console);

  // Keep pointers to these items for efficiency
  myConsole = console;
  mySystem = &(myConsole->system());

  // Create a new 6502 debugger for this console
  delete myDebugger;
  myDebugger = new D6502(mySystem);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::run(const string& command)
{
  return myParser->run(command);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::state()
{
  string result;
  char buf[255];

  result += "\nPC=";
  result += to_hex_16(myDebugger->pc());
  result += " A=";
  result += to_hex_8(myDebugger->a());
  result += " X=";
  result += to_hex_8(myDebugger->x());
  result += " Y=";
  result += to_hex_8(myDebugger->y());
  result += " S=";
  result += to_hex_8(myDebugger->sp());
  result += " P=";
  result += to_hex_8(myDebugger->ps());
  result += "\n  ";
  myDebugger->disassemble(myDebugger->pc(), buf);
  result += buf;

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Debugger::readRAM(uInt16 addr)
{
  return mySystem->peek(addr + kRamStart);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::writeRAM(uInt16 addr, uInt8 value)
{
  mySystem->poke(addr + kRamStart, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::dumpRAM(uInt16 start)
{
  string result;
  char buf[128];

  for (uInt8 i = 0x00; i < kRamStart; i += 0x10)
  {
    sprintf(buf, "%.4x: ", start+i);
    result += buf;

    for (uInt8 j = 0; j < 0x010; j++)
    {
      sprintf(buf, "%.2x ", mySystem->peek(start+i+j));
      result += buf;

      if(j == 0x07) result += "- ";
    }
    result += "\n";
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::dumpTIA()
{
  string result;
  char buf[128];

  sprintf(buf, "%.2x: ", 0);
  result += buf;
  for (uInt8 j = 0; j < 0x010; j++)
  {
    sprintf(buf, "%.2x ", mySystem->peek(j));
    result += buf;

    if(j == 0x07) result += "- ";
  }
  result += "\n";

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::quit()
{
  myOSystem->eventHandler().leaveDebugMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::trace()
{
  mySystem->m6502().execute(1);
}
